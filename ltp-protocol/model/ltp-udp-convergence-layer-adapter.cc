/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Universitat Autònoma de Barcelona
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Rubén Martínez <rmartinez@deic.uab.cat>
 */

#include "ltp-udp-convergence-layer-adapter.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"


namespace ns3 {
namespace ltp {

NS_LOG_COMPONENT_DEFINE ("LtpUdpConvergenceLayerAdapter");


LtpUdpConvergenceLayerAdapter::LtpUdpConvergenceLayerAdapter ()
  : m_serverPort (0),
    m_keepAliveValue (0),
    m_rcvSocket (0),
    m_rcvSocket6 (0),
    m_l4SendSockets (),
    m_ltp (0),
    m_ltpRouting (0)
{
  NS_LOG_FUNCTION (this);

  m_linkUp = MakeNullCallback< void, Ptr<LtpConvergenceLayerAdapter> > ();
  m_linkDown = MakeNullCallback< void, Ptr<LtpConvergenceLayerAdapter>  > ();
  m_checkpointSent = MakeNullCallback< void, SessionId, RedSegmentInfo> ();
  m_reportSent = MakeNullCallback< void, SessionId, RedSegmentInfo> ();
  m_cancelSent = MakeNullCallback< void, SessionId > ();
  m_endOfBlockSent = MakeNullCallback< void, SessionId> ();

}

LtpUdpConvergenceLayerAdapter::~LtpUdpConvergenceLayerAdapter ()
{
  NS_LOG_FUNCTION (this);
}

bool LtpUdpConvergenceLayerAdapter::EnableReceive (const uint64_t &localLtpEngineId)
{
  NS_LOG_FUNCTION (this << localLtpEngineId);
  TypeId tid;
  tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  if (m_ltp)
    {
      m_rcvSocket = Socket::CreateSocket (m_ltp->GetNode (), tid);
      m_rcvSocket6 = Socket::CreateSocket (m_ltp->GetNode (), tid);

      m_rcvSocket->Bind ( InetSocketAddress (Ipv4Address::GetAny (),m_serverPort));
      m_rcvSocket->SetRecvCallback (MakeCallback (&LtpUdpConvergenceLayerAdapter::Receive,  this));

      m_rcvSocket6->Bind6 ();
      m_rcvSocket6->SetRecvCallback (MakeCallback (&LtpUdpConvergenceLayerAdapter::Receive,  this));

      return true;
    }

  else
    {
      NS_LOG_DEBUG ("Protocol instance is not assigned");
      return false;
    }
}

TypeId
LtpUdpConvergenceLayerAdapter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LtpUdpConvergenceLayerAdapter")
    .SetParent<LtpConvergenceLayerAdapter> ()
    .AddConstructor<LtpUdpConvergenceLayerAdapter> ()
    .AddAttribute ("ServerPort", "UDP port to listen for incoming transmissions",
                   UintegerValue (1113),
                   MakeUintegerAccessor (&LtpUdpConvergenceLayerAdapter::m_serverPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("KeepAlive", "Keep-Alive Timeout",
                   UintegerValue (20),
                   MakeUintegerAccessor (&LtpUdpConvergenceLayerAdapter::m_keepAliveValue),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}


uint32_t LtpUdpConvergenceLayerAdapter::Send (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  Address addr = m_ltpRouting->GetRoute (m_peerLtpEngineId);

  Ptr<Socket> socket = 0;
  std::map<uint64_t, Ptr<Socket> >::const_iterator it_sock;
  it_sock = m_l4SendSockets.find (m_peerLtpEngineId);

  if (it_sock == m_l4SendSockets.end ())
    {
      TypeId tid;
      tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      socket = Socket::CreateSocket (m_ltp->GetNode (), tid);

      if (m_ltpRouting->GetAddressMode () == 0)
        {
          socket->Bind ();
          socket->Connect (addr);
        }
      m_l4SendSockets.insert (std::pair<uint64_t, Ptr<Socket> > (m_peerLtpEngineId,socket));

    }
  else
    {
      NS_LOG_DEBUG ("LtpUdpConvergenceLayerAdapter:: Reuse socket");
      socket = it_sock->second;
    }

  uint32_t bytes = socket->Send (p);

  LtpHeader header;
  LtpContentHeader contentHeader;

  Ptr<Packet> packet = p->Copy ();

  packet->RemoveHeader (header);
  contentHeader.SetSegmentType (header.GetSegmentType ());
  packet->RemoveHeader (contentHeader);

  RedSegmentInfo info;

  switch (header.GetSegmentType ())
    {
    case LTPTYPE_RD_CP_EORP_EOB:
      if (!m_endOfBlockSent.IsNull ())
        {
          m_endOfBlockSent (m_activeSessionId);
        }
    case LTPTYPE_RD_CP_EORP:
    case LTPTYPE_RD_CP:
      if (!m_checkpointSent.IsNull ())
        {

          info.CpserialNum = contentHeader.GetCpSerialNumber ();
          info.RpserialNum = contentHeader.GetRpSerialNumber ();
          info.high_bound = contentHeader.GetUpperBound ();
          info.low_bound = contentHeader.GetLowerBound ();
  
          LtpContentHeader::ReceptionClaim claim;
          claim.offset = contentHeader.GetOffset ();
          claim.length = contentHeader.GetLength ();
          info.claims.insert(claim);

          m_checkpointSent (m_activeSessionId, info); // Link State Cue CP Tx start timer
        }
      break;
    case LTPTYPE_GD_EOB:
      m_endOfBlockSent (m_activeSessionId);
      break;
    case LTPTYPE_RS:
      if (!m_reportSent.IsNull ())
        {
          info.CpserialNum = contentHeader.GetCpSerialNumber ();
          info.RpserialNum = contentHeader.GetRpSerialNumber ();
          info.high_bound = contentHeader.GetUpperBound ();
          info.low_bound = contentHeader.GetLowerBound ();

          m_reportSent (m_activeSessionId,info);          // Link State Cue RS Tx start timer
        }
      break;
    case LTPTYPE_CS:
      if (!m_cancelSent.IsNull ())
        {
          m_cancelSent (m_activeSessionId);   // Link State Cue CX Tx start timers
        }
      break;
    default:
      break;
    }

  return bytes;

}


void LtpUdpConvergenceLayerAdapter::Receive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << " " << socket);

  Ptr<Packet> packet;
  Address peer;
  while ((packet = socket->RecvFrom (peer)))
    {
	Simulator::ScheduleNow ( &LtpProtocol::Receive, m_ltp, packet, this);
    }
}

uint16_t LtpUdpConvergenceLayerAdapter::GetMtu () const
{
  NS_LOG_FUNCTION (this);

  Address addr = m_ltpRouting->GetRoute (m_peerLtpEngineId);

  Ptr<Socket> socket = Socket::CreateSocket (m_ltp->GetNode (), UdpSocketFactory::GetTypeId ());
  socket->Bind ();
  socket->Connect (addr);

  Ipv4Header iph;
  UdpHeader udph;

  uint16_t  mtu =  socket->GetTxAvailable () - iph.GetSerializedSize () - udph.GetSerializedSize ();

  return (uint16_t) mtu;

}


void LtpUdpConvergenceLayerAdapter::SetProtocol (Ptr<LtpProtocol> prot)
{
  NS_LOG_FUNCTION (this << " " << prot);
  m_ltp = prot;
}

Ptr<LtpProtocol> LtpUdpConvergenceLayerAdapter::GetProtocol () const
{
  NS_LOG_FUNCTION (this);
  return m_ltp;
}

void LtpUdpConvergenceLayerAdapter::SetRoutingProtocol (Ptr<LtpIpResolutionTable> prot)
{
  NS_LOG_FUNCTION (this << " " << prot);
  m_ltpRouting = prot;
}

Ptr<LtpIpResolutionTable> LtpUdpConvergenceLayerAdapter::GetRoutingProtocol () const
{
  NS_LOG_FUNCTION (this);
  return m_ltpRouting;
}




} //namespace ltp
} //namespace ns3

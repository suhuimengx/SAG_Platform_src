#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/assert.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/object-vector.h"

#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/socket-factory.h"

#include "bp-ltp-cla-protocol.h"
#include "bp-cla-protocol.h"
#include "bundle-protocol.h"
#include "bp-static-routing-protocol.h"
#include "bp-header.h"
#include "bp-endpoint-id.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"

#define DTN_BUNDLE_LTP_PORT 1113

NS_LOG_COMPONENT_DEFINE ("BpLtpClaProtocol");

namespace ns3 {
namespace ltp {

NS_OBJECT_ENSURE_REGISTERED (BpLtpClaProtocol);

TypeId 
BpLtpClaProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BpLtpClaProtocol")
    .SetParent<BpClaProtocol> ()
    .AddConstructor<BpLtpClaProtocol> ()
  ;
  return tid;
}

BpLtpClaProtocol::BpLtpClaProtocol ()
  :m_bp (0),
   m_bpRouting (0)
{ 
  NS_LOG_FUNCTION (this);
}


BpLtpClaProtocol::~BpLtpClaProtocol ()
{ 
  NS_LOG_FUNCTION (this);
}

void 
BpLtpClaProtocol::SetBundleProtocol (Ptr<BundleProtocol> bundleProtocol)
{ 
  NS_LOG_FUNCTION (this << " " << bundleProtocol);
  m_bp = bundleProtocol;
}

void
BpLtpClaProtocol::SetLtpProtocol (Ptr<LtpProtocol> ltpProtocol)
{
  NS_LOG_FUNCTION (this);
  m_ltp = ltpProtocol;
  m_ltp->SetBpCallback(MakeCallback(&BpLtpClaProtocol::PacketRecv, this));
}

Ptr<Socket>
BpLtpClaProtocol::GetL4Socket (Ptr<Packet> packet)
{ 
  NS_LOG_FUNCTION (this << " " << packet);
  BpHeader bph;
  packet->PeekHeader (bph);
  BpEndpointId dst = bph.GetDestinationEid ();
  BpEndpointId src = bph.GetSourceEid ();

  std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4SendSockets.end ();
  it = m_l4SendSockets.find (src);
  if (it == m_l4SendSockets.end ())
    {
      if (EnableSend (src, dst) < 0)
        return NULL;
    }

  it = m_l4SendSockets.find (src);

  return ((*it).second);
}

int
BpLtpClaProtocol::EnableReceive (const BpEndpointId &local)
{
  NS_LOG_FUNCTION (this << " " << local.Uri ());
  Ptr<BpStaticRoutingProtocol> route = DynamicCast <BpStaticRoutingProtocol> (m_bpRouting);
  InetSocketAddress addr = route->GetRoute (local);

  uint16_t port;
  InetSocketAddress defaultAddr ("127.0.0.1", 0);
  if (addr == defaultAddr)
    port = DTN_BUNDLE_LTP_PORT;
  else
    port = addr.GetPort ();

  InetSocketAddress address (Ipv4Address::GetAny (), port);

  // set LTP socket in listen state
//  Ptr<Socket> socket = Socket::CreateSocket (m_bp->GetNode (), ltp::LtpProtocol::GetTypeId ());
//  if (socket->Bind (address) < 0)
//    return -1;
//  if (socket->Listen () < 0)
//    return -1;
//  if (socket->ShutdownSend () < 0)
//    return -1;
//
//  SetL4SocketCallbacks (socket);

  // store the receiving socket so that the convergence layer can dispatch the bundles to different LTP connections
//  std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4RecvSockets.end ();
//  it = m_l4RecvSockets.find (local);
//  if (it == m_l4RecvSockets.end ())
//    m_l4RecvSockets.insert (std::pair<BpEndpointId, Ptr<Socket> >(local, socket));
//  else
//    return -1;

  return 0;
}

int
BpLtpClaProtocol::DisableReceive (const BpEndpointId &local)
{
  NS_LOG_FUNCTION (this << " " << local.Uri ());
  std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4RecvSockets.end ();
  it = m_l4RecvSockets.find (local);
  if (it == m_l4RecvSockets.end ())
    {
      return -1;
    }
  else
    {
      // close the LTP connection
      return ((*it).second)->Close ();
    }

  return 0;
}



int
BpLtpClaProtocol::SendPacket (Ptr<Packet> packet)
{ 
  NS_LOG_FUNCTION (this << " " << packet);

  BpHeader bph;
  packet->PeekHeader (bph);
  BpEndpointId src = bph.GetSourceEid ();
  Ptr<Packet> pkt = m_bp->GetBundle (src);

  if (pkt)
    {
	  uint32_t size = pkt->GetSize();
      std::vector<uint8_t> block;
	  block.resize(size);
      pkt->CopyData(block.data(), size);

	  uint64_t ClientServiceId = 1;
      uint64_t receiverLtpId = 1;
      //std::cout<<"bp-cla-ltp "<<size<<" "<<std::endl;
      //just for example test here
      if (size>=1500)
	   m_bp->GetNode()->GetObject<ltp::LtpProtocol>()->StartTransmission(ClientServiceId,ClientServiceId,receiverLtpId,block,1500);
      else
    	  m_bp->GetNode()->GetObject<ltp::LtpProtocol>()->StartTransmission(ClientServiceId,ClientServiceId,receiverLtpId,block,0);
      return 0;
    }

  return -1;
}


int 
BpLtpClaProtocol::EnableSend (const BpEndpointId &src, const BpEndpointId &dst)
{ 
  NS_LOG_FUNCTION (this << " " << src.Uri () << " " << dst.Uri ());
  if (!m_bpRouting)
    NS_FATAL_ERROR ("BpLtpClaProtocol::SendPacket (): cannot find bundle routing protocol");

  Ptr<BpStaticRoutingProtocol> route = DynamicCast <BpStaticRoutingProtocol> (m_bpRouting);
  InetSocketAddress address = route->GetRoute (dst);

  InetSocketAddress defaultAddr ("127.0.0.1", 0);
  if (address == defaultAddr)
    {
      NS_LOG_DEBUG ("BpLtpClaProtocol::EnableSend (): cannot find route for destination endpoint id " << dst.Uri ());
      return -1;
    }

//  Ptr<Socket> socket = Socket::CreateSocket (m_bp->GetNode (), ltp::LtpProtocol::GetTypeId ());
//  if (socket->Bind () < 0)
//    return -1;
//  if (socket->Connect (address) < 0)
//    return -1;
//  if (socket->ShutdownRecv () < 0)
//    return -1;
//
//  SetL4SocketCallbacks (socket);
//
//  std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4SendSockets.end ();
//  it = m_l4SendSockets.find (src);
//  if (it == m_l4SendSockets.end ())
//    m_l4SendSockets.insert (std::pair<BpEndpointId, Ptr<Socket> >(src, socket));
//  else
//    return -1;

  return 0;
}

void 
BpLtpClaProtocol::SetL4SocketCallbacks (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
  socket->SetConnectCallback (
    MakeCallback (&BpLtpClaProtocol::ConnectionSucceeded, this),
    MakeCallback (&BpLtpClaProtocol::ConnectionFailed, this));

  socket->SetCloseCallbacks (
    MakeCallback (&BpLtpClaProtocol::NormalClose, this),
    MakeCallback (&BpLtpClaProtocol::ErrorClose, this));

  socket->SetAcceptCallback (
    MakeCallback (&BpLtpClaProtocol::ConnectionRequest, this),
    MakeCallback (&BpLtpClaProtocol::NewConnectionCreated, this));

  socket->SetDataSentCallback (
    MakeCallback (&BpLtpClaProtocol::DataSent, this));

  socket->SetSendCallback (
    MakeCallback (&BpLtpClaProtocol::Sent, this));

  socket->SetRecvCallback (
    MakeCallback (&BpLtpClaProtocol::DataRecv, this));
}

void 
BpLtpClaProtocol::ConnectionSucceeded (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
} 

void 
BpLtpClaProtocol::ConnectionFailed (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
}

void 
BpLtpClaProtocol::NormalClose (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
}

void 
BpLtpClaProtocol::ErrorClose (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
}

bool
BpLtpClaProtocol::ConnectionRequest (Ptr<Socket> socket, const Address &address)
{ 
  NS_LOG_FUNCTION (this << " " << socket << " " << address);
  return true;
}

void 
BpLtpClaProtocol::NewConnectionCreated (Ptr<Socket> socket, const Address &address)
{ 
  NS_LOG_FUNCTION (this << " " << socket << " " << address);
  SetL4SocketCallbacks (socket);  // reset the callbacks due to fork in TcpSocketBase
}

void 
BpLtpClaProtocol::DataSent (Ptr<Socket> socket, uint32_t size)
{ 
  NS_LOG_FUNCTION (this << " " << socket << " " << size);
}

void 
BpLtpClaProtocol::Sent (Ptr<Socket> socket, uint32_t size)
{ 
  NS_LOG_FUNCTION (this << " " << socket << " " << size);
}

void 
BpLtpClaProtocol::DataRecv (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
   {
     m_bp->ReceivePacket (packet);
   }
}

void
BpLtpClaProtocol::PacketRecv (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << " " << packet);
  m_bp->ReceivePacket (packet);
}

void
BpLtpClaProtocol::SetRoutingProtocol (Ptr<BpRoutingProtocol> route)
{ 
  NS_LOG_FUNCTION (this << " " << route);
  m_bpRouting = route;
}

Ptr<BpRoutingProtocol>
BpLtpClaProtocol::GetRoutingProtocol ()
{ 
  NS_LOG_FUNCTION (this);
  return m_bpRouting;
}

} // namespace ns3
}

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering, University of Padova
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
 * Authors: Alvise De Biasio <alvise.debiasio@gmail.com>
 *          Federico Chiariotti <chiariotti.federico@gmail.com>
 *          Michele Polese <michele.polese@gmail.com>
 *          Davide Marcato <davidemarcato@outlook.com>
 *
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/object-vector.h"
#include "ns3/pointer.h"

#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"

#include "quic-l4-protocol.h"
#include "quic-header.h"
#include "ns3/ipv4-end-point-demux.h"
#include "ns3/ipv6-end-point-demux.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/ipv6-end-point.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "quic-socket-factory.h"
#include "ns3/tcp-congestion-ops.h"
#include "quic-congestion-ops.h"
#include "ns3/rtt-estimator.h"
#include "ns3/random-variable-stream.h"

#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <math.h>
#include <iostream>
//Mengy's::s
#include "ns3/id_seq_tag.h"
#include "quic-bbr.h"



namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (QuicL4Protocol);
NS_OBJECT_ENSURE_REGISTERED (QuicUdpBinding);
NS_LOG_COMPONENT_DEFINE ("QuicL4Protocol");

QuicUdpBinding::QuicUdpBinding ()
  : m_budpSocket (0),
  m_budpSocket6 (0),
  m_quicSocket (nullptr),
  m_listenerBinding (false)
{
  NS_LOG_FUNCTION (this);
}

QuicUdpBinding::~QuicUdpBinding ()
{
  NS_LOG_FUNCTION (this);

  m_budpSocket = 0;
  m_budpSocket6 = 0;
  m_quicSocket = nullptr;
  m_listenerBinding = false;
}

TypeId
QuicUdpBinding::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QuicUdpBinding")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
    .AddConstructor<QuicUdpBinding> ()
    .AddAttribute ("QuicSocketBase", "The QuicSocketBase pointer.",
                   PointerValue (),
                   MakePointerAccessor (&QuicUdpBinding::m_quicSocket),
                   MakePointerChecker<QuicSocketBase> ())
  ;
  //NS_LOG_UNCOND("QuicUdpBinding");
  return tid;
}

TypeId
QuicUdpBinding::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}



#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_node) { std::clog << " [node " << m_node->GetId () << "] "; }

/* see http://www.iana.org/assignments/protocol-numbers */
const uint8_t QuicL4Protocol::PROT_NUMBER = 143;

TypeId
QuicL4Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QuicL4Protocol")
    .SetParent<IpL4Protocol> ()
    .SetGroupName ("Internet")
    .AddConstructor<QuicL4Protocol> ()
    .AddAttribute ("RttEstimatorType",
                   "Type of RttEstimator objects.",
                   TypeIdValue (RttMeanDeviation::GetTypeId ()),
                   MakeTypeIdAccessor (&QuicL4Protocol::m_rttTypeId),
                   MakeTypeIdChecker ())
    .AddAttribute ("0RTT-Handshake", "0RTT-Handshake start",
                   BooleanValue (false),
                   MakeBooleanAccessor (&QuicL4Protocol::m_0RTTHandshakeStart),
                   MakeBooleanChecker ())
    .AddAttribute ("SocketType",
                   "Socket type of QUIC objects.",
                   TypeIdValue (QuicBbr::GetTypeId ()),
                   MakeTypeIdAccessor (&QuicL4Protocol::m_congestionTypeId),
                   MakeTypeIdChecker ())
    .AddAttribute ("SocketList", "The list of UDP and QUIC sockets associated to this protocol.",
                   ObjectVectorValue (),
                   MakeObjectVectorAccessor (&QuicL4Protocol::m_quicUdpBindingList),
                   MakeObjectVectorChecker<QuicUdpBinding> ())
    /*.AddAttribute ("AuthAddresses", "The list of Authenticated addresses associated to this protocol.",
                                           ObjectVectorValue (),
                                           MakeObjectVectorAccessor (&QuicL4Protocol::m_authAddresses),
                                           MakeObjectVectorChecker<Address> ())*/
  ;
  return tid;
}

QuicL4Protocol::QuicL4Protocol ()
  : m_node (0),
  m_0RTTHandshakeStart (false),
  m_isServer (false),
  m_endPoints (new Ipv4EndPointDemux ()),
  m_endPoints6 (new Ipv6EndPointDemux ())
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_LOGIC ("Created QuicL4Protocol object " << this);

  m_quicUdpBindingList = QuicUdpBindingList ();
  //Mengy's::
  m_quicConnectionNum = 0;
}

QuicL4Protocol::~QuicL4Protocol ()
{
  NS_LOG_FUNCTION (this);
  m_quicUdpBindingList.clear ();
}

void
QuicL4Protocol::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_FUNCTION (this << node);

  m_node = node;
}

Ptr<Node> 
QuicL4Protocol::GetNode (void)
{
  return m_node;
}

int
QuicL4Protocol::UdpBind (Ptr<QuicSocketBase> socket)
{
  NS_LOG_FUNCTION (this << socket);

  int res = -1;
  QuicUdpBindingList::iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == socket and item->m_budpSocket == nullptr)
        {
          Ptr<Socket> udpSocket = CreateUdpSocket ();
          res = udpSocket->Bind ();
          item->m_budpSocket = udpSocket;
          break;
        }
    }

  return res;
}

int
QuicL4Protocol::UdpBind6 (Ptr<QuicSocketBase> socket)
{
  NS_LOG_FUNCTION (this << socket);

  int res = -1;
  QuicUdpBindingList::iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == socket and item->m_budpSocket6 == nullptr)
        {
          Ptr<Socket> udpSocket6 = CreateUdpSocket6 ();
          res = udpSocket6->Bind ();
          item->m_budpSocket6 = udpSocket6;
          break;
        }
    }

  return res;
}

int
QuicL4Protocol::UdpBind (const Address &address, Ptr<QuicSocketBase> socket)
{
  NS_LOG_FUNCTION (this << address << socket);

  int res = -1;
  if (InetSocketAddress::IsMatchingType (address))
    {
      QuicUdpBindingList::iterator it;
      for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
        {
          Ptr<QuicUdpBinding> item = *it;
          if (item->m_quicSocket == socket and item->m_budpSocket == nullptr)
            {
              Ptr<Socket> udpSocket = CreateUdpSocket ();
              res = udpSocket->Bind (address);
              item->m_budpSocket = udpSocket;
              break;
            }
        }

      return res;
    }
  else if (Inet6SocketAddress::IsMatchingType (address))
    {
      QuicUdpBindingList::iterator it;
      for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
        {
          Ptr<QuicUdpBinding> item = *it;
          if (item->m_quicSocket == socket and item->m_budpSocket6 == nullptr)
            {
              Ptr<Socket> udpSocket6 = CreateUdpSocket ();
              res = udpSocket6->Bind (address);
              item->m_budpSocket6 = udpSocket6;
              break;
            }
        }

      return res;
    }
  return -1;
}

int
QuicL4Protocol::UdpConnect (const Address & address, Ptr<QuicSocketBase> socket)
{
  NS_LOG_FUNCTION (this << address << socket);
  if (InetSocketAddress::IsMatchingType (address) == true)
    {
      UdpBind (address, socket);

      QuicUdpBindingList::iterator it;
      for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
        {
          Ptr<QuicUdpBinding> item = *it;
          if (item->m_quicSocket == socket)
            {
              return item->m_budpSocket->Connect (address);
            }
        }

      NS_LOG_INFO ("UDP Socket: Connecting");

    }
  else if (Inet6SocketAddress::IsMatchingType (address) == true)
    {
      UdpBind (address, socket);

      QuicUdpBindingList::iterator it;
      for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
        {
          Ptr<QuicUdpBinding> item = *it;
          if (item->m_quicSocket == socket)
            {
              return item->m_budpSocket6->Connect (address);
            }
        }
      NS_LOG_INFO ("UDP Socket: Connecting");

    }
  NS_LOG_WARN ("UDP Connection Failed");
  return -1;
}

int
QuicL4Protocol::UdpSend (Ptr<Socket> udpSocket, Ptr<Packet> p, uint32_t flags) const
{
  NS_LOG_FUNCTION (this << udpSocket);

  return udpSocket->Send (p, flags);
}

Ptr<Packet>
QuicL4Protocol::UdpRecv (Ptr<Socket> udpSocket, uint32_t maxSize, uint32_t flags, Address &address)
{
  NS_LOG_FUNCTION (this);

  return udpSocket->RecvFrom (maxSize, flags, address);
}

uint32_t
QuicL4Protocol::GetTxAvailable (Ptr<QuicSocketBase> quicSocket) const
{
  NS_LOG_FUNCTION (this);

  QuicUdpBindingList::const_iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == quicSocket)
        {
          return item->m_budpSocket->GetTxAvailable ();
        }
    }
  return 0;
}

uint32_t
QuicL4Protocol::GetRxAvailable (Ptr<QuicSocketBase> quicSocket) const
{
  NS_LOG_FUNCTION (this);

  QuicUdpBindingList::const_iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == quicSocket)
        {
          return item->m_budpSocket->GetRxAvailable ();
        }
    }
  return 0;
}

int
QuicL4Protocol::GetSockName (const ns3::QuicSocketBase* quicSocket, Address &address) const
{
  NS_LOG_FUNCTION (this);

  QuicUdpBindingList::const_iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == quicSocket)
        {
          return item->m_budpSocket->GetSockName (address);
        }
    }

  return -1;
}

int
QuicL4Protocol::GetPeerName (const ns3::QuicSocketBase* quicSocket, Address &address) const
{
  NS_LOG_FUNCTION (this);

  QuicUdpBindingList::const_iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == quicSocket)
        {
          return item->m_budpSocket->GetPeerName (address);
        }
    }

  return -1;
}

void
QuicL4Protocol::BindToNetDevice (Ptr<QuicSocketBase> quicSocket, Ptr<NetDevice> netdevice)
{
  NS_LOG_FUNCTION (this);

  QuicUdpBindingList::iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == quicSocket)
        {
          item->m_budpSocket->BindToNetDevice (netdevice);
        }
    }
}

bool
QuicL4Protocol::SetListener (Ptr<QuicSocketBase> sock)
{
  NS_LOG_FUNCTION (this);

  if (sock != nullptr )
    {
      m_isServer = true;
      if (m_quicUdpBindingList[0]->m_quicSocket == nullptr)
      {
        NS_ASSERT_MSG(0 == 1, "Set Listen Error!");
      }
      //Mengy's::
      uint64_t connectionId = m_quicUdpBindingList.back ()->m_quicSocket->GetConnectionId();
      m_quicUdpBindingList.back ()->m_quicSocket->SetConnectionId(connectionId + 100);
      m_quicUdpBindingList.back ()->m_quicSocket = sock;
      m_quicUdpBindingList.back ()->m_listenerBinding = true;

      return true;
    }

  return false;
}

bool
QuicL4Protocol::IsServer (void)  const
{
  return m_isServer;
}

const std::vector<Address>&
QuicL4Protocol::GetAuthAddresses () const
{
  return m_authAddresses;
}

void
QuicL4Protocol::ForwardUp (Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this);

  Address from;
  Ptr<Packet> packet;

  while ((packet = sock->RecvFrom (from)))
    {
      NS_LOG_INFO ("Receiving packet on UDP socket");
      //packet->Print (std::clog);
      // NS_LOG_INFO ("");
      //std::cout<<"Start Print Tag!"<<std::endl;
      //packet->PrintPacketTags (std::cout);


      QuicHeader header;
      packet->RemoveHeader (header);

      bool find =false;
      if(m_isServer)
      {
        #if 1
        std::string filePath = "/home/liyisen/tarballs/SAG_Platform/data/test_data/logs_ns3/QuicL4Recv_"
        + std::to_string(m_node->GetId()) +"_" + std::to_string(InetSocketAddress::ConvertFrom(from).GetPort()) + ".txt";
        std::ofstream file(filePath, std::ios::app);
        if(file.is_open()){
            file << (int)packet->GetSize() << ","<<Simulator::Now().GetMilliSeconds() << std::endl;
            file.close();
        }
        #endif

        if(m_node->GetId() == 1589 && InetSocketAddress::ConvertFrom(from).GetPort() == 49153)
        {
          find = true;
        }
      }
      

      //std::cout<<"Recv Size: "<<packet->GetSize()<<std::endl;

      uint64_t connectionId;
      if (header.HasConnectionId ())
        {
          connectionId = header.GetConnectionId ();
        }
      /*else if (m_sockets.size () <= 2) // Rivedere
        {
          if (m_sockets[0]->GetSocketState () != QuicSocket::LISTENING)
            {
              connectionId = m_sockets[0]->GetConnectionId ();
            }
          else if (m_sockets.size () == 2 && m_sockets[1]->GetSocketState () != QuicSocket::LISTENING)
            {
              connectionId = m_sockets[1]->GetConnectionId ();
            }
          else
            {
              NS_FATAL_ERROR ("The Connection ID can only be omitted by means of m_omit_connection_id transport parameter"
                              " if source and destination IP address and port are sufficient to identify a connection");
            }

        }*/
      else
        {
          NS_FATAL_ERROR ("The Connection ID can only be omitted by means of m_omit_connection_id transport parameter"
                          " if source and destination IP address and port are sufficient to identify a connection");
        }


      QuicUdpBindingList::iterator it;
      Ptr<QuicSocketBase> socket;

      for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
        {
          Ptr<QuicUdpBinding> item = *it;;

          if (item->m_quicSocket->GetConnectionId () == connectionId)
            {
              socket = item->m_quicSocket;
              break;
            }

        }


      NS_LOG_LOGIC ((socket == nullptr));
      /*NS_LOG_INFO ("Initial " << header.IsInitial ());
      NS_LOG_INFO ("Handshake " << header.IsHandshake ());
      NS_LOG_INFO ("Short " << header.IsShort ());
      NS_LOG_INFO ("Version Negotiation " << header.IsVersionNegotiation ());
      NS_LOG_INFO ("Retry " << header.IsRetry ());
      NS_LOG_INFO ("0Rtt " << header.IsORTT ());*/

      if (header.IsInitial () and m_isServer and socket == nullptr)
        {
          NS_LOG_LOGIC (this << " Cloning listening socket " << m_quicUdpBindingList[(int)connectionId]->m_quicSocket);
          std::cout<<"Succeed in Cloning Socket: "<<m_quicUdpBindingList[(int)connectionId]->m_quicSocket<<std::endl;
          socket = CloneSocket (m_quicUdpBindingList[(int)connectionId]->m_quicSocket);
          socket->SetConnectionId (connectionId);
          socket->Connect (from);
          socket->SetupCallback ();

          if(find)
          {
            std::cout<<"End Initial"<<std::endl;
          }

        }
      else if (header.IsHandshake () and m_isServer and socket != nullptr)
        {
          if(find)
          {
            std::cout<<"End handshake for Server"<<std::endl;
          }
          NS_LOG_LOGIC ("CONNECTION AUTHENTICATED - Server authenticated Client " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                        InetSocketAddress::ConvertFrom (from).GetPort () << "");
          m_authAddresses.push_back (InetSocketAddress::ConvertFrom (from).GetIpv4 ()); //add to the list of authenticated sockets
        }
      else if (header.IsHandshake () and !m_isServer and socket != nullptr)
        {
          NS_LOG_LOGIC ("CONNECTION AUTHENTICATED - Client authenticated Server " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                        InetSocketAddress::ConvertFrom (from).GetPort () << "");
          m_authAddresses.push_back (InetSocketAddress::ConvertFrom (from).GetIpv4 ()); //add to the list of authenticated sockets
        }
      else if (header.IsORTT () and m_isServer)
        {
          if(find)
          {
            std::cout<<"End 0RTT for Server"<<std::endl;
          }
          auto result = std::find (m_authAddresses.begin (), m_authAddresses.end (), InetSocketAddress::ConvertFrom (from).GetIpv4 ());
          // check if a 0-RTT is allowed with this endpoint - or if the attribute m_0RTTHandshakeStart has been forced to be true
          if (result == m_authAddresses.end () && m_0RTTHandshakeStart)
            {
              m_authAddresses.push_back (InetSocketAddress::ConvertFrom (from).GetIpv4 ()); //add to the list of authenticated sockets
            }
          else if (result == m_authAddresses.end () && !m_0RTTHandshakeStart)
            {
              NS_LOG_WARN ( this << " CONNECTION ABORTED: 0RTT Packet from unauthenticated address " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                            InetSocketAddress::ConvertFrom (from).GetPort ());
              continue;
            }

          NS_LOG_LOGIC ("CONNECTION AUTHENTICATED - Server authenticated Client " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                        InetSocketAddress::ConvertFrom (from).GetPort () << "");
          NS_LOG_LOGIC ( this << " Cloning listening socket " << m_quicUdpBindingList.front ()->m_quicSocket);
          socket = CloneSocket (m_quicUdpBindingList.front ()->m_quicSocket);
          socket->SetConnectionId (connectionId);
          socket->Connect (from);
          socket->SetupCallback ();

        }
      else if (header.IsShort ())
        {

          auto result = std::find (m_authAddresses.begin (), m_authAddresses.end (), InetSocketAddress::ConvertFrom (from).GetIpv4 ());

          if (result == m_authAddresses.end () && m_0RTTHandshakeStart)
            {
              m_authAddresses.push_back (InetSocketAddress::ConvertFrom (from).GetIpv4 ()); //add to the list of authenticated sockets
            }
          else if (result == m_authAddresses.end () && !m_0RTTHandshakeStart)
            {
              NS_LOG_WARN ( this << " CONNECTION ABORTED: Short Packet from unauthenticated address " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                            InetSocketAddress::ConvertFrom (from).GetPort ());
              //continue;
            }
        }
      

      // Handle callback for the correct socket
      if (!m_socketHandlers[socket].IsNull ())
        {
          NS_LOG_LOGIC (this << " waking up handler of socket " << socket);
          m_socketHandlers[socket] (packet, header, from);
        }
      else
        {
          NS_FATAL_ERROR ( this << " no handler for socket " << socket);
        }
    }
}

void
QuicL4Protocol::SetRecvCallback (Callback<void, Ptr<Packet>, const QuicHeader&,  Address& > handler, Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this);

  m_socketHandlers.insert ( std::pair< Ptr<Socket>, Callback<void, Ptr<Packet>, const QuicHeader&, Address& > > (sock,handler));
  QuicUdpBindingList::iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == sock && item->m_budpSocket != 0)
        {
          item->m_budpSocket->SetRecvCallback (MakeCallback (&QuicL4Protocol::ForwardUp, this));
          break;
        }
      else if (item->m_quicSocket == sock && item->m_budpSocket6 != 0)
        {
          item->m_budpSocket6->SetRecvCallback (MakeCallback (&QuicL4Protocol::ForwardUp, this));
          break;
        }
      else if (item->m_quicSocket == sock)
        {
          NS_FATAL_ERROR ("The UDP socket for this QuicUdpBinding item is not set");
        }
    }
}

void
QuicL4Protocol::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Node> node = this->GetObject<Node> ();

  if (m_node == 0)
    {
      if ((node != 0))
        {
          this->SetNode (node);
          Ptr<QuicSocketFactory> quicFactory = CreateObject<QuicSocketFactory> ();
          quicFactory->SetQuicL4 (this);
          node->AggregateObject (quicFactory);
        }
    }

  IpL4Protocol::NotifyNewAggregate ();
}

int
QuicL4Protocol::GetProtocolNumber (void) const
{
  return PROT_NUMBER;
}

void
QuicL4Protocol::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_quicUdpBindingList.clear ();

  m_node = 0;
//  m_downTarget.Nullify ();
//  m_downTarget6.Nullify ();
  IpL4Protocol::DoDispose ();
}

Ptr<QuicSocketBase>
QuicL4Protocol::CloneSocket (Ptr<QuicSocketBase> oldsock)
{
  NS_LOG_FUNCTION (this);
  Ptr<QuicSocketBase> newsock = CopyObject<QuicSocketBase> (oldsock);
  NS_LOG_LOGIC (this << " cloned socket " << oldsock << " to socket " << newsock);
  Ptr<QuicUdpBinding> udpBinding = CreateObject<QuicUdpBinding> ();
  udpBinding->m_budpSocket = nullptr;
  udpBinding->m_budpSocket6 = nullptr;
  udpBinding->m_quicSocket = newsock;
  m_quicUdpBindingList.insert (m_quicUdpBindingList.end (), udpBinding);

  return newsock;
}



Ptr<Socket>
QuicL4Protocol::CreateSocket ()
{
  return CreateSocket (m_congestionTypeId);
}

Ptr<Socket>
QuicL4Protocol::CreateSocket (TypeId congestionTypeId)
{
  NS_LOG_FUNCTION (this);

  ObjectFactory congestionAlgorithmFactory;
  congestionAlgorithmFactory.SetTypeId (m_congestionTypeId);

  // create the socket
  Ptr<QuicSocketBase> socket = CreateObject<QuicSocketBase> ();
  // create the congestion control algorithm
  Ptr<TcpCongestionOps> algo = congestionAlgorithmFactory.Create<TcpCongestionOps> ();
  socket->SetCongestionControlAlgorithm (algo);

  // TODO consider if rttFactory is needed
  // Ptr<RttEstimator> rtt = rttFactory.Create<RttEstimator> ();
  // socket->SetRtt (rtt);

  socket->SetNode (m_node);
  socket->SetQuicL4 (this);

  socket->InitializeScheduling ();

  // generate a random connection ID and check that has not been assigned to other
  // sockets associated to this L4 protocol
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();

  bool found = false;
  uint64_t connectionId;
  while (not found)
    {
      //Mengy's::
      connectionId = (uint64_t)(m_quicConnectionNum);
      found = true;
      for (auto it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
        {
          found = false;
          if (connectionId == (*it)->m_quicSocket->GetConnectionId ())
            {
              break;
            }
          found = true;
        }
    }
  //Mengy's::
  m_quicConnectionNum ++;
  socket->SetConnectionId (connectionId);
  //std::cout<<"My Connection ID: "<<connectionId<<std::endl;
  Ptr<QuicUdpBinding> udpBinding = Create<QuicUdpBinding> ();
  udpBinding->m_budpSocket = nullptr;
  udpBinding->m_budpSocket6 = nullptr;
  udpBinding->m_quicSocket = socket;
  m_quicUdpBindingList.insert (m_quicUdpBindingList.end (), udpBinding);
  //std::cout<<"My UDPBindListSize: "<<(int)m_quicUdpBindingList.size()<<std::endl;

  return socket;
}

Ptr<Socket>
QuicL4Protocol::CreateUdpSocket ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_node != 0);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> udpSocket = Socket::CreateSocket (m_node, tid);

  return udpSocket;
}

Ptr<Socket>
QuicL4Protocol::CreateUdpSocket6 ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_node != 0);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> udpSocket6 = Socket::CreateSocket (m_node, tid);

  return udpSocket6;
}

void
QuicL4Protocol::ReceiveIcmp (Ipv4Address icmpSource, uint8_t icmpTtl,
                             uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                             Ipv4Address payloadSource,Ipv4Address payloadDestination,
                             const uint8_t payload[8])
{
  NS_LOG_FUNCTION (this << icmpSource << (uint16_t) icmpTtl << (uint16_t) icmpType << (uint16_t) icmpCode << icmpInfo
                        << payloadSource << payloadDestination);
}

void
QuicL4Protocol::ReceiveIcmp (Ipv6Address icmpSource, uint8_t icmpTtl,
                             uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                             Ipv6Address payloadSource,Ipv6Address payloadDestination,
                             const uint8_t payload[8])
{
  NS_LOG_FUNCTION (this << icmpSource << (uint16_t) icmpTtl << (uint16_t) icmpType << (uint16_t) icmpCode << icmpInfo
                        << payloadSource << payloadDestination);

}

enum IpL4Protocol::RxStatus
QuicL4Protocol::Receive (Ptr<Packet> packet,
                         Ipv4Header const &incomingIpHeader,
                         Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << incomingIpHeader << incomingInterface);
  NS_FATAL_ERROR ("This call should not be used: QUIC packets need to go through a UDP socket");
  return IpL4Protocol::RX_OK;
}

enum IpL4Protocol::RxStatus
QuicL4Protocol::Receive (Ptr<Packet> packet,
                         Ipv6Header const &incomingIpHeader,
                         Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << packet << incomingIpHeader.GetSource () <<
                   incomingIpHeader.GetDestination ());
  NS_FATAL_ERROR ("This call should not be used: QUIC packets need to go through a UDP socket");
  return IpL4Protocol::RX_OK;
}

void
QuicL4Protocol::SendPacket (Ptr<QuicSocketBase> socket, Ptr<Packet> pkt, const QuicHeader &outgoing) const
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC (this
                << " sending seq " << outgoing.GetPacketNumber ()
                << " data size " << pkt->GetSize ());

  NS_LOG_INFO ("Sending Packet Through UDP Socket");

  // Given the presence of multiple subheaders in pkt,
  // we create a new packet, add the new QUIC header and
  // then add pkt as payload
  Ptr<Packet> packetSent = Create<Packet> ();
  packetSent->AddHeader (outgoing);
  packetSent->AddAtEnd (pkt);
  // NS_LOG_INFO ("" );
  //packetSent->Print (std::clog);
  // NS_LOG_INFO ("");

  //std::cout<<"Socket "<<socket<<" Send Packet Size: "<<packetSent->GetSize()<<std::endl;
  bool sendSuccess = false;
  QuicUdpBindingList::const_iterator it;
  for (it = m_quicUdpBindingList.begin (); it != m_quicUdpBindingList.end (); ++it)
    {
      Ptr<QuicUdpBinding> item = *it;
      if (item->m_quicSocket == socket)
        {
          int error = UdpSend (item->m_budpSocket, packetSent, 0);

          if(error == -1)
          {
            std::cout<<"udp send Error! Packet Size: "<<(int)packetSent->GetSize ()<<std::endl;
            NS_ASSERT_MSG(0==1,"udp send Error!");
          }
          else{
            if(!m_isServer)
            {
              #if 1
              std::string filePath = "/home/liyisen/tarballs/SAG_Platform/data/test_data/logs_ns3/QuicL4Send_"+std::to_string(m_node->GetId())+"_"
              + std::to_string(socket->GetEndPoint()->GetLocalPort())  + ".txt";
              std::ofstream file(filePath, std::ios::app);
              if(file.is_open()){
                  file << (int)packetSent->GetSize() << ","<<Simulator::Now().GetMilliSeconds() << std::endl;
                  file.close();
              }
              #endif

            }
          }
          sendSuccess = true;
          break;
        }
    }
  if(!sendSuccess)
  {
    std::cout<<"Send Fail for socket "<<socket<<std::endl;
  }
  
}


bool
QuicL4Protocol::RemoveSocket (Ptr<QuicSocketBase> socket)
{
  NS_LOG_FUNCTION (this);

  QuicUdpBindingList::iterator iter;
  bool found = false;
  bool closedListener = false;

  for (iter = m_quicUdpBindingList.begin (); iter != m_quicUdpBindingList.end (); ++iter)
    {
      Ptr<QuicUdpBinding> item = *iter;
      if (item->m_quicSocket == socket)
        {
          found = true;
          if (item->m_listenerBinding)
            {
              closedListener = true;
            }
          m_quicUdpBindingList.erase (iter);

          break;
        }
    }

  //if closing the listener, close all the clone ones
  if (closedListener)
    {
      NS_LOG_LOGIC (this << " Closing all the cloned sockets");
      iter = m_quicUdpBindingList.begin ();
      while (iter != m_quicUdpBindingList.end ())
        {
          (*iter)->m_quicSocket->Close ();
          ++iter;
        }
    }

  return found;
}

Ipv4EndPoint *
QuicL4Protocol::Allocate (void)
{
  NS_LOG_FUNCTION (this);
  return m_endPoints->Allocate ();
}

Ipv4EndPoint *
QuicL4Protocol::Allocate (Ipv4Address address)
{
  NS_LOG_FUNCTION (this << address);
  return m_endPoints->Allocate (address);
}

Ipv4EndPoint *
QuicL4Protocol::Allocate (Ptr<NetDevice> boundNetDevice, uint16_t port)
{
  NS_LOG_FUNCTION (this << boundNetDevice << port);
  return m_endPoints->Allocate (boundNetDevice, port);
}

Ipv4EndPoint *
QuicL4Protocol::Allocate (Ptr<NetDevice> boundNetDevice, Ipv4Address address, uint16_t port)
{
  NS_LOG_FUNCTION (this << boundNetDevice << address << port);
  return m_endPoints->Allocate (boundNetDevice, address, port);
}

Ipv4EndPoint *
QuicL4Protocol::Allocate (Ptr<NetDevice> boundNetDevice,
                          Ipv4Address localAddress, uint16_t localPort,
                          Ipv4Address peerAddress, uint16_t peerPort)
{
  NS_LOG_FUNCTION (this << boundNetDevice << localAddress << localPort << peerAddress << peerPort);
  return m_endPoints->Allocate (boundNetDevice,
                                localAddress, localPort,
                                peerAddress, peerPort);
}

Ipv6EndPoint *
QuicL4Protocol::Allocate6 (void)
{
  NS_LOG_FUNCTION (this);
  return m_endPoints6->Allocate ();
}

Ipv6EndPoint *
QuicL4Protocol::Allocate6 (Ipv6Address address)
{
  NS_LOG_FUNCTION (this << address);
  return m_endPoints6->Allocate (address);
}

Ipv6EndPoint *
QuicL4Protocol::Allocate6 (Ptr<NetDevice> boundNetDevice, uint16_t port)
{
  NS_LOG_FUNCTION (this << boundNetDevice << port);
  return m_endPoints6->Allocate (boundNetDevice, port);
}

Ipv6EndPoint *
QuicL4Protocol::Allocate6 (Ptr<NetDevice> boundNetDevice, Ipv6Address address, uint16_t port)
{
  NS_LOG_FUNCTION (this << boundNetDevice << address << port);
  return m_endPoints6->Allocate (boundNetDevice, address, port);
}

Ipv6EndPoint *
QuicL4Protocol::Allocate6 (Ptr<NetDevice> boundNetDevice,
                           Ipv6Address localAddress, uint16_t localPort,
                           Ipv6Address peerAddress, uint16_t peerPort)
{
  NS_LOG_FUNCTION (this << boundNetDevice << localAddress << localPort << peerAddress << peerPort);
  return m_endPoints6->Allocate (boundNetDevice,
                                 localAddress, localPort,
                                 peerAddress, peerPort);
}

void
QuicL4Protocol::DeAllocate (Ipv4EndPoint *endPoint)
{
  NS_LOG_FUNCTION (this << endPoint);
  m_endPoints->DeAllocate (endPoint);
}

void
QuicL4Protocol::DeAllocate (Ipv6EndPoint *endPoint)
{
  NS_LOG_FUNCTION (this << endPoint);
  m_endPoints6->DeAllocate (endPoint);
}

void
QuicL4Protocol::SetDownTarget (IpL4Protocol::DownTargetCallback callback)
{
  NS_LOG_FUNCTION (this);
  m_downTarget = callback;
}

IpL4Protocol::DownTargetCallback
QuicL4Protocol::GetDownTarget (void) const
{
  NS_LOG_FUNCTION (this);
  return m_downTarget;
}

void
QuicL4Protocol::SetDownTarget6 (IpL4Protocol::DownTargetCallback6 callback)
{
  NS_LOG_FUNCTION (this);
  m_downTarget6 = callback;
}

IpL4Protocol::DownTargetCallback6
QuicL4Protocol::GetDownTarget6 (void) const
{
  return m_downTarget6;
}

bool
QuicL4Protocol::Is0RTTHandshakeAllowed () const
{
  return m_0RTTHandshakeStart;
}

} // namespace ns3


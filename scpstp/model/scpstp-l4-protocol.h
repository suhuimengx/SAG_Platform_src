#ifndef SCPSTP_L4_PROTOCOL_H
#define SCPSTP_L4_PROTOCOL_H

#include <stdint.h>

#include "ns3/tcp-l4-protocol.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/sequence-number.h"
#include "ns3/ip-l4-protocol.h"

namespace ns3 {

class Node;
class Socket;
class TcpHeader;
class Ipv4EndPointDemux;
class Ipv6EndPointDemux;
class Ipv4Interface;
class TcpSocketBase;
class ScpsTpSocketBase;
class Ipv4EndPoint;
class Ipv6EndPoint;
class NetDevice;


/**
 * \ingroup scpstp
 * \defgroup scpstp SCPSTP
 *
 * This is an implementation of the Space Communications Protocol Standards - Transport Protocol (SCPS-TP).
 *
 * SCPS-TP is designed for challenging communication environments such as space networks.
 * It extends traditional transport protocols with mechanisms to handle congestion,
 * corruption, and link outages effectively.
 *
 * See CCSDS and related documents.
 */


/**
 * \ingroup scpstp
 * \brief SCPS-TP socket creation and multiplexing/demultiplexing
 * 
 * A single instance of this class is held by one instance of class Node.
 *
 * The creation of ScpsTpSocket is handled in the method CreateSocket, which is
 * called by ScpsTpSocketFactory. Upon creation, this class is responsible for
 * the socket initialization and handles multiplexing/demultiplexing of data
 * between node's SCPS-TP sockets. Demultiplexing is done by receiving
 * packets from IP and forwarding them to the correct socket. Multiplexing
 * is done through the SendPacket function, which sends the packet down the stack.
 *
 * Moreover, this class allocates "endpoint" objects (ns3::Ipv4EndPoint) for SCPS-TP.
 * Unlike standard TCP, SCPS-TP is designed for challenging network environments,
 * incorporating mechanisms to handle congestion, corruption, and link outages.
 * 
 * \see CreateSocket
 * \see NotifyNewAggregate
 * \see SendPacket
 */
class ScpsTpL4Protocol : public IpL4Protocol {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  static const uint8_t PROT_NUMBER; //!< protocol number (0x6)

  ScpsTpL4Protocol ();
  virtual ~ScpsTpL4Protocol ();

  /**
   * Set node associated with this stack
   * \param node the node
   */
  void SetNode (Ptr<Node> node);

  // NOTE: API from here should not be removed, only added. Be backward-compatible!

  /**
   * \brief Create a SCPS-TP socket using the TypeId set by SocketType attribute
   *
   * \return A smart Socket pointer to a TcpSocket allocated by this instance
   * of the SCPS-TP protocol
   */
  Ptr<Socket> CreateSocket (void);

  /**
   * \brief Create a SCPS-TP socket using the specified congestion control algorithm TypeId
   *
   * \return A smart Socket pointer to a TcpSocket allocated by this instance
   * of the SCPS-TP protocol
   *
   * \warning using a congestionTypeId other than SCPS-TP is a bad idea.
   *
   * \param congestionTypeId the congestion control algorithm TypeId
   * \param recoveryTypeId the recovery algorithm TypeId
   */
  Ptr<Socket> CreateSocket (TypeId congestionTypeId, TypeId recoveryTypeId);

  /**
    * \brief Create a SCPS-TP socket using the specified congestion control algorithm
    * \return A smart Socket pointer to a TcpSocket allocated by this instance
    * of the SCPS-TP protocol
    *
    * \param congestionTypeId the congestion control algorithm TypeId
    *
    */
  Ptr<Socket> CreateSocket (TypeId congestionTypeId);

  /**
   * \brief Allocate an IPv4 Endpoint
   * \return the Endpoint
   */
  Ipv4EndPoint *Allocate (void);
  /**
   * \brief Allocate an IPv4 Endpoint
   * \param address address to use
   * \return the Endpoint
   */
  Ipv4EndPoint *Allocate (Ipv4Address address);
  /**
   * \brief Allocate an IPv4 Endpoint
   * \param boundNetDevice Bound NetDevice (if any)
   * \param port port to use
   * \return the Endpoint
   */
  Ipv4EndPoint *Allocate (Ptr<NetDevice> boundNetDevice, uint16_t port);
  /**
   * \brief Allocate an IPv4 Endpoint
   * \param boundNetDevice Bound NetDevice (if any)
   * \param address address to use
   * \param port port to use
   * \return the Endpoint
   */
  Ipv4EndPoint *Allocate (Ptr<NetDevice> boundNetDevice, Ipv4Address address, uint16_t port);
  /**
   * \brief Allocate an IPv4 Endpoint
   * \param boundNetDevice Bound NetDevice (if any)
   * \param localAddress local address to use
   * \param localPort local port to use
   * \param peerAddress remote address to use
   * \param peerPort remote port to use
   * \return the Endpoint
   */
  Ipv4EndPoint *Allocate (Ptr<NetDevice> boundNetDevice,
                          Ipv4Address localAddress, uint16_t localPort,
                          Ipv4Address peerAddress, uint16_t peerPort);
  /**
   * \brief Allocate an IPv6 Endpoint
   * \return the Endpoint
   */
  Ipv6EndPoint *Allocate6 (void);
  /**
   * \brief Allocate an IPv6 Endpoint
   * \param address address to use
   * \return the Endpoint
   */
  Ipv6EndPoint *Allocate6 (Ipv6Address address);
  /**
   * \brief Allocate an IPv6 Endpoint
   * \param boundNetDevice Bound NetDevice (if any)
   * \param port port to use
   * \return the Endpoint
   */
  Ipv6EndPoint *Allocate6 (Ptr<NetDevice> boundNetDevice, uint16_t port);
  /**
   * \brief Allocate an IPv6 Endpoint
   * \param boundNetDevice Bound NetDevice (if any)
   * \param address address to use
   * \param port port to use
   * \return the Endpoint
   */
  Ipv6EndPoint *Allocate6 (Ptr<NetDevice> boundNetDevice, Ipv6Address address, uint16_t port);
  /**
   * \brief Allocate an IPv6 Endpoint
   * \param boundNetDevice Bound NetDevice (if any)
   * \param localAddress local address to use
   * \param localPort local port to use
   * \param peerAddress remote address to use
   * \param peerPort remote port to use
   * \return the Endpoint
   */
  Ipv6EndPoint *Allocate6 (Ptr<NetDevice> boundNetDevice,
                           Ipv6Address localAddress, uint16_t localPort,
                           Ipv6Address peerAddress, uint16_t peerPort);

  /**
   * \brief Send a packet via SCPS-TP (IP-agnostic)
   *
   * \param pkt The packet to send
   * \param outgoing The packet header
   * \param saddr The source Ipv4Address
   * \param daddr The destination Ipv4Address
   * \param oif The output interface bound. Defaults to null (unspecified).
   */
  void SendPacket (Ptr<Packet> pkt, const TcpHeader &outgoing,
                   const Address &saddr, const Address &daddr,
                   Ptr<NetDevice> oif = 0) const;

  /**
   * \brief Make a socket fully operational
   *
   * Called after a socket has been bound, it is inserted in an internal vector.
   *
   * \param socket Socket to be added
   */
  void AddSocket (Ptr<ScpsTpSocketBase> socket);

  /**
   * \brief Remove a socket from the internal list
   *
   * \param socket socket to Remove
   * \return true if the socket has been removed
   */
  bool RemoveSocket (Ptr<ScpsTpSocketBase> socket);

  /**
   * \brief Remove an IPv4 Endpoint.
   * \param endPoint the end point to remove
   */
  void DeAllocate (Ipv4EndPoint *endPoint);
  /**
   * \brief Remove an IPv6 Endpoint.
   * \param endPoint the end point to remove
   */
  void DeAllocate (Ipv6EndPoint *endPoint);

  // From IpL4Protocol
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> p,
                                               Ipv4Header const &incomingIpHeader,
                                               Ptr<Ipv4Interface> incomingInterface);
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> p,
                                               Ipv6Header const &incomingIpHeader,
                                               Ptr<Ipv6Interface> incomingInterface);

  virtual void ReceiveIcmp (Ipv4Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                            Ipv4Address payloadSource,Ipv4Address payloadDestination,
                            const uint8_t payload[8]);
  virtual void ReceiveIcmp (Ipv6Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                            Ipv6Address payloadSource,Ipv6Address payloadDestination,
                            const uint8_t payload[8]);

  virtual void SetDownTarget (IpL4Protocol::DownTargetCallback cb);
  virtual void SetDownTarget6 (IpL4Protocol::DownTargetCallback6 cb);
  virtual int GetProtocolNumber (void) const;
  virtual IpL4Protocol::DownTargetCallback GetDownTarget (void) const;
  virtual IpL4Protocol::DownTargetCallback6 GetDownTarget6 (void) const;

protected:
  virtual void DoDispose (void);

  /**
   * \brief Setup socket factory and callbacks when aggregated to a node
   *
   * This function will notify other components connected to the node that a
   * new stack member is now connected. This will be used to notify Layer 3
   * protocol of layer 4 protocol stack to connect them together.
   * The aggregation is completed by setting the node in the SCPS-TP stack, link
   * it to the ipv4 or ipv6 stack and adding SCPS-TP socket factory to the node.
   */
  virtual void NotifyNewAggregate ();

  /**
   * \brief Get the scps-tp header of the incoming packet and checks its checksum if needed
   *
   * \param packet Received packet
   * \param incomingTcpHeader Overwritten with the scps-tp header of the packet
   * \param source Source address (an underlying Ipv4Address or Ipv6Address)
   * \param destination Destination address (an underlying Ipv4Address or Ipv6Address)
   *
   * \return RX_CSUM_FAILED if the checksum check fails, RX_OK otherwise
   */
  enum IpL4Protocol::RxStatus
  PacketReceived (Ptr<Packet> packet, TcpHeader &incomingTcpHeader,
                  const Address &source, const Address &destination);

  /**
   * \brief Check if RST packet should be sent, and in case, send it
   *
   * The function is called when no endpoint is found for the received
   * packet. So ScpsTpL4Protocol do not know to who the packet should be
   * given to. An RST packet is sent out as reply unless the received packet
   * has the RST flag set.
   *
   * \param incomingHeader TCP header of the incoming packet
   * \param incomingSAddr Source address of the incoming packet
   * \param incomingDAddr Destination address of the incoming packet
   *
   */
  void NoEndPointsFound (const TcpHeader &incomingHeader, const Address &incomingSAddr,
                         const Address &incomingDAddr);

private:
  Ptr<Node> m_node;                //!< the node this stack is associated with
  Ipv4EndPointDemux *m_endPoints;  //!< A list of IPv4 end points.
  Ipv6EndPointDemux *m_endPoints6; //!< A list of IPv6 end points.
  TypeId m_rttTypeId;              //!< The RTT Estimator TypeId
  TypeId m_congestionTypeId;       //!< The socket TypeId
  TypeId m_recoveryTypeId;         //!< The recovery TypeId
  std::vector<Ptr<ScpsTpSocketBase> > m_sockets;      //!< list of sockets
  IpL4Protocol::DownTargetCallback m_downTarget;   //!< Callback to send packets over IPv4
  IpL4Protocol::DownTargetCallback6 m_downTarget6; //!< Callback to send packets over IPv6
  /**
   * \brief Copy constructor
   *
   * Defined and not implemented to avoid misuse
   */
  ScpsTpL4Protocol (const ScpsTpL4Protocol &);
  /**
   * \brief Copy constructor
   *
   * Defined and not implemented to avoid misuse
   * \returns
   */
  ScpsTpL4Protocol &operator = (const ScpsTpL4Protocol &);

  /**
   * \brief Send a packet via SCPS-TP (IPv4)
   *
   * \param pkt The packet to send
   * \param outgoing The packet header
   * \param saddr The source Ipv4Address
   * \param daddr The destination Ipv4Address
   * \param oif The output interface bound. Defaults to null (unspecified).
   */
  void SendPacketV4 (Ptr<Packet> pkt, const TcpHeader &outgoing,
                     const Ipv4Address &saddr, const Ipv4Address &daddr,
                     Ptr<NetDevice> oif = 0) const;

  /**
   * \brief Send a packet via SCPS-TP (IPv6)
   *
   * \param pkt The packet to send
   * \param outgoing The packet header
   * \param saddr The source Ipv4Address
   * \param daddr The destination Ipv4Address
   * \param oif The output interface bound. Defaults to null (unspecified).
   */
  void SendPacketV6 (Ptr<Packet> pkt, const TcpHeader &outgoing,
                     const Ipv6Address &saddr, const Ipv6Address &daddr,
                     Ptr<NetDevice> oif = 0) const;
};

} // namespace ns3

#endif // SCPSTP_L4_PROTOCOL_H

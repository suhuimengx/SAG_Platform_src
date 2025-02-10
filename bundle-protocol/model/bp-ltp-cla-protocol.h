

#ifndef BP_LTP_CLA_PROTOCOL_H
#define BP_LTP_CLA_PROTOCOL_H

#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "bp-cla-protocol.h"
#include "bp-endpoint-id.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "bundle-protocol.h"
#include "ns3/ltp-protocol.h"
#include "bp-routing-protocol.h"
#include <map>

namespace ns3 {
namespace ltp {

class Node;

class BpLtpClaProtocol : public BpClaProtocol
{
public:

  static TypeId GetTypeId (void);

  /**
   * \brief Constructor
   */
  BpLtpClaProtocol ();

  /**
   * Destroy
   */
  virtual ~BpLtpClaProtocol ();

  /**
   * send packet to the transport layer
   *
   * \param packet packet to sent
   */
  virtual int SendPacket (Ptr<Packet> packet);

  /**
   * Set the LTP socket in listen state;
   *
   * \param local the local endpoint id
   */
  virtual int EnableReceive (const BpEndpointId &local);

  /**
   * Close the LTP connection
   *
   * \param local the endpoint id of registration
   */
  virtual int DisableReceive (const BpEndpointId &local);

  /**
   * Enable this bundle node to send bundles at the transport layer
   * This method calls the bind (), connect primitives of LTP socket
   *
   * \param src the source endpoint id
   * \param dst the destination endpoint id
   */
  virtual int EnableSend (const BpEndpointId &src, const BpEndpointId &dst);

  /**
   * \brief Get the transport layer socket
   *
   * This method finds the socket by the source endpoint id of the bundle. 
   * If it cannot find, which means that this bundle is the first bundle required
   * to be transmitted for this source endpoint id, it start a LTP connection with
   * the destination endpoint id.
   *
   * \param packet the bundle required to be transmitted
   *
   * \return return NULL if it cannot find a socket for the source endpoing id of 
   * the bundle. Otherwise, it returns the socket.
   */
  virtual Ptr<Socket> GetL4Socket (Ptr<Packet> packet);

  /**
   * Connect to routing protocol
   *
   * \param route routing protocol
   */
  void SetRoutingProtocol (Ptr<BpRoutingProtocol> route);

  /**
   * Get routing protocol
   *
   * \return routing protocol
   */
  virtual Ptr<BpRoutingProtocol> GetRoutingProtocol ();

  /**
   * Connect to bundle protocol
   *
   * \param bundleProtocol bundle protocol
   */
  void SetBundleProtocol (Ptr<BundleProtocol> bundleProtocol);

  void SetLtpProtocol (Ptr<LtpProtocol> ltpProtocol);

  /**
   *  Callbacks methods called by NotifyXXX methods in LtpSocketBase
   *
   *  Those methods will further call the callback wrap methods in BpSocket
   *  The only exception is the DataRecv (), which will call BpSocket::Receive ()
   *
   *  The operations within those methods will be updated in the future versions
   */

  /**
   * \brief connection succeed callback
   */
  void ConnectionSucceeded (Ptr<Socket> socket );

  /**
   * \brief connection fail callback
   */
  void ConnectionFailed (Ptr<Socket> socket);

  /**
   * \brief normal close callback
   */
  void NormalClose (Ptr<Socket> socket);

  /**
   * \brief error close callback
   */
  void ErrorClose (Ptr<Socket> socket);

  /**
   * \brief connection request callback
   */
  bool ConnectionRequest (Ptr<Socket>, const Address &);

  /**
   * \brief new connection created callback
   */
  void NewConnectionCreated (Ptr<Socket>, const Address &);

  /**
   * \brief data sent callback
   */
  void DataSent (Ptr<Socket>, uint32_t size);

  /**
   * \brief sent callback
   */
  void Sent (Ptr<Socket>, uint32_t size);

  /**
   * \brief data receive callback
   */
  void DataRecv (Ptr<Socket> socket);

  void PacketRecv (Ptr<Packet> packet);

private:

  /**
   * Set callbacks of the transport layer
   *
   * \param socket the transport layer socket
   */
  virtual void SetL4SocketCallbacks (Ptr<Socket> socket);

private:
  Ptr<BundleProtocol> m_bp;                             /// bundle protocol
  Ptr<LtpProtocol> m_ltp;                               /// ltp protocol
  std::map<BpEndpointId, Ptr<Socket> > m_l4SendSockets; /// the transport layer sender sockets
  std::map<BpEndpointId, Ptr<Socket> > m_l4RecvSockets; /// the transport layer receiver sockets

  Ptr<BpRoutingProtocol> m_bpRouting;                   /// bundle routing protocol
};

} // namespace ns3
}

#endif /* BP_LTP_CLA_PROTOCOL_H */

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
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
 * Author: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */

#ifndef SCPSTPSOCKETBASE_H
#define SCPSTPSOCKETBASE_H

#include "ns3/tcp-socket-base.h"
#include "ns3/scpstp-l4-protocol.h"
#include <stdint.h>
#include <queue>
#include "ns3/traced-value.h"
#include "ns3/tcp-socket.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/timer.h"
#include "ns3/sequence-number.h"
#include "ns3/data-rate.h"
#include "ns3/node.h"
#include "ns3/tcp-socket-state.h"

namespace ns3 {

class Ipv4EndPoint;
class Ipv6EndPoint;
class Node;
class Packet;
class TcpL4Protocol;
class TcpHeader;
class TcpCongestionOps;
class TcpRecoveryOps;
class RttEstimator;
class TcpRxBuffer;
class TcpTxBuffer;
class TcpOption;
class Ipv4Interface;
class Ipv6Interface;
class TcpRateOps;



/**
 * \ingroup socket
 * \ingroup scpstp
 *
 * \brief A base class for implementation of a stream socket using SCPS-TP.
 *
 * This class extends the TCP socket functionality and introduces SCPS-TP-specific 
 * modifications, including a fixed ACK frequency mechanism, SNACK mechanism, 
 * and SCPS-TP error recovery mechanism.
 *
 * The class provides connection orientation and sliding window flow control. 
 * Congestion control and other protocol-specific mechanisms are managed within 
 * the SCPS-TP framework.
 *
 * The socket maintains two state machines: the SCPS-TP state machine and 
 * the congestion control state machine. These ensure efficient data transmission 
 * and loss recovery under various network conditions.
 *
 * SCPS-TP incorporates mechanisms such as selective negative acknowledgment 
 * (SNACK) for improved error recovery and an optimized ACK frequency adjustment 
 * to enhance performance over constrained links.
 *
 * Additional enhancements and protocol-specific optimizations will be documented 
 * in detail.
 */
class ScpsTpSocketBase : public TcpSocketBase
{
public:
  enum LossType
  {
    Corruption,  //!< Packet corruption
    Congestion,  //!< Packet loss due to congestion
    Link_Outage //!< Connection interruption
  };

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  
  /**
   * \brief Get the instance TypeId
   * \return the instance TypeId
   */
  virtual TypeId GetInstanceTypeId () const;

  /**
   *  Create an unbound scpstp socket
   */
  ScpsTpSocketBase(void);

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  ScpsTpSocketBase (const ScpsTpSocketBase& sock);

  virtual ~ScpsTpSocketBase(void);

  /**
   * \brief Get the reason for data loss
   * \return the object lossType 
   */
  virtual LossType GetLossType(void) const;
  
  /**
   * \brief Set the reason for data loss
   * \param losstype the reason for data loss
   */
  void SetLossType(LossType losstype);

  /**
   * \brief Set the associated ScpsTp L4 protocol.
   * \param scpstp the scpsp L4 protocol
   */
  virtual void SetScpsTp (Ptr<ScpsTpL4Protocol> scpstp);

  virtual int Connect (const Address &address);      // Setup endpoint and call ProcessAction() to connect
  virtual int Bind (void);    // Bind a socket by setting up endpoint in ScpsTpL4Protocol
  virtual int Bind6 (void);    // Bind a socket by setting up endpoint in ScpsTpL4Protocol
  virtual int Bind (const Address &address);         // ... endpoint of specific addr or port
  virtual int Send (Ptr<Packet> p, uint32_t flags);  // Call by app to send data to network

protected:
  /**
   * \brief Called by ScpsTpSocketBase::ForwardUp{,6}().
   *
   * Get a packet from L3. This is the real function to handle the
   * incoming packet from lower layers. This is
   * wrapped by ForwardUp() so that this function can be overloaded by daughter
   * classes.
   *
   * \param packet the incoming packet
   * \param fromAddress the address of the sender of packet
   * \param toAddress the address of the receiver of packet (hopefully, us)
   */
  virtual void DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
                            const Address &toAddress);

 /**
   * \brief Kill this socket by zeroing its attributes (IPv4)
   *
   * This is a callback function configured to m_endpoint in
   * SetupCallback(), invoked when the endpoint is destroyed.
   */
  virtual void Destroy (void);

  /**
   * \brief Common part of the two Bind(), i.e. set callback and remembering local addr:port
   *
   * \returns 0 on success, -1 on failure
   */
  virtual int SetupCallback (void);

  /**
   * \brief Kill this socket by zeroing its attributes (IPv6)
   *
   * This is a callback function configured to m_endpoint in
   * SetupCallback(), invoked when the endpoint is destroyed.
   */
  virtual void Destroy6 (void);

  /**
   * \brief Send a empty packet that carries a flag, e.g., ACK
   *
   * \param flags the packet's flags
   */
  virtual void SendEmptyPacket (uint8_t flags);

  /**
   * \brief Deallocate m_endPoint and m_endPoint6
   */
  virtual void DeallocateEndPoint (void);

  /**
   * \brief Complete a connection by forking the socket
   *
   * This function is called only if a SYN received in LISTEN state. After
   * ScpsTpSocketBase cloned, allocate a new end point to handle the incoming
   * connection and send a SYN+ACK to complete the handshake.
   *
   * \param p the packet triggering the fork
   * \param tcpHeader the TCP header of the triggering packet
   * \param fromAddress the address of the remote host
   * \param toAddress the address the connection is directed to
   */
  virtual void CompleteFork (Ptr<Packet> p, const TcpHeader& tcpHeader,
                             const Address& fromAddress, const Address& toAddress);

  /**
   * \brief Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
   *        TCP header, and send to ScpsTpL4Protocol
   *
   * \param seq the sequence number
   * \param maxSize the maximum data block to be transmitted (in bytes)
   * \param withAck forces an ACK to be sent
   * \returns the number of bytes sent
   */
  virtual uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck);

  /**
   * \brief Send 1 byte probe to get an updated window size
   */
  virtual void PersistTimeout (void);

  /**
   * \brief Received a packet upon LISTEN state.
   *
   * \param packet the packet
   * \param tcpHeader the packet's TCP header
   * \param fromAddress the source address
   * \param toAddress the destination address
   */
  virtual void ProcessListen (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                      const Address& fromAddress, const Address& toAddress);
  /**
   * \brief Received a packet upon SYN_SENT
   *
   * \param packet the packet
   * \param tcpHeader the packet's TCP header
   */
  virtual void ProcessSynSent (Ptr<Packet> packet, const TcpHeader& tcpHeader);

  /**
   * \brief FIN is in sequence, notify app and respond with a FIN
   */
  virtual void DoPeerClose (void);

  /**
   * \brief Timeout at LAST_ACK, close the connection
   */
  virtual void LastAckTimeout (void);

  /**
   * \brief Move from CLOSING or FIN_WAIT_2 to TIME_WAIT state
   */
  virtual void TimeWait (void);

  /**
   * \brief Call CopyObject<> to clone me
   * \returns a copy of the socket
   */
  virtual Ptr<ScpsTpSocketBase> ForkScpsTp (void);

  /**
   * \brief Enter CA_CWR state upon receipt of an ECN Echo
   *
   * \param currentDelivered Currently (S)ACKed bytes
   */
  virtual void EnterCwr (uint32_t currentDelivered);

  /**
   * \brief Received an ACK packet
   * \param packet the packet
   * \param tcpHeader the packet's TCP header
   */
  virtual void ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader);
  
  /**
   * \brief Update buffers w.r.t. ACK
   * \param seq the sequence number
   * \param resetRTO indicates if RTO should be reset
   */
  virtual void NewAck (SequenceNumber32 const& seq, bool resetRTO);

  /**
   * \brief Recv of a data, put into buffer, call L7 to get it if necessary
   * \param packet the packet
   * \param tcpHeader the packet's TCP header
   */
  virtual void ReceivedData (Ptr<Packet> packet, const TcpHeader& tcpHeader);

  /**
   * \brief An RTO event happened
   */
  virtual void ReTxTimeout (void);

  /**
   * \brief Enter the CA_RECOVERY, and retransmit the head
   *
   * \param currentDelivered Currently (S)ACKed bytes
   */
  virtual void EnterRecovery (uint32_t currentDelivered);

 /**
   * \brief Process a received ack
   * \param ackNumber ack number
   * \param scoreboardUpdated if true indicates that the scoreboard has been
   * \param oldHeadSequence value of HeadSequence before ack
   * updated with SACK information
   * \param currentDelivered The number of bytes (S)ACKed
   */
  virtual void ProcessAck (const SequenceNumber32 &ackNumber, bool scoreboardUpdated,
                           uint32_t currentDelivered, const SequenceNumber32 &oldHeadSequence);

  /**
   * \brief Send 1 byte probe to get an updated link state
   */
  virtual void LinkOutPersistTimeout (void);

  /**
   * \brief Return the max possible number of unacked bytes
   * if m_lossType is LinkOut, return 0
   * \returns the max possible number of unacked bytes
   */
  virtual uint32_t Window (void) const;
  
  /**
   * \brief Received a packet upon ESTABLISHED state.
   *
   * This function is mimicking the role of tcp_rcv_established() in tcp_input.c in Linux kernel.
   *
   * \param packet the packet
   * \param tcpHeader the packet's TCP header
   */
  virtual void ProcessEstablished (Ptr<Packet> packet, const TcpHeader& tcpHeader); // Received a packet upon ESTABLISHED state

  /**
   * \brief Dupack management
   *
   * \param currentDelivered Current (S)ACKed bytes
   */
  virtual void DupAck (uint32_t currentDelivered);

  /**
   * \brief The amount of Rx window announced to the peer
   * \param scale indicate if the window should be scaled. True for
   * almost all cases, except when we are sending a SYN
   * \returns size of Rx window announced to the peer
   */
  virtual uint16_t AdvertisedWindowSize (bool scale = true) const;

  /**
   * \brief Add the SNACK option to the header
   *
   * \param header TcpHeader where the method should add the option
   */
  void AddOptionSnack (TcpHeader& header, uint16_t hole1Offset, uint16_t hole1Size);

  /**
   * \brief Read TCP options before Ack processing
   *
   * Timestamp and Window scale are managed in other pieces of code.
   *
   * \param tcpHeader Header of the segment
   * \param [out] bytesSacked Number of bytes SACKed, or 0
   */
  virtual void ReadOptions (const TcpHeader &tcpHeader, uint32_t *bytesSacked);

  /**
   * \brief Read the SNACK option ,update the snack list and then pass it to the TxBuffer
   *
   * \param option SNACK option from the header
   * \param snackList_temp the temporary snack list
   * \param ackNumber the ack number
   * 
   */
  void ProcessOptionSnack(const Ptr<const TcpOption> option, ScpsTpOptionSnack::SnackList& snackList, uint32_t ackNumber);

  virtual void SetLinkOutPersistTimeout (Time timeout);
  virtual Time GetLinkOutPersistTimeout (void) const;

  /**
   * \brief Cancel all timer when endpoint is deleted
   */
  virtual void CancelAllTimers (void);

  /**
   * \brief Checks whether the given TCP segment is valid or not.
   *
   * \param seq the sequence number of packet's TCP header
   * \param tcpHeaderSize the size of packet's TCP header
   * \param tcpPayloadSize the size of TCP payload
   * \return true if the TCP segment is valid
   */
  virtual bool IsValidTcpSegment (const SequenceNumber32 seq, const uint32_t tcpHeaderSize,
                          const uint32_t tcpPayloadSize);

    /**
   * \brief Send as much pending data as possible in limit according to the Tx window.
   *
   * Note that this function did not implement the PSH flag.
   *
   * \param withAck forces an ACK to be sent
   * \returns the number of packets sent
   */
  uint32_t SendPendingDataInLimit (bool withAck = false);


  /**
   * \brief Take into account the packet for RTT estimation
   * \param tcpHeader the packet's TCP header
   */
  virtual void EstimateRtt (const TcpHeader& tcpHeader);
                          
private:

protected:
  TracedValue<LossType> m_lossType;                     //!< the reason for data loss
  Ptr<ScpsTpL4Protocol>  m_scpstp;                 //!< the associated ScpsTp L4 protocol  
  bool m_isCorruptionRecovery;                     //!< true if the recovery is due to corruption
  EventId m_linkOutPersistEvent;                   //!< Link outage persist event
  EventId m_linkCongPersistEvent;                   //!< Link congestion event
  Time         m_linkOutPersistTimeout   {Seconds (0.0)};   //!< Time between sending 1-byte probes for link outage
  Time         m_linkOutTimeFrom         {Seconds (0.0)};   //!< Time when the link outage started       
  uint32_t          m_dataRetrCountForLinkOut {0}; //!< Count of remaining data retransmission attempts to enter link outage state
  uint32_t          m_dataRetriesForLinkOut   {0}; //!< Number of data retransmission attempts for link outage state
  ScpsTpOptionSnack::SnackList m_snackList; //!< Snack list

};

} // namespace ns3





#endif //SCPSTPSOCKETBASE_H
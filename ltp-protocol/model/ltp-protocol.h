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

#ifndef LTP_PROTOCOL_H
#define LTP_PROTOCOL_H

#include "ltp-session-state-record.h"
#include "ns3/random-variable-stream.h"
#include "ltp-convergence-layer-adapter.h"
#include "ltp-ip-resolution-table.h"
#include "ns3/node.h"
#include "ltp-queue-base.h"

namespace ns3 {
namespace ltp {

class LtpUdpConvergenceLayerAdapter;

/*
 * \enum StatusNotificationCode
 * \brief Notices to client service
 * Defined in RFC-5326 Sections 7.1-7.7.
 */
enum StatusNotificationCode
{
  SESSION_START = 0,
  GP_SEGMENT_RCV = 1,
  RED_PART_RCV = 2,
  TX_COMPLETED = 3,
  TX_SESSION_CANCEL = 4,
  RX_SESSION_CANCEL = 5,
  SESSION_END = 6
};

/**
 * \ingroup dtn
 *
 * \brief Class representing active client service instances
 * registered within the LTP protocol.
 *
 * This class is used to keep track of active sessions
 * that are being used by each client service instance. It contains methods
 * to report the changes on session status that may happen during
 * a transmission session.
 */
class ClientServiceStatus : public Object
{
public:
  /* Constructor */
  ClientServiceStatus ();

  /* Destructor */
  ~ClientServiceStatus ();

  static TypeId GetTypeId (void);

  /*
   * \brief This function reports changes in session state to a client service instance.
   *
   * Different notification codes require different parameters,
   * as such the Session Id and the Code are mandatory.
   * the other parameters have been assigned default values. Those
   * parameters should only be provided if specifically needed by the type of notification.
   *
   * \param id Session Id.
   * \param code StatusNotificationCode
   * \param data Data to deliver to the client service instance. (used by RED_PART_RCV & GP_SEGMENT_RCV)
   * \param dataLength Length of the delivered data.  (used by RED_PART_RCV & GP_SEGMENT_RCV)
   * \param srcLtpEngine Source Ltp Engine id.  (used by RED_PART_RCV and GP_SEGMENT_RCV)
   * \param offset Offset within the block of the delivered data (used by GP_SEGMENT_RCV)
   *
   * */
  void ReportStatus (SessionId id,
                     StatusNotificationCode code,
                     std::vector<uint8_t> data,
                     uint32_t dataLength,
                     bool endFlag,
                     uint64_t srcLtpEngine,
                     uint32_t offset
                     );

  /*
   * \brief This function reports session cancelation to a client service instance.
   * \param id Session Id.
   * \param code StatusNotificationCode
   * \param cx Cancelation reason code
   **/
  void ReportCancelStatus (SessionId id,
                           StatusNotificationCode code,
                           CxReasonCode cx);

  /*
   * \brief Add an active session id to this client service instance.
   * \param id session id.
   */
  void AddSession (SessionId id);
  /*
   * \brief Remove all active sessions
   */
  void ClearSessions ();
  /*
   * \brief Get number of active sessions.
   * \return number of sessions.
   */
  uint32_t GetNSessions () const;
  /*
   * \brief Get session id for the given index.
   * \param index
   * \return Session id.
   */
  SessionId GetSession (uint32_t index);

private:
  std::vector<SessionId> m_activeSessions; //!< Client Service Instance Active Sessions

  TracedCallback<SessionId,
                 StatusNotificationCode,
                 std::vector<uint8_t>,
                 uint32_t,
                 bool,
                 uint64_t,
                 uint32_t >
  m_reportStatus; //!< Callback used to report events to the client service instances
};

/*
 * \ingroup dtn
 *
 * \brief Ltp protocol core class. Contains the protocol logic and the
 * sender and receiver state machines.
 *
 *  This class is a container for:
 *  - Active session state records,
 *  - Available convergence layer adapters.
 *  - Registered client service instances.
 *  Main function tasks:
 *  - Interface functions with the upper and lower layers.
 *  - Session state record management functions.
 *  - Protocol logic defining the state machines for both sender and receiver.
 *
 *  */
class LtpProtocol : public Object
{

public:
  /**
     * \brief Default Constructor
     */
  LtpProtocol (void);

  /**
   * \brief Destructor
   */
  ~LtpProtocol (void);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /* Requests from client service */

  /* \brief Must be called by clients to register their client service ID, this
   * must be done so the LTP can deliver received messages to this destination
   * \param id Client service id to register.
   * \param cb Client service function that will receive status reports.
   * \return true on success, false otherwise.
   */
  bool RegisterClientService (uint64_t id, const CallbackBase &cb );

  /*
   *  \brief Unregister client, this will no longer be a valid destination
   *  \param id Client Service Id to remove.
   */
  void UnregisterClientService (uint64_t id);

  /*
   * \brief Request the transmission of a block of client service data.
   * As defined in RFC-5326 Section 4.1.
   * \param sourceClientService Source client service id.
   * \param dstClientService Destination client service id.
   * \param dstLtpEngine Destination LTP engine id.
   * \param data Block of client service data to transmit
   * \param rdSize Size of red part (data that needs to be sent in a reliable manner).
   * \return Number of generated segments.
   */
  uint32_t StartTransmission ( uint64_t sourceClientService, uint64_t dstClientService,uint64_t dstLtpEngine, std::vector<uint8_t> data, uint64_t rdSize );

  /*
   * \brief Requests the cancellation of a session.
   * As defined in RFC-5326 Section 4.2.
   * \param id Session Id of the session to cancel.
   */
  void CancelSession (SessionId id);

  /* Engines Interaction */

  /*
   * \brief Send buffered data, intended for internal usage only.
   * Users should use the StartTransmission() method.
   * \param cla Ltp Convergence layer adapter linked to the remote ltp engine.
   */
  void Send (Ptr<LtpConvergenceLayerAdapter> cla);

  /*
   * \brief Receive packet from lower layer.
   * \param packet received packet.
   * \param cla Ltp Convergence layer adapter which received the packet.
   */
  void Receive (Ptr<Packet> packet, Ptr<LtpConvergenceLayerAdapter> cla);

  /* Getters Methods */

  /**
   * \return  Local Engine ID.
   */
  uint64_t GetLocalEngineId () const;
  /**
   * \return Checkpoint retransmission limit
   */
  uint32_t GetCheckPointRetransLimit () const;
  /**
   * \return Report segment retransmission
   */
  uint32_t GetReportRetransLimit () const;
  /**
   * \return Reception problem limit
   */
  uint32_t GetReceptionProblemLimit () const;
  /**
   * \return Cancellation segment retransmission limit
   */
  uint32_t GetCancellationRetransLimit () const;
  /**
   * \return  Retransmission cycle limit
   */
  uint32_t GetRetransCycleLimit () const;
  /**
   * Get node of this ltp protocol instance
   * \return pointer to node
   */
  Ptr<Node> GetNode () const;
  /**
   * Get Convergence layer adapter linked to a given remote engine id
   * \param engineId Ltp engine id.
   * \return pointer to convergence layer adapter.
   */
  Ptr<LtpConvergenceLayerAdapter> GetConvergenceLayerAdapter (uint64_t engineId);
  /**
   * \brief Check whether the LTP engine is active for the given time or not
   * \param t Specifies a time point in simulation time for which to test whether
   * the ltp-engine will be active or not.
   * \return True if the LTP engine is active, false otherwise
   */
  bool IsActive (Time t);

  /* Setter functions */

  /**
   * Set node of this ltp protocol instance
   * \param node
   */
  void SetNode (Ptr<Node> node);

  /**
   * Set Ip resolution table for this ltp protocol instance, allows conversion
   * between ltp engine ids and ip addresses.
   * \param rprot Ltp Ip resolution table.
   */
  void SetIpResolutionTable (Ptr<LtpIpResolutionTable> rprot);

  void SetBpCallback(Callback<void, Ptr<Packet>> cb);

  /**
   * Enable link state cue notifications from the given ltp convergence layer adapter.
   * \param cla pointer to the ltp convergence layer adapter.
   */
  void EnableLinkStateCues (Ptr<LtpConvergenceLayerAdapter> cla);

  /*
   * Stores a new convergence layer adapter.
   * \param cla pointer to convergence layer adapter.
   */
  bool AddConvergenceLayerAdapter (Ptr<LtpConvergenceLayerAdapter> cla);



private:

  /*
   * \brief Closes a session and frees resources.
   * \param id Session Id of the session to close.
   */
  void CloseSession (SessionId id);

  /*
   * \brief Signals the reception of the full red part.
   * \param id Session id.
   */
  void SignifyRedPartReception (SessionId id);

  /*
   * \brief Signals the reception of a green segment.
   * \param id Session id.
   */
  void SignifyGreenPartSegmentArrival (SessionId id);

  /*
   * \brief Given a destination and a block of data, splits the block
   * into N segments of mtu size and enqeueues them for transmission
   * into the outbound transmission queue of the provided session state record.
   * \param dstClientService Destination client service id.
   * \param ssr session state record into which enqueue segments.
   * \param data Block of client service data to transmit
   * \param rdSize Size of red part (data that needs to be sent in a reliable manner).
   * \param rtx called for a retransmission ( this flags is used internally).
   */
  void EncapsulateBlockData (uint64_t dstClientService, Ptr<SessionStateRecord> ssr, std::vector<uint8_t> data, uint64_t rdSize, uint64_t claimOffset = 0, uint64_t claimLength = 0, uint32_t claimSerialNum = 0);
  
  Ptr<Packet> EncapsulateSegment (uint64_t dstClientService, SessionId id, std::vector<uint8_t> data, uint32_t offset, uint32_t length, SegmentType type, uint32_t cpSerialNum, uint32_t rpSerialNum);

  /*
   * \brief Generate and enqueue a Report segment in response to a CP.
   * \param id Session Id.
   * \param cpSerialNum Checkpoint serial number to which this RP responds.
   */
  void ReportSegmentTransmission (SessionId id, uint64_t cpSerialNum, uint64_t lower = 0, uint64_t upper =0);

  /*
   * \brief Generate and enqueue a Report ACK segment in response to a RP.
   *
   * Note: Reports may be received even when the session has already been closed,
   * A third parameter (reference to the CLA that received the report) is provided to
   * handle these cases.
   *
   * \param id Session Id.
   * \param cpSerialNum Report serial number to which this RA responds.
   * \param cla pointer to convergence layer adapter that received the report.
   */
  void ReportSegmentAckTransmission (SessionId id, uint64_t rpSerialNum, Ptr<LtpConvergenceLayerAdapter> cla);

  /* Retransmission Functions */

  /* \brief Retransmit lost data segments
   * \param id session id.
   * \param info RedSegment info, contains serial number for checkpoints and the
   *  bounds of the lost data.
   */
  void RetransmitSegment (SessionId id, RedSegmentInfo info);

  /* \brief Retransmit lost report segment
   * \param id session id.
   * \param info RedSegmentInfo contains data of the lost report.
   */
  void RetransmitReport (SessionId id, RedSegmentInfo info);

  /* \brief Retransmit lost checkpoint segment
   * \param id session id.
   * \param info RedSegmentInfo contains data of the lost checkpoint.
   */
  void RetransmitCheckpoint (SessionId id, RedSegmentInfo info);

  /*
   * \brief Starts the checkpoint retransmission timer.
   * \param id session id.
   * \param info RedSegmentInfo information about the transmitted checkpoint to be used in
   * case of segment loss.
   */
  void SetCheckPointTransmissionTimer (SessionId id, RedSegmentInfo info);

  /*
   * \brief Starts the report retransmission timer.
   * \param id session id.
   * \param info RedSegmentInfo information about the transmitted report to be used in
   * case of segment loss.
   */
  void SetReportReTransmissionTimer (SessionId id, RedSegmentInfo info);

/*
 * \brief Signals the end of transmission, closes the session if certain conditions are meet.
 * \param id session id.
 */
  void SetEndOfBlockTransmission (SessionId id);

  /*
   * \brief Check received data segments, if all red data received signals end of red part.
   * \param id session id.
   */
  void CheckRedPartReceived (SessionId id);

  Ptr<Node>  m_node; // Node in which this ltp engine is running

  typedef std::map<SessionId,Ptr<SessionStateRecord> > SessionStateRecords;
  typedef std::map<uint64_t, Ptr<ClientServiceStatus> > ClientServiceInstances;
  typedef std::map<uint64_t, Ptr<LtpConvergenceLayerAdapter> > ConvergenceLayerAdapters;

  SessionStateRecords           m_activeSessions;       //!<  Active sessions.
  ClientServiceInstances        m_activeClients;        //!<  Active client service instances.
  ConvergenceLayerAdapters      m_clas;                 //!< Mapping LtpEngineId with corresponding point-to-point link.

  Ptr<RandomVariableStream> m_randomSession;    ///< Provides session numbers.
  Ptr<RandomVariableStream> m_randomSerial;    ///< Provides serial numbers.

  /*
   *  Configurable parameters for the local LTP engine.
   *  Defined in CCSDS 734.1-R-2 - Annex B.
   */
  uint64_t m_localEngineId;             //!< Local Engine ID
  uint32_t m_cpRtxLimit;                //!< Checkpoint retransmission limit. RFC 5326 - section 6.7
  uint32_t m_rpRtxLimit;                //!< Report segment retransmission limit. RFC 5326 - section 6.8
  uint32_t m_rxProblemLimit;            //!< Reception problem limit. RFC 5326 - section 6.11
  uint32_t m_cxRtxLimit;                //!< Cancellation segment retransmission limit. RFC 5326 - section 6.16
  uint32_t m_rtxCycleLimit;             //!< Retransmission cycle limit. RFC 5326 - section 6.22

  uint8_t m_version; //!< Protocol version

  /* This is a simplification of the times presented in RFC 5325- Section 3.1.3: Timers.
   *
   * m_localDelays represents the sum of :
   * - outbound/inbound queue delays.
   * - protocol processing time.
   * - radiation time.
   *
   */
  Time m_localDelays;                                       //!< Local Processing Times (for use in timers)
  Time m_onewayLightTime;                                   //!< On way light time: time required to reach the remote ltp engine (for use in timers)

  Time m_inactivityLimit; 										//!< Time to maintain a session with no activity.
  /* Simple data structure to define time intervals */
  struct ActivationInterval
  {
    Time start;
    Time stop;
  };
  std::queue<ActivationInterval> m_localOperatingSchedule;  //!< Time intervals that the local LTP engine expects to be operating

  Callback<void, Ptr<Packet>> m_bpCallback;
};


} // namespace ltp
} // namespace ns3

#endif /* LTP_PROTOCOL_H */


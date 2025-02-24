#ifndef SCPSTP_NEWRENO_H
#define SCPSTP_NEWRENO_H

#include "ns3/tcp-rate-ops.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/tcp-congestion-ops.h"

namespace ns3 {

/**
 * \brief The NewReno implementation
 *
 * New Reno introduces partial ACKs inside the well-established Reno algorithm.
 * This and other modifications are described in RFC 6582.
 *
 * \see IncreaseWindow
 */
class ScpsTpNewReno : public TcpCongestionOps
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ScpsTpNewReno ();

  /**
   * \brief Copy constructor.
   * \param sock object to copy.
   */
  ScpsTpNewReno (const ScpsTpNewReno& sock);

  ~ScpsTpNewReno ();

  std::string GetName () const;

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);
  virtual Ptr<TcpCongestionOps> Fork ();

protected:
  virtual uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
};

} // namespace ns3

#endif // SCPSTP_NEWRENO_H

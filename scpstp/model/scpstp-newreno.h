#ifndef SCPSTP_NEWRENO_H
#define SCPSTP_NEWRENO_H

#include "ns3/tcp-rate-ops.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/tcp-congestion-ops.h"

namespace ns3 {

/**
 * \brief The NewReno implementation with enhancements for delay ACK handling
 *
 * The class builds upon the well-established Reno algorithm and incorporates
 * partial ACK handling as described in RFC 6582, while also introducing
 * improvements tailored for delay ACK scenarios.
 *
 * \see TcpCongestionOps
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

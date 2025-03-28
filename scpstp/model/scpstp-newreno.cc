#include "ns3/tcp-congestion-ops.h"
#include "ns3/log.h"
#include "ns3/scpstp-newreno.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ScpsTpNewReno");
NS_OBJECT_ENSURE_REGISTERED (ScpsTpNewReno);

TypeId
ScpsTpNewReno::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ScpsTpNewReno")
    .SetParent<TcpCongestionOps> ()
    .SetGroupName ("Internet")
    .AddConstructor<ScpsTpNewReno> ()
  ;
  return tid;
}

ScpsTpNewReno::ScpsTpNewReno (void) : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
}

ScpsTpNewReno::ScpsTpNewReno (const ScpsTpNewReno& sock)
  : TcpCongestionOps (sock)
{
  NS_LOG_FUNCTION (this);
}

ScpsTpNewReno::~ScpsTpNewReno (void)
{
}

/**
 * \brief Tcp NewReno slow start algorithm
 *
 * Defined in RFC 5681 as
 *
 * > During slow start, a TCP increments cwnd by at most SMSS bytes for
 * > each ACK received that cumulatively acknowledges new data.  Slow
 * > start ends when cwnd exceeds ssthresh (or, optionally, when it
 * > reaches it, as noted above) or when congestion is observed.  While
 * > traditionally TCP implementations have increased cwnd by precisely
 * > SMSS bytes upon receipt of an ACK covering new data, we RECOMMEND
 * > that TCP implementations increase cwnd, per:
 * >
 * >    cwnd += min (N, SMSS)                      (2)
 * >
 * > where N is the number of previously unacknowledged bytes acknowledged
 * > in the incoming ACK.
 *
 * The ns-3 implementation respect the RFC definition. Linux does something
 * different:
 * \verbatim
u32 tcp_slow_start(struct tcp_sock *tp, u32 acked)
  {
    u32 cwnd = tp->snd_cwnd + acked;

    if (cwnd > tp->snd_ssthresh)
      cwnd = tp->snd_ssthresh + 1;
    acked -= cwnd - tp->snd_cwnd;
    tp->snd_cwnd = min(cwnd, tp->snd_cwnd_clamp);

    return acked;
  }
  \endverbatim
 *
 * As stated, we want to avoid the case when a cumulative ACK increases cWnd more
 * than a segment size, but we keep count of how many segments we have ignored,
 * and return them.
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 * \return the number of segments not considered for increasing the cWnd
 */
uint32_t
ScpsTpNewReno::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - 1;
    }

  return 0;
}

/**
 * \brief NewReno congestion avoidance with delayed-ACK adaptation
 *
 * During congestion avoidance, cwnd is incremented by approximately 1 full-sized
 * segment per round-trip time (RTT). This implementation explicitly accounts for
 * delayed-ACK effects by scaling the window increase with the number of acknowledged
 * segments per ACK event.
 *
 * The window growth follows:
 * - For each received ACK: 
 *   Δcwnd = (segmentsAcked × segmentSize²) / cwnd
 * - Enforces minimum increment of 1 byte per ACK to prevent stagnation
 *
 * This modification ensures linear window growth compliant with RFC 5681 even
 * under delayed-ACK scenarios where multiple segments may be acknowledged by
 * a single ACK.
 *
 * \param tcb Internal congestion state
 * \param segmentsAcked Count of segments cumulatively acknowledged in this ACK,
 *                      reflecting delayed-ACK aggregation at the receiver
 */
void
ScpsTpNewReno::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  if (segmentsAcked > 0)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder * segmentsAcked);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);
    }
}

/**
 * \brief Try to increase the cWnd following the NewReno specification
 *
 * \see SlowStart
 * \see CongestionAvoidance
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
ScpsTpNewReno::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }

  /* At this point, we could have segmentsAcked != 0. This because RFC says
   * that in slow start, we should increase cWnd by min (N, SMSS); if in
   * slow start we receive a cumulative ACK, it counts only for 1 SMSS of
   * increase, wasting the others.
   *
   * // Incorrect assert, I am sorry
   * NS_ASSERT (segmentsAcked == 0);
   */
}

std::string
ScpsTpNewReno::GetName () const
{
  return "ScpsTpNewReno";
}

uint32_t
ScpsTpNewReno::GetSsThresh (Ptr<const TcpSocketState> state,
                         uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  return std::max (2 * state->m_segmentSize, bytesInFlight / 2);
  
}

Ptr<TcpCongestionOps>
ScpsTpNewReno::Fork ()
{
  return CopyObject<ScpsTpNewReno> (this);
}

} // namespace ns3


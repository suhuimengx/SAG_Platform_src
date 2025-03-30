
#ifndef SCPSTP_RX_BUFFER_H
#define SCPSTP_RX_BUFFER_H


#include <map>
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/sequence-number.h"
#include "ns3/ptr.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-option-sack.h"
#include "ns3/tcp-rx-buffer.h"
#include "ns3/scpstp-option-snack.h"

namespace ns3 {

class Packet;

/**
 *  \ingroup scpstp
 * 
 * \brief Rx reordering buffer for SCPS-TP
 */
class ScpsTpRxBuffer : public TcpRxBuffer
{ 
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  
  /**
   * \brief Constructor
   * \param n initial Sequence number to be received
   */
  ScpsTpRxBuffer (uint32_t n = 0);

  ScpsTpRxBuffer (const TcpRxBuffer &buffer);

  virtual ~ScpsTpRxBuffer ();

  /**
   * \brief Get the snack list
   *
   * The snack list can be empty, and it is updated each time Add or Extract
   * are called through the private method UpdateSackList.
   *
   * \return a list of isolated holes
   */
  virtual ScpsTpOptionSnack::SnackList GetSnackList (void) const;

  /**
   * \brief Get the size of the snack list
   *
   * \return the size of the snack list
   */
  virtual uint32_t GetSnackListSize (void) const;


protected:
  virtual void UpdateSackList (const SequenceNumber32 &head, const SequenceNumber32 &tail) override;

  virtual void ClearSackList (const SequenceNumber32 &seq) override;

private:


  /**
   * \brief Remove old blocks from the snack list
   *
   * Used to remove blocks already delivered to the application.
   *
   * After this call, in the SNACK list there will be only blocks with
   * sequence numbers greater than seq; it is perfectly safe to call this
   * function with an empty snack list.
   *
   * \param seq Last sequence to remove
   */
  void ClearSnackList (const SequenceNumber32 &seq);



  /**
   * \brief Update the snack list
   *
   * This method is called by UpdateSackList and is responsible for updating
   * the snack list.
   */
  void UpdateSnackList (void);

  ScpsTpOptionSnack::SnackList m_snackList; //!< Snack list

};




}

















#endif /* SCPSTP_RX_BUFFER_H */
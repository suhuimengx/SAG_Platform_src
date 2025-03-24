#ifndef SCPS_OPTION_CAPABILITIES_H
#define SCPS_OPTION_CAPABILITIES_H

#include "ns3/tcp-option.h"

namespace ns3 {

/**
 * \brief Defines the SCPS option capabilities
 *
 * SCPS Option Capabilities is used to define the capabilities of SCPS options.
 */

class ScpsOptionCapabilities : public TcpOption
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  ScpsOptionCapabilities ();
  virtual ~ScpsOptionCapabilities ();

  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  virtual uint8_t GetKind (void) const;
  virtual uint32_t GetSerializedSize (void) const;

  void SetSnackEnabled (bool snackEnabled);
  bool GetSnackEnabled (void) const;

private:
  bool m_snackEnabled;
};

} // namespace ns3

#endif /* SCPS_OPTION_CAPABILITIES_H */
 
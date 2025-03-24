 #include "scps-option-capabilities.h"
 #include "ns3/log.h"
 #include "ns3/tcp-header.h"
 
 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("ScpsOptionCapabilities");
 
 NS_OBJECT_ENSURE_REGISTERED (ScpsOptionCapabilities);
 
 ScpsOptionCapabilities::ScpsOptionCapabilities ()
   : TcpOption (),
   m_snackEnabled(true)
 {
 }
 
 ScpsOptionCapabilities::~ScpsOptionCapabilities ()
 {
 }
 
 TypeId
 ScpsOptionCapabilities::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::ScpsOptionCapabilities")
     .SetParent<TcpOption> ()
     .SetGroupName ("Internet")
     .AddConstructor<ScpsOptionCapabilities> ()
   ;
   return tid;
 }
 
 TypeId
 ScpsOptionCapabilities::GetInstanceTypeId (void) const
 {
   return GetTypeId ();
 }
 
 void
 ScpsOptionCapabilities::Print (std::ostream &os) const
 {
   os << "ScpsOptionCapabilities";
 }
 
 uint32_t
 ScpsOptionCapabilities::GetSerializedSize (void) const
 {
   return 4;
 }
 
 void
 ScpsOptionCapabilities::Serialize (Buffer::Iterator start) const
 {
    Buffer::Iterator i = start;
    i.WriteU8 (GetKind ());// Kind
    i.WriteU8 (4);        // Length
    uint8_t bitVector = 0x00;
    if(m_snackEnabled)
    {
      bitVector |= (1 << 6); 
    }
    i.WriteU8 (bitVector); // Bit Vector
    i.WriteU8 (0x00);// Connection ID
 }
uint32_t
ScpsOptionCapabilities::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  uint8_t readKind = i.ReadU8 ();
  if (readKind != GetKind ())
    {
      NS_LOG_WARN ("Malformed SCPS Capabilities option");
      return 0;
    }

  uint8_t size = i.ReadU8 ();
  if (size != 4)
    {
      NS_LOG_WARN ("Malformed SCPS Capabilities option");
      return 0;
    }

  uint8_t bitVector = i.ReadU8 ();
  if((bitVector & 0x40) == 0x40)
  {
    m_snackEnabled = true;
  }
  else
  {
    m_snackEnabled = false;
  }
  uint8_t connectionId = i.ReadU8 ();
  std::cout << "SCPS Capabilities option: Bit Vector: " << (int)bitVector << " Connection ID: " << (int)connectionId << std::endl;

  return GetSerializedSize ();
}
 
uint8_t
ScpsOptionCapabilities::GetKind (void) const
{
  return TcpOption::SCPSCAPABILITIES;
}

void
ScpsOptionCapabilities::SetSnackEnabled (bool snackEnabled)
{
  m_snackEnabled = snackEnabled;
}

bool
ScpsOptionCapabilities::GetSnackEnabled (void) const
{
  return m_snackEnabled;
}
 
 } // namespace ns3
 
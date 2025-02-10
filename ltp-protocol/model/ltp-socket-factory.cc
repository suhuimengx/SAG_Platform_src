#include "ltp-socket-factory.h"
#include "ns3/socket.h"
#include "ns3/log.h"

namespace ns3 {
namespace ltp {

NS_LOG_COMPONENT_DEFINE ("LtpSocketFactory");

NS_OBJECT_ENSURE_REGISTERED (LtpSocketFactory);

TypeId
LtpSocketFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LtpSocketFactory")
    .SetParent<SocketFactory> ()
    .SetGroupName ("Ltp")
    .AddConstructor<LtpSocketFactory> ();
  return tid;
}


}
} // namespace ns3

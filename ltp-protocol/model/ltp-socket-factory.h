
#ifndef LTP_SOCKET_FACTORY_H
#define LTP_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"

namespace ns3 {
//namespace ltp {

class Socket;
/**
 * \ingroup ltp
 * \defgroup ltpsocket LtpSocket
 *
 * \brief A factory for creating LTP sockets
 *
 */
class LtpSocketFactory : public SocketFactory
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

};

} // namespace ns3
//}


#endif /* LTP_SOCKET_FACTORY_H */

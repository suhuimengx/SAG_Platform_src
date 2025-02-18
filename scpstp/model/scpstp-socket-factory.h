#ifndef SCPSTP_SOCKET_FACTORY_H
#define SCPSTP_SOCKET_FACTORY_H

#include "ns3/scpstp-socket-base.h"
#include "ns3/socket-factory.h"  
namespace ns3 {

class Socket;

/**
 * \ingroup socket
 * \ingroup scpstp
 *
 * \brief API to create SCPS-TP socket instances 
 *
 * This abstract class defines the API for SCPS-TP sockets.
 * This class also holds the global default variables used to
 * initialize newly created sockets, such as values that are
 * set through the sysctl or proc interfaces in Linux.

 * All SCPS-TP socket factory implementations must provide an implementation 
 * of CreateSocket
 * below, and should make use of the default values configured below.
 * 
 * \see ScpsTpSocketFactoryImpl
 * \see TcpSocketFactory
 *
 */
class ScpsTpSocketFactory : public SocketFactory
{
public:
    /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

};

}  // namespace ns3

#endif // SCPSTP_SOCKET_FACTORY_H
#ifndef MYSYA_QSERVICE_TRANSPORT_AGENT_H
#define MYSYA_QSERVICE_TRANSPORT_AGENT_H

#include <functional>
#include <unordered_map>

#include <mysya/ioevent/event_loop.h>
#include <mysya/qservice/message_queue.h>
#include <mysya/qservice/tcp_service.h>
#include <mysya/util/timestamp.h>

namespace mysya {
namespace ioevent {

class EventChannel;
class TcpSocket;

}  // namespace ioevent
}  // namespace mysya

namespace mysya {
namespace qservice {

class TransportAgent {
  friend class TcpService;

 public:
  class TransportChannel;

  typedef std::unordered_map<int, TransportChannel *> TransportChannelHashmap;
  typedef std::unordered_map<int64_t, ::mysya::ioevent::TcpSocket *> TimerSocketHashmap;
  typedef std::unordered_map< ::mysya::ioevent::TcpSocket *, int64_t> SocketTimerHashmap;
  typedef ::mysya::ioevent::EventLoop::ExpireCallback ExpireCallback;

  explicit TransportAgent(TcpService *host, ::mysya::ioevent::EventLoop *network_event_loop,
      ::mysya::ioevent::EventLoop *app_event_loop);
  ~TransportAgent();

  ::mysya::ioevent::EventLoop *GetNetworkEventLoop() const;
  ::mysya::ioevent::EventLoop *GetAppEventLoop() const;

  // running in app event loop.
  bool SendMessage(int sockfd, const char *data, size_t size);

  // running in ReceiveDecodeCallback.
  int DoReceive(int sockfd, const char *data, int size);

  // runnint in app event loop.
  int AsyncConnect(const ::mysya::ioevent::SocketAddress &addr, int timeout_ms);
  int Listen(const ::mysya::ioevent::SocketAddress &addr);

 private:
  void AddAsyncConnectSocketTimer(::mysya::ioevent::TcpSocket *socket,
      int expire_ms, const ExpireCallback &cb);
  void RemoveAsyncConnectSocketTimer(::mysya::ioevent::TcpSocket *socket);

  bool AddTcpSocket(::mysya::ioevent::TcpSocket *tcp_socket);

  ::mysya::ioevent::TcpSocket *RemoveTcpSocket(int sockfd);
  void CloseTcpSocket(int sockfd);

  // async connect.
  void OnAsyncConnectSocketWrite(::mysya::ioevent::EventChannel *event_channel);
  void OnAsyncConnectError(::mysya::ioevent::EventChannel *event_channel);
  void OnAsyncConnectTimeout(int64_t timer_id);

  // socket connection.
  void OnSocketRead(::mysya::ioevent::EventChannel *event_channel);
  void OnSocketWrite(::mysya::ioevent::EventChannel *event_channel);
  void OnSocketError(::mysya::ioevent::EventChannel *event_channel);

  // wakeup listen addr handlers.
  void OnHandleListen(::mysya::ioevent::TcpSocket *listen_socket,
      const ::mysya::ioevent::SocketAddress &addr);
  void OnHandleListened(int listen_sockfd);
  void OnHandleListenError(int listen_sockfd, int socket_errno);

  // wakeup async connect handlers.
  void OnHandleAsyncConnect(::mysya::ioevent::TcpSocket *socket, int timeout_ms);
  void OnHandleAsyncConnected(int sockfd);
  void OnHandleAsyncConnectError(int sockfd, int socket_errno);

  // Wakeup connection handlers.
  void OnHandleConnected(int sockfd);
  void OnHandleClosed(int sockfd);
  void OnHandleError(int sockfd, int sys_errno);

  void OnReceiveQueueReady(int host, const char *data, int size);
  void OnSendQueueReady(int host, const char *data, int size);

  TcpService *host_;

  ::mysya::ioevent::EventLoop *network_event_loop_;
  ::mysya::ioevent::EventLoop *app_event_loop_;
  TransportChannelHashmap channels_;

  MessageQueue receive_queue_;
  MessageQueue send_queue_;

  TimerSocketHashmap async_connect_timer_sockets_;
  SocketTimerHashmap async_connect_socket_timers_;
};

}  // qservice
}  // namespace mysya 

#endif  // MYSYA_QSERVICE_TRANSPORT_AGENT_H

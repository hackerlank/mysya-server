#include <mysya/qservice/transport_agent.h>

#include <memory>

#include <mysya/ioevent/dynamic_buffer.h>
#include <mysya/ioevent/event_loop.h>
#include <mysya/ioevent/mutex.h>
#include <mysya/ioevent/logger.h>
#include <mysya/ioevent/tcp_socket.h>

#include <mysya/qservice/message_queue.h>

namespace mysya {
namespace qservice {

class TransportAgent::TransportChannel {
 public:
  TransportChannel(TransportAgent *agent, ::mysya::ioevent::TcpSocket *tcp_socket,
      int init_size = 1024, int ext_size = 256);
  ~TransportChannel();

  TransportAgent *GetHost() const { return this->host_; }
  int GetFileDescriptor() const { return this->tcp_socket_->GetFileDescriptor(); }

  ::mysya::ioevent::TcpSocket *GetTcpSocket() const { return this->tcp_socket_; }
  ::mysya::ioevent::DynamicBuffer *GetReadBuffer() { return &this->read_buffer_; }
  ::mysya::ioevent::DynamicBuffer *GetWriteBuffer() { return &this->write_buffer_; }

  void SetErrno(int value) { this->errno_ = value; }
  int GetErrno() const { return this->errno_; }

  bool SendMessage(const char *data, size_t size);

 private:
  TransportAgent *host_;
  ::mysya::ioevent::TcpSocket *tcp_socket_;

  ::mysya::ioevent::DynamicBuffer read_buffer_;
  ::mysya::ioevent::DynamicBuffer write_buffer_;

  int errno_;
};

TransportAgent::TransportChannel::TransportChannel(TransportAgent *agent,
    ::mysya::ioevent::TcpSocket *tcp_socket, int init_size, int ext_size)
  : host_(agent), tcp_socket_(tcp_socket),
    read_buffer_(init_size, ext_size),
    write_buffer_(init_size, ext_size),
    errno_(0) {}

TransportAgent::TransportChannel::~TransportChannel() {}

bool TransportAgent::TransportChannel::SendMessage(const char *data, size_t size) {
  int remain_size = size;
  int send_size = 0;

  if (this->write_buffer_.ReadableBytes() <= 0) {
    send_size = this->tcp_socket_->Write(data, size);
    if (send_size >= 0) {
      remain_size -= send_size;
    } else if (errno != EAGAIN) {
      this->SetErrno(errno);
      return false;
    }
  }

  if (remain_size > 0) {
    this->write_buffer_.Append(data + send_size, remain_size);
  } else {
    // TODO: send complete callback.
  }

  return true;
}

TransportAgent::TransportAgent(TcpService *host, ::mysya::ioevent::EventLoop *network_event_loop,
    ::mysya::ioevent::EventLoop *app_event_loop)
  : host_(host),
    network_event_loop_(network_event_loop),
    app_event_loop_(app_event_loop),
    receive_queue_(network_event_loop_, app_event_loop_,
        std::bind(&TransportAgent::OnReceiveQueueReady, this,
          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 10),
    send_queue_(app_event_loop_, network_event_loop_,
        std::bind(&TransportAgent::OnSendQueueReady, this,
          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 10) {}

TransportAgent::~TransportAgent() {}

::mysya::ioevent::EventLoop *TransportAgent::GetNetworkEventLoop() const {
  return this->network_event_loop_;
}

::mysya::ioevent::EventLoop *TransportAgent::GetAppEventLoop() const {
  return this->app_event_loop_;
}

bool TransportAgent::AddTcpSocket(::mysya::ioevent::TcpSocket *tcp_socket) {
  int sockfd = tcp_socket->GetFileDescriptor();
  TransportChannelHashmap::const_iterator iter = this->channels_.find(sockfd);
  if (iter != this->channels_.end()) {
    MYSYA_ERROR("Duplicate tcp_socket(%d).", sockfd);
    return false;
  }

  ::mysya::ioevent::EventChannel *event_channel = tcp_socket->GetEventChannel();

  std::unique_ptr<TransportChannel> transport_channel(
    new (std::nothrow) TransportChannel(this, tcp_socket));
  if (transport_channel.get() == NULL) {
    MYSYA_ERROR("Allocate TransportChannel(%d) failed.", sockfd);
    return false;
  }

  tcp_socket->SetAppHandle(transport_channel.get());
  this->channels_.insert(std::make_pair(sockfd, transport_channel.get()));

  event_channel->SetReadCallback(
      std::bind(&TransportAgent::OnSocketRead, this, std::placeholders::_1));
  event_channel->SetWriteCallback(
      std::bind(&TransportAgent::OnSocketWrite, this, std::placeholders::_1));
  event_channel->SetErrorCallback(
      std::bind(&TransportAgent::OnSocketError, this, std::placeholders::_1));

  if (event_channel->AttachEventLoop(this->network_event_loop_) == false) {
    MYSYA_ERROR("sockfd(%d) EventChannel::AttachEventLoop() failed.", sockfd);
    return false;
  }

  this->app_event_loop_->PushWakeupCallback(
      std::bind(&TransportAgent::OnHandleConnected, this, sockfd));

  transport_channel.release();

  return true;
}

bool TransportAgent::SendMessage(int sockfd, const char *data, size_t size) {
  return this->send_queue_.Push(sockfd, data, size) != (int)size;
}

int TransportAgent::DoReceive(int sockfd, const char *data, int size) {
  return this->receive_queue_.Push(sockfd, data, size);
}

::mysya::ioevent::TcpSocket *TransportAgent::RemoveTcpSocket(int sockfd) {
  ::mysya::ioevent::TcpSocket *tcp_socket = NULL;

  TransportChannelHashmap::iterator iter = this->channels_.find(sockfd);
  if (iter == this->channels_.end()) {
    TransportChannel *transport_channel = iter->second;
    tcp_socket = iter->second->GetTcpSocket();
    this->channels_.erase(iter);
    delete transport_channel;
  }

  return tcp_socket;
}

void TransportAgent::CloseTcpSocket(int sockfd) {
  delete this->RemoveTcpSocket(sockfd);
}

void TransportAgent::OnSocketRead(::mysya::ioevent::EventChannel *event_channel) {
  ::mysya::ioevent::TcpSocket *socket =
    (::mysya::ioevent::TcpSocket *)event_channel->GetAppHandle();

  int sockfd = socket->GetFileDescriptor();
  TransportChannel *transport_channel = (TransportChannel *)socket->GetAppHandle();
  ::mysya::ioevent::DynamicBuffer *receive_buffer = transport_channel->GetReadBuffer();

  bool received = false;
  bool peer_closed = false;

  for (;;) {
    int readable_bytes = socket->ReadableSize();
    if (readable_bytes <= 0) {
      peer_closed = true;
      break;
    }

    receive_buffer->ReserveWritableBytes(readable_bytes);

    int received_bytes = socket->Read(receive_buffer->WriteBegin(),
        receive_buffer->WritableBytes());
    if (received_bytes > 0) {
      receive_buffer->WrittenBytes(received_bytes);
      received = true;
    } else if (received_bytes == 0) {
      peer_closed = true;
    } else if (errno != EAGAIN) {
      transport_channel->SetErrno(errno);
      this->app_event_loop_->PushWakeupCallback(
          std::bind(&TransportAgent::OnHandleError, this, sockfd, errno));
      this->CloseTcpSocket(sockfd);
      return;
    } else {
      // do nothing.
    }

    break;
  }

  // received callback.
  TcpService::ReceiveDecodeCallback receive_decode_cb = this->host_->GetReceiveDecodeCallback();
  if (received == true and receive_decode_cb) {
    receive_decode_cb(sockfd, receive_buffer);
  }

  if (peer_closed == true) {
    this->app_event_loop_->PushWakeupCallback(
        std::bind(&TransportAgent::OnHandleClosed, this, sockfd));
    this->CloseTcpSocket(sockfd);
  }
}

void TransportAgent::OnSocketWrite(::mysya::ioevent::EventChannel *event_channel) {
  ::mysya::ioevent::TcpSocket *socket =
    (::mysya::ioevent::TcpSocket *)event_channel->GetAppHandle();

  int sockfd = socket->GetFileDescriptor();
  TransportChannel *transport_channel = (TransportChannel *)socket->GetAppHandle();
  ::mysya::ioevent::DynamicBuffer *send_buffer = transport_channel->GetWriteBuffer();

  int sendable_bytes = send_buffer->ReadableBytes();
  int send_bytes = socket->Write(send_buffer->ReadBegin(), sendable_bytes);
  if (send_bytes < 0) {
    transport_channel->SetErrno(errno);

    this->app_event_loop_->PushWakeupCallback(
        std::bind(&TransportAgent::OnHandleError, this, sockfd, errno));
    this->CloseTcpSocket(sockfd);
    return;
  }

  send_buffer->ReadBytes(send_bytes);
}

void TransportAgent::OnSocketError(::mysya::ioevent::EventChannel *event_channel) {
  ::mysya::ioevent::TcpSocket *socket =
    (::mysya::ioevent::TcpSocket *)event_channel->GetAppHandle();

  int sockfd = socket->GetFileDescriptor();
  TransportChannel *transport_channel = (TransportChannel *)socket->GetAppHandle();
  transport_channel->SetErrno(errno);

  this->app_event_loop_->PushWakeupCallback(
      std::bind(&TransportAgent::OnHandleError, this, sockfd, errno));
  this->CloseTcpSocket(sockfd);
}

void TransportAgent::OnHandleConnected(int sockfd) {
  TcpService::ConnectCallback cb = this->host_->GetConnectCallback();
  if (cb) {
    cb(sockfd, this);
  }
}

void TransportAgent::OnHandleClosed(int sockfd) {
  TcpService::CloseCallback cb = this->host_->GetCloseCallback();
  if (cb) {
    cb(sockfd);
  }
}

void TransportAgent::OnHandleError(int sockfd, int sys_errno) {
  TcpService::ErrorCallback cb = this->host_->GetErrorCallback();
  if (cb) {
    cb(sockfd, sys_errno);
  }
}

void TransportAgent::OnReceiveQueueReady(int host, const char *data, int size) {
  TcpService::ReceiveCallback cb = this->host_->GetReceiveCallback();
  if (cb) {
    cb(host, data, size);
  }
}

void TransportAgent::OnSendQueueReady(int host, const char *data, int size) {
    TransportChannelHashmap::iterator iter = this->channels_.find(host);
    if (iter == this->channels_.end()) {
      return;
    }

    TransportChannel *transport_channel = iter->second;
    transport_channel->SendMessage(data, size);
}

}  // namespace qservice
}  // namespace mysya

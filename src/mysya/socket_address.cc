#include <mysya/socket_address.h>

#include <errno.h>
#include <arpa/inet.h>

#include <mysya/logger.h>

namespace mysya {

SocketAddress::SocketAddress() : port_(0), generated_(false) {}

SocketAddress::SocketAddress(const std::string &host, uint16_t port) :
  host_(host), port_(port), generated_(false) {}

SocketAddress::SocketAddress(const SocketAddress &from) {
  this->CopyFrom(from);
}

SocketAddress::~SocketAddress() {}

const std::string &SocketAddress::GetHost() const {
  return this->host_;
}

void SocketAddress::SetHost(const std::string &value) {
  this->host_ = value;
  this->generated_ = false;
}

uint16_t SocketAddress::GetPort() const {
  return this->port_;
}

void SocketAddress::SetPort(uint16_t value) {
  this->port_ = value;
  this->generated_ = false;
}

const NativeAddress *SocketAddress::GetNativeHandle() const {
  if (this->generated_ == false) {
    this->GenerateNativeHandle();
  }

  return &this->native_handle_;
}

void SocketAddress::SetNativeHandle(const NativeAddress &value) {
  char host[1024];

  if (::inet_ntop(AF_INET, &value.sin_addr, host, sizeof(host)) == NULL) {
    MYSYA_ERROR("::inet_ntop() failed.")
    return false;
  }

  this->host_ = host;
  this->port_ = ::htons(value.sin_port);
  ::memcpy(this->native_handle_, &value, sizeof(this->native_handle_));

  this->generated_ = true;
}

size_t SocketAddress::GetNativeHandleSize() const {
  if (this->generated_ == false) {
    this->GenerateNativeHandle();
  }

  return sizeof(this->native_handle_);
}

void SocketAddress::CopyFrom(const SocketAddress &from) {
  if (&from == this) return;

  this->host_ = from.host_;
  this->port_ = from.port_;

  this->generated_ = from.generated_;
  ::memcpy(&this->native_handle_, from.native_handle_,
      sizeof(from.native_handle_));
}

void SocketAddress::GenerateNativeHandle() {
  ::memset(&this->native_handle_, 0, sizeof(this->native_handle_));

  if (::inet_pton(AF_INET, this->host_.data(), &this->native_handle_.sin_addr) != 1) {
    MYSYA_ERROR("::inet_pton() failed.")
    return;
  }

  this->native_handle_.sin_family = AF_INET;
  this->native_handle_.sin_port = htons(this->port_);

  this->generated_ = true;
}

}  // namespace mysya

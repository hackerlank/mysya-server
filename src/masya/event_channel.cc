#include <masya/event_channel.h>

#include <masya/logger.h>
#include <masya/event_loop.h>

namespace masya {

EventChannel::EventChannel()
  : fd_(-1), event_loop_(NULL) {}
EventChannel::~EventChannel() {}

bool EventChannel::AttachEventLoop(EventLoop *event_loop) {
  if (this->event_loop_ != NULL) {
    this->DetachEventLoop();
  }

  if (event_loop->AddEventChannel(this) == false) {
    MASYA_ERROR("EventLoop::AddEventChannel() failed.");
    return false;
  }

  this->event_loop_ = event_loop;

  return true;
}

void EventChannel::DetachEventLoop() {
  if (this->event_loop_ == NULL) {
    return;
  }

  this->event_loop_->RemoveEventChannel(this);
  this->event_loop_ = NULL;
}

void EventChannel::SetReadCallback(const ReadCallback &cb) {
  this->read_cb_ = cb;
  this->UpdateEventLoop();
}

void EventChannel::SetWriteCallback(const WriteCallback &cb) {
  this->write_cb_ = cb;
  this->UpdateEventLoop();
}

void EventChannel::SetErrorCallback(const ErrorCallback &cb) {
  this->error_cb_ = cb;
  this->UpdateEventLoop();
}

void EventChannel::UpdateEventLoop() {
  if (this->event_loop_ != NULL) {
    this->event_loop_->UpdateEventChannel(this);
  }
}

}  // namespace masya

#ifndef MYSYA_EVENT_LOOP_H
#define MYSYA_EVENT_LOOP_H

#include <sys/epoll.h>

#include <functional>
#include <vector>

#include <ext/hash_set>

#include <mysya/class_util.h>
#include <mysya/timestamp.h>

namespace mysya {

class EventChannel;
class TimingWheel;

class EventLoop {
 public:
  typedef std::vector<struct epoll_event> EventVector;
  typedef std::function<void (int64_t)> TimerCallback;
  typedef __gnu_cxx::hash_set<intptr_t> EventChannelHashset;

  EventLoop();
  ~EventLoop();

  bool Looping() const;
  void Loop();

  void Quit();
  void Wakeup();

  bool AddEventChannel(EventChannel *channel);
  bool RemoveEventChannel(EventChannel *channel);
  bool UpdateEventChannel(EventChannel *channel);

  int64_t StartTimer(int expire_ms, const TimerCallback &cb,
      int call_times = -1);
  void StopTimer(int64_t timer_id);

#ifndef _MYSYA_DEBUG_
  int64_t GetTimerDebugTickCounts() const;
  void SetTimerDebugTickCounts(int64_t value) const;
#endif  // _MYSYA_DEBUG_

  const Timestamp &GetTimestamp() const { return this->timestamp_; }

 private:
  bool CheckEventChannelRemoved(EventChannel *channel) const;

  static const int kReadEventMask;
  static const int kWriteEventMask;
  static const int kErrorEventMask;

  bool quit_;
  int epoll_fd_;
  EventVector active_events_;
  EventChannelHashset removed_event_channels_;

  pid_t thread_id_;

  TimingWheel *timing_wheel_;
  Timestamp timestamp_;

  MYSYA_DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}  // namespace mysya

#endif  // MYSYA_EVENT_LOOP_H

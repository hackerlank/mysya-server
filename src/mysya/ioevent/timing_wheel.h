#ifndef MYSYA_IOEVENT_TIMING_WHEEL_H
#define MYSYA_IOEVENT_TIMING_WHEEL_H

#include <stdint.h>

#include <functional>
#include <vector>
#include <unordered_map>

#include <mysya/util/class_util.h>
#include <mysya/util/timestamp.h>

namespace mysya {

namespace util {

class Timestamp;

}  // namespace util

namespace ioevent {

class EventChannel;
class EventLoop;

class TimingWheel {
  class Timer;
  class TimerIdAllocator;
  class Wheel;
  typedef std::unordered_map<int64_t, Timer *> TimerHashmap;
  typedef std::vector<Wheel *> WheelVector;

 public:
  typedef std::function<void (int64_t)> ExpireCallback;

  TimingWheel(int tick_ms, EventLoop *event_loop);
  ~TimingWheel();

  int64_t AddTimer(const ::mysya::util::Timestamp &now, int expire_ms,
      const ExpireCallback &cb, int call_times = -1);
  void RemoveTimer(int64_t timer_id);

  void SetTimestamp(const ::mysya::util::Timestamp &timestamp) {
    this->timestamp_ = timestamp;
  }

#ifndef _MYSYA_DEBUG_
  int64_t GetDebugTickCounts() const {
    return this->debug_tick_counts_;
  }
  void SetDebugTickCounts(int64_t value) {
    this->debug_tick_counts_ = value;
  }
#endif  // _MYSYA_DEBUG_

 private:
  Wheel *GetWheel(int index) const;

  void AddWheel(Timer *timer, int expire_tick_cout);
  void RemoveWheel(Timer *timer);

  void OnRead(EventChannel *event_channel);
  void OnExpired(const ::mysya::util::Timestamp &now_timestamp);

  // The wheel num and each wheel's bucket num.
  static const int kTimingWheelNum = 5;
  static const int kTimingWheelBucketNum[kTimingWheelNum];

  int tick_ms_;
  int undo_nsec_;
  ::mysya::util::Timestamp timestamp_;

  EventLoop *event_loop_;
  EventChannel *event_channel_;

  TimerHashmap timers_;
  TimerIdAllocator *timer_ids_;

  WheelVector wheels_;

#ifndef _MYSYA_DEBUG_
  int64_t debug_tick_counts_;
#endif  // _MYSYA_DEBUG_

  MYSYA_DISALLOW_COPY_AND_ASSIGN(TimingWheel);
};

}  // namespace ioevent
}  // namespace mysya

#endif  // MYSYA_IOEVENT_TIMING_WHEEL_H

#ifndef MASYA_THREAD_H
#define MASYA_THREAD_H

#include <functional>

#include "masya/condition_variable.h"
#include "masya/class_util.h"
#include "masya/mutex.h"

namespace masya {

class Thread {
 public:
  typedef std::function<void ()> ThreadFunc;

  Thread();
  ~Thread();

  void Start(const ThreadFunc &thread_func, bool joinable = true);
  bool Join();
  void Detach();

  bool Joinable();

 private:
  static void *StartThread(void *arg);

  pthread_t tid_;
  ThreadFunc thread_func_;
  Mutex mutex_;
  ConditionVariable cond_;

  bool started_;
  bool joined_;

  MASYA_DISALLOW_COPY_AND_ASSIGN(Thread);
};

template <class ThreadType>
class ThreadGuardTemplate {
 public:
  explicit ThreadGuardTemplate(ThreadType &thread)
    : thread_(thread) {
  }

  ~ThreadGuardTemplate() {
    if (this->thread_.joinable()) {
      this->thread_.join();
    }
  }

 private:
  ThreadType thread_;

  MASYA_DISALLOW_COPY_AND_ASSIGN(ThreadGuardTemplate<ThreadType>);
};

typedef ThreadGuardTemplate<Thread> ThreadGuard;

}  // namespace masya

#endif  // MASYA_THREAD_H

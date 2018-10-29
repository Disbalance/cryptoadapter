/***************************************************
 * task_queue.h
 * Created on Mon, 01 Oct 2018 09:31:29 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <pthread.h>
#include <atomic>
#include <thread>
#include <queue>

#define WAIT_LOOP_DURATION  0xffffff

namespace platform {

class TaskQueue;

class Task {
public:
  virtual ~Task() { }
  virtual void run(TaskQueue*) = 0;
};

// Implemented as spinlock queue
// May consume CPU a lot, but has low latency
class TaskQueue {
public:
  TaskQueue();
  virtual ~TaskQueue();

  virtual void onIdle();

  void run() noexcept;
  void stop();
  void flushQueue() noexcept;

  inline void processAndDelete(Task *task) noexcept {
    task->run(this);
    delete task;
  }

  inline void push(Task *task) noexcept {
    if(!task) return;

    if(pthread_self() == m_runThread) {
      m_queue.emplace_back(task);
    } else {
      SafeLock lock(*this);
      m_queue.emplace_back(task);
    }
    m_hasData = true;
  }

  long getMaxSize() { return m_maxSize; }
  inline void lock() noexcept {
    int waitCycle = 0;
    while(__builtin_expect(m_lock.test_and_set(std::memory_order_acquire), 0)) {
      if(!(++waitCycle & WAIT_LOOP_DURATION)) {
        std::this_thread::yield();
      }
    }
  }

  inline void unlock() noexcept {
    m_lock.clear();
  }

  inline bool empty() noexcept {
    return !m_hasData;
  }

  inline bool running() noexcept {
    return m_running;
  }

private:
  class SafeLock {
  private:
    TaskQueue &m_queue;

  public:
    inline SafeLock(TaskQueue &q) noexcept
      : m_queue(q)
    {
      m_queue.lock();
    }

    inline ~SafeLock() {
      m_queue.unlock();
    }
  };

  std::vector<Task*> m_queue;
  std::atomic_flag m_lock;
  std::atomic<bool> m_hasData;
  pthread_t m_runThread;
  std::atomic<bool> m_running;
  long m_maxSize;
};

}

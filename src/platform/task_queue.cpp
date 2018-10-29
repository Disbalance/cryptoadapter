/***************************************************
 * task_queue.cpp
 * Created on Mon, 01 Oct 2018 09:43:12 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "task_queue.h"

namespace platform {

TaskQueue::TaskQueue()
  : m_lock(ATOMIC_FLAG_INIT)
  , m_hasData(false)
  , m_running(false)
  , m_maxSize(0)
{
  m_queue.reserve(200);
}

TaskQueue::~TaskQueue() {
  stop();
  for(auto task : m_queue) {
    delete task;
  }
  m_queue.resize(0);
}

void TaskQueue::stop() {
  m_running = false;
}

//void TaskQueue::preProcess(Task *task) { }
void TaskQueue::onIdle() { }

void TaskQueue::flushQueue() noexcept {
  lock();
  for(auto task : m_queue) {
    processAndDelete(task);
  }
  m_queue.resize(0);
  unlock();
}

void TaskQueue::run() noexcept {
  m_runThread = pthread_self();
  m_running = true;
  std::vector<Task*> localQueue;
  localQueue.reserve(200);

  while(m_running) {
    while(m_hasData.exchange(false)) {
      //m_hasData = false;
      {
      SafeLock lock(*this);
      m_queue.swap(localQueue);
      }

      for(auto task : localQueue) {
        processAndDelete(task);
      }
      localQueue.resize(0);
    }
    onIdle();
    // Induce the context switch so it less likely happen when we process the queue
    if(!m_hasData)
      sched_yield();
  }
}


}

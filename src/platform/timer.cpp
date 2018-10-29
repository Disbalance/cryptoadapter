/***************************************************
 * timer.cpp
 * Created on Fri, 05 Oct 2018 20:26:26 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <iostream>
#include "timer.h"

namespace platform {

TimerService *TimerService::s_instance = NULL;

TimerService::TimerService() {
  using namespace boost::asio;

  if(s_instance != NULL) {
    throw std::runtime_error("TimerService instance already exists!");
  }

  s_instance = this;
  m_thread = std::thread(
      [this]() {
        io_service::work work(m_service);
        m_service.run();
      }
    );
}

TimerService::~TimerService() {
  s_instance = NULL;
  m_service.stop();
  m_thread.join();
}

TimerService &TimerService::instance() {
  if(s_instance == NULL) {
    throw std::runtime_error("TimerService instance does not exists!");
  }

  return *s_instance;
}

Timer::~Timer() {
  m_timer.cancel();
}

void Timer::stop() {
  m_timer.cancel();
}

};

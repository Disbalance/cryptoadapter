/***************************************************
 * timer.h
 * Created on Fri, 05 Oct 2018 19:56:30 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#ifndef BOOST_ASIO_HAS_STD_CHRONO
#define BOOST_ASIO_HAS_STD_CHRONO
#endif

#include <thread>
#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>

namespace platform {

class Timer;
class TimerService;

typedef std::function<void(const boost::system::error_code&)> TimerCallback;


class Timer {
public:
  ~Timer();
  void stop();

  template<typename DurationType>
  void start(DurationType duration) {
    m_timer.expires_from_now(duration);
    m_timer.async_wait(m_callback);
  }

private:
  template<typename Callable>
  Timer(Callable callback, boost::asio::io_service &service)
    : m_timer(service)
    , m_callback(
                  [callback, this](const boost::system::error_code &error)
                  {
                    if(error != boost::asio::error::operation_aborted) {
                      callback(this);
                    }
                  }
                )
  { }

  Timer(const Timer&) = delete;

  boost::asio::high_resolution_timer m_timer;
  TimerCallback m_callback;
  friend class TimerService;
};


class TimerService {
public:
  TimerService();
  ~TimerService();
  static TimerService &instance();

  template<typename Callable>
  Timer *createTimer(Callable callback) {
    return new Timer(callback, m_service);
  }

private:
  static TimerService *s_instance;
  boost::asio::io_service m_service;
  std::thread m_thread;
};

}

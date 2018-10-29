/***************************************************
 * log.h
 * Created on Thu, 20 Sep 2018 10:29:29 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <sstream>
#include <thread>
#include <unistd.h>
#include "tbb/concurrent_queue.h"
#include "perflogger.h"

namespace platform {

struct LoggerBuffer;

class Logger {
public:
  Logger();
  virtual ~Logger();
  static Logger &instance();
  void start();
  void stop();

  enum Severity {
    Critical = 0,
    Error,
    Warning,
    Info,
    Debug,
  };

  void setLogLevel(Severity &s);
  Severity getLogLevel();

  Logger &write(LoggerBuffer *msg);
  void output(LoggerBuffer *buffer);

private:
  void run();
  void flushQueue();

  static Logger *s_instance;
  std::thread m_runThread;
  bool m_running;
  std::ostream *m_output;
  tbb::concurrent_queue<LoggerBuffer *> m_queue;
  Severity m_severity;
};


struct LoggerBuffer {
  mutable std::stringstream message;
  Logger::Severity severity;
  struct timespec timestamp;
};

class LoggerMessage {
public:
  LoggerMessage(Logger::Severity);
  virtual ~LoggerMessage();

  template<typename T>
  LoggerMessage &operator <<(T what) {
    if(m_willWrite) {
      m_buffer->message << what;
    }
    return *this;
  }

  void logf(const char *format, ...);
  const char *what() const;
  Logger::Severity severity() const;

private:
  mutable std::unique_ptr<LoggerBuffer> m_buffer;
  bool m_willWrite;

  friend class LoggerTask;
};

class LogCritical
  : public LoggerMessage {
public:
  LogCritical();
};

class LogError
  : public LoggerMessage {
public:
  LogError();
};

class LogWarning
  : public LoggerMessage {
public:
  LogWarning();
};

class LogInfo
  : public LoggerMessage {
public:
  LogInfo();
};

class LogDebug
  : public LoggerMessage {
public:
  LogDebug();
};

class LogPerfLoggerSink
  : public PerfLoggerSink {
public:
  LogPerfLoggerSink(Logger::Severity severity = Logger::Info);
  virtual void log(const struct timespec &diff, std::string location) override;

private:
  Logger::Severity m_severity;
};

}

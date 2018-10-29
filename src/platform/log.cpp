/***************************************************
 * log.cpp
 * Created on Thu, 20 Sep 2018 13:28:38 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <cassert>
#include <cstdarg>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <chrono>
#include "log.h"

namespace platform {

Logger *Logger::s_instance = NULL;

Logger::Logger()
  : m_severity(Logger::Debug)
{
  if(s_instance)
  {
    throw std::runtime_error("Logger instance already exists!");
  }

  m_output = &std::cout;
  s_instance = this;
  start();
};

void Logger::start() {
  m_runThread = std::thread([this]() { run(); });
}

void Logger::stop() {
  m_running = false;
  if(m_runThread.joinable()) {
    m_runThread.join();
  }
}

void Logger::flushQueue() {
  LoggerBuffer *buffer;
  while(m_queue.try_pop(buffer)) {
    std::unique_ptr<LoggerBuffer> msg(buffer);
    output(buffer);
  }
}

void Logger::run() {
  m_running = true;

  while(m_running) {
    flushQueue();
  }
}

void Logger::setLogLevel(Logger::Severity &s) {
  m_severity = s;
}

Logger::Severity Logger::getLogLevel() {
  return m_severity;
}

Logger::~Logger()
{
  stop();
  flushQueue();
  s_instance = NULL;
}

Logger &Logger::instance()
{
  assert(s_instance);

  return *s_instance;
}

Logger &Logger::write(LoggerBuffer *buffer)
{
  m_queue.push(buffer);
  return *this;
}

void Logger::output(LoggerBuffer *buffer)
{
  if(m_severity < buffer->severity) {
    return;
  }

  struct timespec &ts(buffer->timestamp);
  (*m_output) << ts.tv_sec << "." << std::setw(3) << std::setfill('0') << (ts.tv_nsec / 1000000) << ":";
  switch(buffer->severity) {
  case Critical:
    (*m_output) << "C:";
    break;
  case Error:
    (*m_output) << "E:";
    break;
  case Warning:
    (*m_output) << "W:";
    break;
  case Info:
    (*m_output) << "I:";
    break;
  case Debug:
    (*m_output) << "D:";
    break;
  default:
    assert(!"Unknown log level!");
  }
  (*m_output) << buffer->message.str() << std::endl;
}

LoggerMessage::LoggerMessage(Logger::Severity severity)
  : m_buffer(new LoggerBuffer)
  , m_willWrite(true)
{
  m_buffer->severity = severity;
  clock_gettime(CLOCK_REALTIME, &m_buffer->timestamp);
  if(severity > Logger::instance().getLogLevel()) {
    m_willWrite = false;
  }
}

LoggerMessage::~LoggerMessage()
{
  if(m_willWrite) {
    Logger::instance().write(m_buffer.release());
  }
}

void LoggerMessage::logf(const char *format, ...)
{
  char buffer[65535];
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  m_buffer->message.write(buffer, len);
}

const char *LoggerMessage::what() const
{
  return m_buffer->message.str().c_str();
}

Logger::Severity LoggerMessage::severity() const
{
  return m_buffer->severity;
}

LogCritical::LogCritical()
  : LoggerMessage(Logger::Critical)
{ }

LogError::LogError()
  : LoggerMessage(Logger::Error)
{ }

LogWarning::LogWarning()
  : LoggerMessage(Logger::Warning)
{ }

LogInfo::LogInfo()
  : LoggerMessage(Logger::Info)
{ }

LogDebug::LogDebug()
  : LoggerMessage(Logger::Debug)
{ }

LogPerfLoggerSink::LogPerfLoggerSink(Logger::Severity severity)
  : m_severity(severity)
{ }

void LogPerfLoggerSink::log(const struct timespec &diff, std::string location)
{
  if(location.empty()) {
    LoggerMessage message(m_severity);
    message << "Time: " << diff.tv_sec << "." << std::setw(9) << std::setfill('0') << diff.tv_nsec;
  } else {
    LoggerMessage message(m_severity);
    message << "Time for [" << location << "]: " << diff.tv_sec << "." << std::setw(9) << std::setfill('0') << diff.tv_nsec;
  }
}

Logger::Severity m_severity;

}

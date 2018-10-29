/***************************************************
 * perflogger.h
 * Created on Sat, 29 Sep 2018 13:25:41 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <time.h>
#include <iostream>
#include <string>

namespace platform {

class PerfLoggerSink {
public:
  virtual void log(const struct timespec &diff, std::string location) = 0;
};

class OStreamPerfLoggerSink
  : public PerfLoggerSink {
public:
  OStreamPerfLoggerSink(std::ostream &);
  virtual void log(const struct timespec &diff, std::string location) override;

private:
  std::ostream &m_output;
};

class PerfLogger {
public:
  PerfLogger(PerfLoggerSink *, const char *location);
  PerfLogger(PerfLoggerSink *);
  ~PerfLogger();

private:
  PerfLoggerSink *m_output;
  std::string m_location;
  struct timespec m_start;
};

}

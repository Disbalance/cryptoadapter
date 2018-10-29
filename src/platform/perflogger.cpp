/***************************************************
 * perflogger.cpp
 * Created on Sat, 29 Sep 2018 19:22:38 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <iomanip>
#include "perflogger.h"

namespace platform {

PerfLogger::PerfLogger(PerfLoggerSink *sink, const char *location)
  : m_output(sink)
  , m_location(location)
{
  clock_gettime(CLOCK_REALTIME, &m_start);
}

PerfLogger::PerfLogger(PerfLoggerSink *sink)
  : m_output(sink)
{
  clock_gettime(CLOCK_REALTIME, &m_start);
}

PerfLogger::~PerfLogger() {
  struct timespec stop;
  struct timespec diff;
  clock_gettime(CLOCK_REALTIME, &stop);

  diff.tv_sec = stop.tv_sec - m_start.tv_sec;
  long long nsec_diff = stop.tv_nsec - m_start.tv_nsec;

  if(nsec_diff < 0) {
    diff.tv_sec --;
    nsec_diff += 1000000000l;
  }

  diff.tv_nsec = nsec_diff;

  m_output->log(diff, std::move(m_location));
}

OStreamPerfLoggerSink::OStreamPerfLoggerSink(std::ostream &output)
  : m_output(output)
{ }

void OStreamPerfLoggerSink::log(const struct timespec &diff, std::string location) {
  if(location.empty()) {
    m_output << "Timeframe: " << diff.tv_sec << "." << std::setw(9) << std::setfill('0') << diff.tv_nsec << std::endl;
  } else {
    m_output << location << " timeframe: " << diff.tv_sec << "." << std::setw(9) << std::setfill('0') << diff.tv_nsec << std::endl;
  }
}

}

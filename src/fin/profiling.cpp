/***************************************************
 * profiling.cpp
 * Created on Sat, 06 Oct 2018 11:02:22 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <time.h>
#include "profiling.h"

namespace fin {

ProfilingTag::ProfilingTag() {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);

  timestamp = now.tv_sec * 1000000000 + now.tv_nsec;
}

ProfilingTag::ProfilingTag(unsigned long ts)
  : timestamp(ts)
{ }

unsigned long ProfilingTag::getProfileStat() {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  unsigned long currentTimestamp = now.tv_sec * 1000000000 + now.tv_nsec;

  return currentTimestamp - timestamp;
}

}

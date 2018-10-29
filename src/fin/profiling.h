/***************************************************
 * profiling.h
 * Created on Sat, 06 Oct 2018 11:00:49 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

namespace fin {

struct ProfilingTag {
  unsigned long timestamp;

  ProfilingTag();
  ProfilingTag(const ProfilingTag &copy) = default;
  ProfilingTag(unsigned long timestamp);
  unsigned long getProfileStat();
};

}

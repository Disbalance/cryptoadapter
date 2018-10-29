/***************************************************
 * cstringmap.h
 * Created on Sat, 29 Sep 2018 18:28:09 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <map>
#include <cstring>

namespace fin {

struct CompareCString {
  bool operator()(char const *a, char const *b) {
    return std::strcmp(a, b) < 0;
  }
};

template <typename T>
using CStringMap = std::map<const char *, T, CompareCString>;

template <typename T>
using CStringMultimap = std::multimap<const char *, T, CompareCString>;

}

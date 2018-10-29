/**
 * @file
 * @brief Class for fixed point and fixed accuracy representation
 *
 */
#pragma once

#include <cstring>
#include <list>
#include <map>
#include <set>
#include "instrument.h"
#include "cstringmap.h"

//!< Classes and functions related to finances
namespace fin {

// Use map with char* key, since we meet the following condition:
// a) we never delete elements
// b) we don't want std::string constructor to be called upon accessing map elements

struct CompareInstrumentHandles {
  bool operator()(InstrumentHandle a, InstrumentHandle b) {
    if(a->first < b->first)
      return true;

    if(b->first < a->first)
      return false;

    return a->second < b->second;
  }
};

typedef std::set<InstrumentHandle, CompareInstrumentHandles> InstrumentSet;

/** Instrument registry implements the registry of all symbols and instruments that are added
 * and supported by the system (can be configured via config file)
 */
class InstrumentRegistry {
public:
  InstrumentRegistry(); //!< Default constructor
  ~InstrumentRegistry();

  void addSymbol(const char *symbol);
  SymbolHandle findSymbol(const char *symbol);
  void addInstrument(const char*, const char*);
  void addInstrument(SymbolHandle, SymbolHandle);
  InstrumentHandle findInstrument(const char*, const char*);
  InstrumentHandle findInstrument(SymbolHandle, SymbolHandle);

  std::list<SymbolHandle> getSymbols();
  std::list<InstrumentHandle> getInstruments();

  static InstrumentRegistry &instance();

private:
  std::list<Symbol> m_symbolList;
  std::list<Instrument> m_instrumentList;

  CStringMap<SymbolHandle> m_symbols;
  InstrumentSet m_instruments;

  static InstrumentRegistry *s_instance;
};

}

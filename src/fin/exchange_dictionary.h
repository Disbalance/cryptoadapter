/***************************************************
 * exchange_dictionary.h
 * Created on Sat, 29 Sep 2018 12:05:03 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <list>
#include <map>
#include "instrument.h"
#include "instrument_registry.h"

namespace fin {

class ExchangeDictionary {
public:
  ExchangeDictionary();

  void addSymbolTranslation(const char *exchangeFormat, SymbolHandle internalFormat);
  void addInstrumentTranslation(const char *exchangeFormat, InstrumentHandle internalFormat);

  const char *symbolToExchange(SymbolHandle);
  const char *instrumentToExchange(InstrumentHandle);
  SymbolHandle symbolFromExchange(const char *);
  InstrumentHandle instrumentFromExchange(const char *);

private:
  std::list<std::string> m_exchangeSymbols;
  std::list<std::string> m_exchangeInstruments;
  CStringMap<SymbolHandle> m_symbolFromExchangeMap;
  CStringMap<InstrumentHandle> m_instrumentFromExchangeMap;
  std::map<SymbolHandle, const char *> m_symbolToExchangeMap;
  std::map<InstrumentHandle, const char *> m_instrumentToExchangeMap;
};

}

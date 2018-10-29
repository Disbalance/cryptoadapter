/***************************************************
 * exchange_dictionary.cpp
 * Created on Sat, 29 Sep 2018 12:16:10 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "exchange_dictionary.h"

namespace fin {

ExchangeDictionary::ExchangeDictionary()
{ }

void ExchangeDictionary::addSymbolTranslation(const char *exchangeFormat, SymbolHandle internalFormat)
{
  m_exchangeSymbols.push_back(exchangeFormat);
  m_symbolFromExchangeMap.insert(std::make_pair(m_exchangeSymbols.back().c_str(), internalFormat));
  m_symbolToExchangeMap.insert(std::make_pair(internalFormat, m_exchangeSymbols.back().c_str()));
}

void ExchangeDictionary::addInstrumentTranslation(const char *exchangeFormat, InstrumentHandle internalFormat)
{
  m_exchangeInstruments.push_back(exchangeFormat);
  m_instrumentFromExchangeMap.insert(std::make_pair(m_exchangeInstruments.back().c_str(), internalFormat));
  m_instrumentToExchangeMap.insert(std::make_pair(internalFormat, m_exchangeInstruments.back().c_str()));
}

const char *ExchangeDictionary::symbolToExchange(SymbolHandle symbol)
{
  auto exchangeFormat = m_symbolToExchangeMap.find(symbol);

  if(exchangeFormat == m_symbolToExchangeMap.end()) {
    return NULL;
  }

  return exchangeFormat->second;
}

const char *ExchangeDictionary::instrumentToExchange(InstrumentHandle instrument)
{
  auto exchangeFormat = m_instrumentToExchangeMap.find(instrument);

  if(exchangeFormat == m_instrumentToExchangeMap.end()) {
    return NULL;
  }

  return exchangeFormat->second;
}

SymbolHandle ExchangeDictionary::symbolFromExchange(const char *exchangeFormat)
{
  auto symbol = m_symbolFromExchangeMap.find(exchangeFormat);
  if(__builtin_expect(symbol == m_symbolFromExchangeMap.end(), 0))
  {
    return NoSymbol;
  }
  return symbol->second;
}

InstrumentHandle ExchangeDictionary::instrumentFromExchange(const char *exchangeFormat)
{
  auto instrument = m_instrumentFromExchangeMap.find(exchangeFormat);
  if(__builtin_expect(instrument == m_instrumentFromExchangeMap.end(), 0))
  {
    return NoInstrument;
  }
  return instrument->second;
}

}

/***************************************************
 * symbol_data.cpp
 * Created on Thu, 27 Sep 2018 20:12:54 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <cstring>
#include <stdexcept>
#include "instrument_registry.h"

namespace fin {

InstrumentRegistry *InstrumentRegistry::s_instance;

InstrumentRegistry::InstrumentRegistry()
{
  if(s_instance) {
    throw std::runtime_error("InstrumentRegistry instance already exists!");
  }

  s_instance = this;
}

InstrumentRegistry &InstrumentRegistry::instance()
{
  if(!s_instance) {
    throw std::runtime_error("InstrumentRegistry instance does not exist!");
  }

  return *s_instance;
}

InstrumentRegistry::~InstrumentRegistry()
{
  s_instance = NULL;
}

void InstrumentRegistry::addSymbol(const char *symbol)
{
  if(m_symbols.find(const_cast<char*>(symbol)) != m_symbols.end()) {
    return;
  }

  // Using char * keys to avoid constructing strings
  // We get the const char* as c_str() from strings, 
  // so they are automatically deleted when the string is deleted
  m_symbolList.push_back(Symbol(symbol, symbol));
  m_symbols.insert(std::make_pair(m_symbolList.back().symbol.c_str(), &m_symbolList.back()));
}

SymbolHandle InstrumentRegistry::findSymbol(const char *symbol)
{
  const auto &symbolIterator = m_symbols.find(symbol);
  if(__builtin_expect(symbolIterator == m_symbols.end(), 0)) {
    return NoSymbol;
  }

  return symbolIterator->second;
}

void InstrumentRegistry::addInstrument(const char *a, const char*b)
{
  return addInstrument(findSymbol(a), findSymbol(b));
}

void InstrumentRegistry::addInstrument(SymbolHandle a, SymbolHandle b)
{
  if(a == NoSymbol || b == NoSymbol) {
    return;
  }

  Instrument current(a, b);
  if(m_instruments.find(&current) != m_instruments.end()) {
    return;
  }

  m_instrumentList.push_back(std::move(current));
  m_instruments.insert(&m_instrumentList.back());
}

InstrumentHandle InstrumentRegistry::findInstrument(SymbolHandle a, SymbolHandle b)
{
  if(a == NoSymbol || b == NoSymbol) {
    return NoInstrument;
  }

  Instrument current(a, b);
  auto instrumentIterator = m_instruments.find(&current);
  if(__builtin_expect(instrumentIterator == m_instruments.end(), 0)) {
    return NoInstrument;
  }

  return *instrumentIterator;
}

InstrumentHandle InstrumentRegistry::findInstrument(const char* a, const char* b)
{
  return findInstrument(findSymbol(a), findSymbol(b));
}

std::list<SymbolHandle> InstrumentRegistry::getSymbols()
{
  std::list<SymbolHandle> result;
  for(auto &val : m_symbols) {
    result.push_back(val.second);
  }
  return result;
}

std::list<InstrumentHandle> InstrumentRegistry::getInstruments()
{
  std::list<InstrumentHandle> result;
  for(auto val : m_instruments) {
    result.push_back(val);
  }
  return result;
}

}

/***************************************************
 * market.cpp
 * Created on Thu, 27 Sep 2018 09:30:04 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "market.h"

namespace fin {

using namespace interface;

BaseStockDataConnector::BaseStockDataConnector(StockDataObserver *observer)
  : m_observer(observer)
{ }

void BaseStockDataConnector::invalidateData(InstrumentHandle symbol, ProfilingTag tag)
{
  if(m_observer) {
    m_observer->invalidateData(symbol, tag);
  }
}

void BaseStockDataConnector::addOrderbookEntry(OrderBookEntry entry, ProfilingTag tag)
{
  if(m_observer) {
    m_observer->orderbookEntryAdded(std::move(entry), tag);
  }
}

void BaseStockDataConnector::addOrderbookBulk(OrderBookList bulk, ProfilingTag tag) {
  if(m_observer) {
    m_observer->orderbookEntriesBulk(std::move(bulk), tag);
  }
}

void BaseStockDataConnector::addCandleStickEntry(CandleStickEntry entry, ProfilingTag tag)
{
  if(m_observer) {
    m_observer->candleStickEntryAdded(std::move(entry), tag);
  }
}

void BaseStockDataConnector::addSymbol(SymbolHandle sym, ProfilingTag tag) {
  if(m_observer) {
    m_observer->symbolAdded(sym, tag);
  }
}

void BaseStockDataConnector::addInstrument(InstrumentHandle instrument, ProfilingTag tag) {
  if(m_observer) {
    m_observer->instrumentAdded(instrument, tag);
  }
}

void BaseStockDataConnector::setConnectorError(std::exception_ptr e) {
  if(m_observer) {
    m_observer->dataConnectorError(e);
  }
}

const char *BaseStockDataConnector::getName() {
  return m_name.c_str();
}

void BaseStockDataConnector::setName(const std::string &name) {
  m_name = name;
}

}

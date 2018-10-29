/**
 * @file
 * @brief Interfaces for trade exchange adapters
 *
 */
#pragma once

#include <vector>
#include "profiling.h"
#include "orderbook.h"

//! Classes and functions related to finances
namespace fin {
//! Interfaces

typedef std::vector<InstrumentHandle> InstrumentsList;

namespace interface {

//! Observer interface for marketing data events
class StockDataObserver {
public:
  virtual void invalidateData(InstrumentHandle, ProfilingTag) = 0; //!< Called when market data needs to be invalidated
  virtual void orderbookEntryAdded(OrderBookEntry, ProfilingTag) = 0; //!< Called when orderbook entry added
  virtual void orderbookEntriesBulk(OrderBookList, ProfilingTag) = 0; //!< Called when multiple orderbook updates are added at once
  virtual void candleStickEntryAdded(CandleStickEntry, ProfilingTag) = 0; //!< Called when a candlestick is added
  virtual void symbolAdded(SymbolHandle, ProfilingTag) = 0; //!< Trade exchange announced new trade symbol
  virtual void instrumentAdded(InstrumentHandle, ProfilingTag) = 0; //!< Trade exchange announced new trade instrument
  virtual void dataConnectorError(std::exception_ptr) = 0; //!< Called when connector detects error (network failure, etc)
};

//! Interface for marketing data connector
class StockDataConnector {
public:
  virtual ~StockDataConnector() { }
  virtual void subscribe(const InstrumentsList& instruments) = 0; //!< Implement connector-specific function to subscribe to given instruments
  virtual void fetchStack(InstrumentHandle symbol) = 0; //!< Implement connector-specific function to try fetch full market depth if possible
  virtual void fetchCandleSticks(InstrumentHandle, long intervalSeconds, long since) = 0; //!< Implement connector-specific function to fetch candle sticks with given interval
  virtual void fetchSymbols() = 0; //!< Implement connector-specific function to fetch supported symbols
  virtual void fetchInstruments() = 0; //!< Implement connector-specific function to fetch available instruments
  virtual void config(std::string config) = 0; //!< Implement connector-specific function to configure connector
  virtual void start() = 0; //!< Implement connector-specific function to start connector
  virtual void stop() = 0; //!< Implement connector-specific function to stop connector

  virtual const char *getName() = 0; //!< Get connector name as it appears in the config
  virtual void setName(const std::string &) = 0; //!< Set connector name
};

} // End of namespace interface

//! Interface for marketing data connector
class BaseStockDataConnector 
  : public interface::StockDataConnector {
public:
  BaseStockDataConnector(interface::StockDataObserver *);
  virtual ~BaseStockDataConnector() { }

  const char *getName(); //!< Get connector name as it appears in the config
  void setName(const std::string &); //!< Set connector name

protected:
  void invalidateData(InstrumentHandle instr = NoInstrument, ProfilingTag = ProfilingTag());
  void addOrderbookEntry(OrderBookEntry, ProfilingTag = ProfilingTag());
  void addOrderbookBulk(OrderBookList, ProfilingTag = ProfilingTag());
  void addCandleStickEntry(CandleStickEntry, ProfilingTag = ProfilingTag());
  void addSymbol(SymbolHandle, ProfilingTag = ProfilingTag());
  void addInstrument(InstrumentHandle, ProfilingTag = ProfilingTag());
  void setConnectorError(std::exception_ptr);

private:
  interface::StockDataObserver *m_observer;
  std::string m_name;
};

}

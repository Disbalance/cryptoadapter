/**
 * @file
 * @brief Interfaces for trade exchange adapters
 *
 */
#pragma once

#include <vector>
#include "numeric.h"
#include "instrument.h"
#include "order.h"

//! Classes and functions related to finances
namespace fin {

//! Represents candle stick
struct CandleStickEntry {
  InstrumentHandle instrument; //!< Trading instrument
  long timestamp; //!< Start timestamp
  long interval; //!< Candlestick interval
  FixedNumber high; //!< Highest price
  FixedNumber open; //!< Open (starting) price
  FixedNumber close; //!< Close (final) price
  FixedNumber low; //!< Lowest price
  FixedNumber volume; //!< Trade volume

  CandleStickEntry(); //!< Default constructor
  CandleStickEntry(CandleStickEntry &&); //!< Move constructor
  CandleStickEntry(const CandleStickEntry &); //!< Copy constructor
};

//! Represents orderbook entry
struct OrderBookEntry {
  InstrumentHandle instrument = NoInstrument; //!< Trading instrument
  OrderDir    direction; //!< Direction - bid or ask
  FixedNumber amount; //!< Trade amount
  FixedNumber price; //!< Price
  long        timestamp; //!< Timestamp when fetched

  OrderBookEntry() = default; //!< Default constructor
  OrderBookEntry(OrderBookEntry &&) = default; //!< Move constructor
  OrderBookEntry(const OrderBookEntry &) = default; //!< Copy constructor
};

typedef std::vector<OrderBookEntry> OrderBookList;

}

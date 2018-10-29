/***************************************************
 * orderbook.cpp
 * Created on Wed, 26 Sep 2018 15:04:25 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "orderbook.h"

namespace fin {

CandleStickEntry::CandleStickEntry()
  : instrument(NoInstrument)
  , timestamp(0)
  , interval(0)
  , high(0)
  , open(0)
  , close(0)
  , low(0)
{ }

CandleStickEntry::CandleStickEntry(CandleStickEntry &&copy)
  : instrument(copy.instrument)
  , timestamp(copy.timestamp)
  , interval(copy.interval)
  , high(std::move(copy.high))
  , open(std::move(copy.open))
  , close(std::move(copy.close))
  , low(std::move(copy.low))
  , volume(std::move(copy.volume))
{ }

CandleStickEntry::CandleStickEntry(const CandleStickEntry &copy)
  : instrument(copy.instrument)
  , timestamp(copy.timestamp)
  , interval(copy.interval)
  , high(copy.high)
  , open(copy.open)
  , close(copy.close)
  , low(copy.low)
  , volume(copy.volume)
{ }

/*
OrderBookEntry::OrderBookEntry()
  : instrument(NoInstrument)
  , direction(OrderDir::Bid)
  , amount(0)
  , price(0)
{ }

OrderBookEntry::OrderBookEntry(OrderBookEntry &&copy)
  : instrument(copy.instrument)
  , direction(copy.direction)
  , amount(std::move(copy.amount))
  , price(std::move(copy.price))
{ }
*/

}

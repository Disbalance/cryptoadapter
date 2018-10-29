#pragma once

#include <stdexcept>
#include <list>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <fin/instrument_registry.h>
#include <fin/market.h>
#include <fin/exchange_dictionary.h>
#include <pjson.h>
#include "connector_rest_price.h"
#include "connector_ws_price.h"

namespace adaptor {
namespace example {

/**
 * This class implements StockDataConnector interface for OKex trading platform
 * It incorporates and utilizes both websocket and REST API connectors
 * @see https://github.com/okcoin-okex/API-docs-OKEx.com/blob/master/API-For-Spot-EN/WEBSOCKET%20API%20for%20SPOT.md
 * @see fin::interface::StockDataConnector
 */
class PriceAdapter
  : public connector::example::WSPriceConnector
  , public connector::example::RESTPriceConnector
  , public fin::BaseStockDataConnector {
public:
  PriceAdapter(fin::interface::StockDataObserver *observer);
  virtual ~PriceAdapter() override = default;

   //! Implements subscribe method of the interface
  virtual void subscribe(const fin::InstrumentsList &instruments) override;
  //! Implements fetchStack method of the interface
  virtual void fetchStack(fin::InstrumentHandle symbol) override;
  //! Implements fetchCandleSticks method of the interface
  virtual void fetchCandleSticks(fin::InstrumentHandle symbol, long interval, long since) override;
  //! Implements fetchSymbols method of the interface
  virtual void fetchSymbols() override;
  //! Implements fetchInstruments method of the interface
  virtual void fetchInstruments() override;
  //! Implements config method of the interface
  virtual void config(std::string conf) override;
  //! Implements start method of the interface
  virtual void start() override;
  //! Implements stop method of the interface
  virtual void stop() override;

protected:
  // Overrides WSSpotPriceAPI::onData
  virtual void onData(const char *msg, size_t size, unsigned long timestamp) override;
  // Overrides WSSpotPriceAPI::onClose
  virtual void onClose() override;
  // Overrides WSSpotPriceAPI::onPingTimeout
  virtual void onPingTimeout() override;
  // Overrides RESTSpotPriceAPI::onTimeout
  virtual void onTimeout(std::string symbol, connector::example::RequestContext *userdata) override;
  // Overrides RESTSpotPriceAPI::onHTTPError
  virtual void onHTTPError(std::string symbol, const platform::HttpResponse *r, connector::example::RequestContext *userdata) override;
  // Overrides RESTSpotPriceAPI::onDepthResponse
  virtual void onDepthResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata = nullptr) override;
  // Overrides RESTSpotPriceAPI::onTickerResponse
  virtual void onTickerResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata = nullptr) override;
  // Overrides RESTSpotPriceAPI::onTradesResponse
  virtual void onTradesResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata = nullptr) override;
  // Overrides RESTSpotPriceAPI::onCandleSticksResponse
  virtual void onKlineResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata = nullptr) override;

  void parseData(long timestamp, unsigned long netTimestamp, fin::InstrumentHandle instr, const pjson::value_variant &data);
  template<typename InsertIterator>
  void processDirection(long timestamp, fin::OrderDir direction, const pjson::value_variant &arr, 
                        fin::InstrumentHandle instrumentHandle, InsertIterator inserter)
  {
    fin::OrderBookEntry entry;
    entry.instrument = instrumentHandle;
    entry.direction = direction;
    entry.timestamp = timestamp;

    for (size_t i = 0; i < arr.size(); ++i) {
      entry.price.assign(arr[i][0].as_string_ptr());
      entry.amount.assign(arr[i][1].as_string_ptr());
      inserter++ = entry;
    }
  }

  fin::ExchangeDictionary m_exchangeDictionary;

private:
  struct RequestedInterval
    : public connector::example::RequestContext
  {
    RequestedInterval(unsigned long i)
      : interval(i)
    { }
    unsigned long interval;
  };
  bool m_started;
  std::mutex m_subscriptionLock;
  fin::InstrumentsList m_subscriptions;
};

}
}
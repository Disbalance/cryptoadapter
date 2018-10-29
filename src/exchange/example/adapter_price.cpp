#include <string.h>
#include <memory.h>
#include <platform/log.h>
#include <pjson.h>
#include "adapter_price.h"

namespace adaptor {
namespace example {

PriceAdapter::PriceAdapter(fin::interface::StockDataObserver *observer)
  : fin::BaseStockDataConnector(observer)
  , m_started(false)
{ }

/**
 * Implementation of the subscribe method of the interface
 * @param instrumentHandle The handle to the instrument to subscribe to
 * @see fin::interface::StockDataConnector::subscribe(fin::InstrumentHandle)
 */
void PriceAdapter::subscribe(const fin::InstrumentsList& instruments)
{
  std::lock_guard<std::mutex> lock(m_subscriptionLock);
  for(auto instrumentHandle : instruments) {
    auto instrument = m_exchangeDictionary.instrumentToExchange(instrumentHandle);
    if(instrument != nullptr) {
      WSPriceConnector::subscribe(instrument);
      m_subscriptions.push_back(instrumentHandle);
    } else {
      platform::LogError() << "no mapping for instrument " << instrumentHandle;
    }
  }
}

void PriceAdapter::onDepthResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata)
{
  auto instr = m_exchangeDictionary.instrumentFromExchange(symbol);

  pjson::document doc;
  doc.deserialize_in_place(const_cast<char*>(data.c_str()));

  if(doc.is_object()) {
    invalidateData(instr); // We got full depth
    long timestamp = time(NULL) * 1000;
    fin::ProfilingTag tag;
    // Parse response
    parseData(timestamp, tag.timestamp, instr, doc);
  }
}

void PriceAdapter::onTickerResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata)
{
}

void PriceAdapter::onTradesResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata)
{
}


void PriceAdapter::onKlineResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata)
{
  using namespace pjson;
  auto instr = m_exchangeDictionary.instrumentFromExchange(symbol);
  unsigned long interval = reinterpret_cast<RequestedInterval*>(userdata)->interval;
  delete userdata;

  document doc;
  doc.deserialize_in_place(const_cast<char*>(data.c_str()));

  fin::CandleStickEntry entry;

  for (unsigned int i = 0; i < doc.size(); i ++) {
    const auto &cs = doc[i];
    entry.instrument = instr;
    entry.timestamp = cs[0].as_int64();
    entry.interval = (intptr_t)userdata;
    entry.open.assign(cs[1].as_double());
    entry.high.assign(cs[2].as_double());
    entry.low.assign(cs[3].as_double());
    entry.close.assign(cs[4].as_double());
    entry.volume.assign(cs[5].as_double());

    addCandleStickEntry(std::move(entry));
  }
}

void PriceAdapter::onHTTPError(std::string symbol, const platform::HttpResponse *r, connector::example::RequestContext *userdata)
{
  try {
    // Try to repeat???
    throw std::runtime_error("Connector HTTP error");
  } catch(std::exception &e) {
    setConnectorError(std::current_exception());
  }
}

void PriceAdapter::onTimeout(std::string symbol, connector::example::RequestContext *userdata)
{
  try {
    throw std::runtime_error("HTTP response timeout");
  } catch(std::exception &e) {
    setConnectorError(std::current_exception());
  }
}

/**
 * Implementation of the fetchStack method of the interface
 * @param symbol The handle to the instrument to get DoM for
 * @see fin::interface::StockDataConnector::fetchStack(fin::InstrumentHandle)
 */
void PriceAdapter::fetchStack(fin::InstrumentHandle symbol)
{
  auto instrument = m_exchangeDictionary.instrumentToExchange(symbol);
  getDepth(instrument, 200);
}

/**
 * Implementation of the fetchCandleSticks method of the interface
 * @param symbol The handle to the instrument to get candlesticks for
 * @param interval The interval for candlesticks (in seconds)
 * @param since The starting timestamp (in seconds)
 * @see fin::interface::StockDataConnector::fetchCandleSticks(fin::InstrumentHandle, long, long)
 */
void PriceAdapter::fetchCandleSticks(fin::InstrumentHandle symbol, long interval, long since)
{
  auto instrument = m_exchangeDictionary.instrumentToExchange(symbol);
  std::string suffix = "min";
  int number = 0;
  std::vector<int> minutes({1, 3, 5, 15, 30});
  std::vector<int> hours({1, 2, 4, 6, 12});
  std::string type;

  for(auto minute : minutes) {
    if(interval <= minute * 60) {
      number = minute;
      break;
    }
  }
  if(!number) {
    interval /= 60;
    suffix = "hour";
    for(auto hour : hours) {
      if(interval <= hour * 60) {
        number = hour;
        type = boost::lexical_cast<std::string>(number) + suffix;
        break;
      }
    }
  }

  if(!number) {
    interval /= 60; // hours
    suffix = "day";
    if(interval <= 24) {
      type = suffix;
      number = 1;
    }
    else {
      interval /= 24; // days
      suffix = "week";
      type = suffix;
      number = 1;
      
      if(interval > 7) {
        return;
      }
    }
  }

  getKline(instrument, type.c_str(), 200, since, new RequestedInterval(interval));
}

/**
 * Implementation of the fetchSymbols method of the interface
 * @see fin::interface::StockDataConnector::fetchSymbols()
 */
void PriceAdapter::fetchSymbols()
{
  auto symbols = fin::InstrumentRegistry::instance().getSymbols();
  for(auto sym : symbols) {
    addSymbol(sym);
  }
}

/**
 * Implementation of the fetchInstruments method of the interface
 * @see fin::interface::StockDataConnector::fetchInstruments()
 */
void PriceAdapter::fetchInstruments()
{
  auto instruments = fin::InstrumentRegistry::instance().getInstruments();
  for(auto instrument : instruments) {
    addInstrument(instrument);
  }
}

/**
 * Implementation of the config method of the interface
 * @see fin::interface::StockDataConnector::config(std::string)
 */
void PriceAdapter::config(std::string conf)
{
  using namespace pjson;
  document doc;
  doc.deserialize_in_place(const_cast<char*>(conf.c_str()));

  if(doc.has_key("dictionary")) {
    const auto &dictionary = doc["dictionary"];

    for (unsigned int i = 0; i < dictionary.get_object().size(); i ++) {
      const auto exchInstrument = dictionary.get_object()[i].get_key().get_ptr(); 
      const auto &symbolA = dictionary.get_object()[i].get_value()[0].as_string_ptr();
      const auto &symbolB = dictionary.get_object()[i].get_value()[1].as_string_ptr();

      m_exchangeDictionary.addInstrumentTranslation(exchInstrument, 
          fin::InstrumentRegistry::instance().findInstrument(symbolA, symbolB));
    }
  }
}

/**
 * Implementation of the start method of the interface
 * @see fin::interface::StockDataConnector::start()
 */
void PriceAdapter::start()
{
  try {
    WSPriceConnector::start();
    fin::InstrumentsList subscriptions;
    {
    std::lock_guard<std::mutex> lock(m_subscriptionLock);
    m_subscriptions.swap(subscriptions);
    }
    subscribe(subscriptions);
    m_started = true;
  } catch(std::exception &e) {
    setConnectorError(std::current_exception());
  }
}

/**
 * Implementation of the start method of the interface
 * @see fin::interface::StockDataConnector::stop()
 */
void PriceAdapter::stop()
{
  WSPriceConnector::stop();
}

void PriceAdapter::onData(const char *msg, size_t size, unsigned long netTime)
{
  using namespace pjson;

  if(size) {
    const_cast<char*>(msg)[size] = '\0';
  }

  document doc;
  doc.deserialize_in_place(const_cast<char*>(msg));

  if(!doc.is_array()) {
    return;
  }

  for(unsigned int n = 0; n < doc.size(); n ++) {
    if(!doc[n].has_key("channel") || !doc[n].has_key("data")) {
      continue;
    }

    auto &data = doc[n]["data"];

    if(!data.is_object()) {
      continue;
    }

    const char *name = doc[n].as_string_ptr("channel");
    char *divisor = const_cast<char*>(strrchr(name, '_'));

    if(!divisor || (divisor - name) < 12) {
      return;
    }

    name += 12;
    *divisor = 0;

    fin::InstrumentHandle instrument = m_exchangeDictionary.instrumentFromExchange(name);

    if(instrument == fin::NoInstrument) {
      continue;
    }

    parseData(netTime / 1000, netTime, instrument, data);
  }
}

void PriceAdapter::parseData(long timestamp, unsigned long netTimestamp, fin::InstrumentHandle instr, const pjson::value_variant &data) {
  if(instr != fin::NoInstrument) {
    fin::OrderBookList entries;

    if (data.has_key("asks")) {
      auto &arr = data["asks"];
      processDirection(timestamp, fin::OrderDir::Ask, arr, instr, std::back_inserter(entries));
    }
    if (data.has_key("bids")) {
      auto &arr = data["bids"];
      processDirection(timestamp, fin::OrderDir::Bid, arr, instr, std::back_inserter(entries));
    }
    fin::ProfilingTag tag(netTimestamp);
    addOrderbookBulk(std::move(entries), tag);
  }
}

void PriceAdapter::onClose()
{
  platform::LogInfo() << "Connection closing";
  invalidateData();
  if(m_started) {
    try {
      throw std::runtime_error("ZB connector remote connection closed!");
    } catch(std::exception &e) {
      setConnectorError(std::current_exception());
    }
  }
}

void PriceAdapter::onPingTimeout()
{
  platform::LogError() << "Ping timeout!";
  invalidateData();
  try {
    throw std::runtime_error("OKex connector ping timeout!");
  } catch(std::exception &e) {
    setConnectorError(std::current_exception());
  }
}

}
}
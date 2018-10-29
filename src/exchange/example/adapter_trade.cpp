#include <string.h>
#include <memory.h>
#include <platform/log.h>
#include <pjson.h>
#include "adapter_trade.h"

namespace adaptor {
namespace example {

class StringLine 
  : public std::string { 
  friend std::istream &operator>>(std::istream &is, StringLine &line) {   
    return std::getline(is, line, '\r');
  }

public:
  typedef std::istream_iterator<StringLine> Iterator;
};

using namespace pjson;

TradeAdapter::TradeAdapter(fin::interface::TradeExchangeObserver *o)
  : BaseTradeExchangeConnector(o)
  , m_orderResponseTimeout(2000)
  , m_lock(ATOMIC_FLAG_INIT)
  , m_started(false)
{
  for(size_t i = 0; i < (size_t)OrderChannelType::order_channel_last; i ++) {
    m_ordersQueue.insert(std::make_pair((OrderChannelType)i, OrdersQueue()));
  }
}

bool TradeAdapter::placeOrder(fin::TradeOrderHandle order) {
  const char *instrument = m_exchangeDictionary.instrumentToExchange(order->getInstrument());
  std::string price = order->getPrice().toString();
  std::string amount = order->getPrice().toString();
  const char *type;

  if(!instrument) {
    return false;
  }

  if(order->getExecutionType() == fin::ExecutionType::MARKET)
    type = (order->getDirection() == fin::OrderDir::Bid ? "buy_market" : "sell_market");
  else
    type = (order->getDirection() == fin::OrderDir::Bid ? "buy" : "sell");

  lock();
  addOrderRequest(OrderChannelType::ok_spot_order, order);
  WSTradeConnector::placeOrder(instrument, type, price.c_str(), amount.c_str());
  unlock();
  return true;
}

bool TradeAdapter::cancelOrder(fin::TradeOrderHandle order) {
  const char *instrument = m_exchangeDictionary.instrumentToExchange(order->getInstrument());

  if(!instrument) {
    return false;
  }

  if(order->getStatus().getOrderId() == "") {
    return false;
  }
  lock();
  addOrderRequest(OrderChannelType::ok_spot_cancel_order, order);
  WSTradeConnector::cancelOrder(instrument, order->getStatus().getOrderId().c_str());
  unlock();
  return true;
}

bool TradeAdapter::getOrderStatus(fin::TradeOrderHandle order) {
  const char *instrument = m_exchangeDictionary.instrumentToExchange(order->getInstrument());

  if(!instrument) {
    return false;
  }

  if(order->getStatus().getOrderId() == "") {
    return false;
  }
  lock();
  addOrderRequest(OrderChannelType::ok_spot_orderinfo, order);
  WSTradeConnector::getOrderInfo(instrument, order->getStatus().getOrderId().c_str());
  unlock();
  return true;
}

bool TradeAdapter::getOrdersList() {
  //getTrades(const char *symbol);
  return true;
}

bool TradeAdapter::getBalance() {
  getUserAccountInfo();
  //getBalance(const char *symbol);
  return true;
}

void TradeAdapter::config(std::string conf) {
  using namespace pjson;
  document doc;
  doc.deserialize_in_place(const_cast<char*>(conf.c_str()));

  if(doc.has_key("dictionary")) {
    const auto &dictionary = doc["dictionary"];

    for (unsigned int i = 0; i < dictionary.get_object().size(); i ++) {
      if(dictionary.get_object()[i].get_value().is_array()) {
        const auto exchInstrument = dictionary.get_object()[i].get_key().get_ptr(); 
        const auto &symbolA = dictionary.get_object()[i].get_value()[0].as_string_ptr();
        const auto &symbolB = dictionary.get_object()[i].get_value()[1].as_string_ptr();

        m_exchangeDictionary.addInstrumentTranslation(exchInstrument, 
            fin::InstrumentRegistry::instance().findInstrument(symbolA, symbolB));
        const auto instrument = fin::InstrumentRegistry::instance().findInstrument(symbolA, symbolB);
        addSymbol(instrument->first);
        addSymbol(instrument->second);
      } else {
        const auto exchSymbol = dictionary.get_object()[i].get_key().get_ptr(); 
        const auto &symbol = dictionary.get_object()[i].get_value().as_string_ptr();
        m_exchangeDictionary.addSymbolTranslation(exchSymbol, 
            fin::InstrumentRegistry::instance().findSymbol(symbol));
      }
    }
  }

  if(doc.has_key("limits-url")) {
    m_limitsUrl = doc["limits-url"].as_string_ptr();
  }

  if(doc.has_key("api-key")) {
    setAPIKey(doc["api-key"].as_string_ptr());
  }

  if(doc.has_key("secret")) {
    setSecretKey(doc["secret"].as_string_ptr());
  }
}

void TradeAdapter::start() {
  try {
    m_started = true;
    fetchLimits();
    WSTradeConnector::start();
    usleep(10); // Give a moment to finish connection
  } catch(std::exception &e) {
    setConnectorError(std::current_exception());
  }
}

void TradeAdapter::stop() {
  m_started = false;
  WSTradeConnector::stop();
}

void TradeAdapter::onLoginResponse(const char *data, size_t size, unsigned long timestamp) {
  const_cast<char*>(data)[size] = '\0';
  pjson::document doc;
  bool result = parseResponse(const_cast<char*>(data), doc);
  bool loginFailed = false;

  if(doc[0]["data"].has_key("error")) {
    loginFailed = true;
  }

  if(!result || loginFailed) {
    try {
      throw std::runtime_error("Login failed!");
    } catch(std::exception &e) {
      setConnectorError(std::current_exception());
    }
  }
}

void TradeAdapter::onOrderPlacedResponse(const char *data, size_t size, unsigned long timestamp) {
  fin::TradeOrderHandle order = removeOrderRequest(OrderChannelType::ok_spot_order);
  const_cast<char*>(data)[size] = '\0';
  pjson::document doc;
  bool result = parseResponse(const_cast<char*>(data), doc);

  if(!result) {
    auto newStatus(order->getStatus());
    newStatus.setState(fin::OrderStatus::Failed);
    if(doc.is_array() && doc[0].is_object()) {
      auto &data = doc[0]["data"];
      auto newStatus(order->getStatus());
      if(data.has_key("error_code")) {
        platform::LogError() << "Order placement failed: " << *order
                             << "\nError reason: " << data["error_code"].as_int32();
      }
    }
    updateOrderStatus(order, newStatus, fin::ProfilingTag(timestamp));
    return;
  }

  for(size_t i = 0; i < doc.size(); i ++) {
    if(!doc[i].is_object() || !doc[i].has_key("data")) {
      continue;
    }
    auto &data = doc[i]["data"];
    auto newStatus(order->getStatus());
    newStatus.setState(fin::OrderStatus::Placed);
    newStatus.setOrderId(data["order_id"].as_string_ptr());
    if(order->getExecutionType() == fin::ExecutionType::IOC) {
      // Emulate IOC - try to cancel placed order
      cancelOrder(order);
    } else {
      updateOrderStatus(order, newStatus, fin::ProfilingTag(timestamp));
    }
  }
}

void TradeAdapter::onOrderCancelledResponse(const char *data, size_t size, unsigned long timestamp) {
  fin::TradeOrderHandle order = removeOrderRequest(OrderChannelType::ok_spot_cancel_order);
  const_cast<char*>(data)[size] = '\0';
  pjson::document doc;
  bool result = parseResponse(const_cast<char*>(data), doc);

  if(!result) {
    // Order cancel failed
    if(doc.is_array() && doc[0].is_object()) {
      auto &data = doc[0]["data"];
      auto newStatus(order->getStatus());
      if(data.has_key("error_code") && (data["error_code"].as_int32() == 1050)) {
        // Tried to cancel filled or cancelled order
        if(newStatus.getState() == fin::OrderStatus::Cancelled) {
          // Do not change, tried to cancel cancelled order
          return;
        }
        // Change to filled, tried to cancel filled order
        newStatus.setState(fin::OrderStatus::Filled);
      } else {
        // We don't know what the error was
        newStatus.setState(fin::OrderStatus::Unknown);
      }
      updateOrderStatus(order, newStatus, fin::ProfilingTag(timestamp));
    }
    return;
  }

  // Order cancel succeeded
  for(size_t i = 0; i < doc[i].size(); i ++) {
    auto newStatus(order->getStatus());

    newStatus.setState(fin::OrderStatus::Cancelled);
    newStatus.setCancelledTimestamp(timestamp / 1000000); // nanoseconds to milliseconds

    // Emulated IOC - if cancel succeeds, the order was not filled
    if(order->getExecutionType() == fin::ExecutionType::IOC) {
      newStatus.setCancelledTimestamp(timestamp / 1000000);
      updateOrderStatus(order, newStatus, fin::ProfilingTag(timestamp));
    }
  }
}

void TradeAdapter::onUserAccountInfoResponse(const char *data, size_t size, unsigned long timestamp) {
  // ?
  const_cast<char*>(data)[size] = '\0';
  pjson::document doc;
  bool result = parseResponse(const_cast<char*>(data), doc);

  if(!result) {
    // Update balance failed
    return;
  }

  for(size_t i = 0; i < doc.size(); i ++) {
    if(!doc[i].is_object()) {
      platform::LogError() << "OKex response error: " << std::string(data, size);
      continue;
    }
    auto &data = doc[i]["data"];
    if(data.has_key("info")) {
      auto &info = data["info"];
      if(info.has_key("funds")) {
        auto &funds = info["funds"];
        if(funds.has_key("free")) {
          auto &free = funds["free"];
          for(size_t i = 0; i < free.get_object().size(); i ++) {
            std::string name(free.get_object()[i].get_key().get_ptr()); 
            fin::FixedNumber value(free[name.c_str()].as_string_ptr()); 
            fin::SymbolHandle symbol = m_exchangeDictionary.symbolFromExchange(name.c_str());
            if(symbol) {
              updateBalance(symbol, value, fin::ProfilingTag(timestamp));
            }
          }
        }
      }
    }
  }
}

void TradeAdapter::onOrderInfoResponse(const char *data, size_t size, unsigned long timestamp) {
  // Order info changed
  const_cast<char*>(data)[size] = '\0';
  pjson::document doc;
  bool result = parseResponse(const_cast<char*>(data), doc);

  std::map<int, fin::OrderStatus::State> statuses = {
    {-1, fin::OrderStatus::Cancelled},
    {0, fin::OrderStatus::Placed},
    {1, fin::OrderStatus::PartialFilled},
    {2, fin::OrderStatus::Filled},
    {4, fin::OrderStatus::PartialCancelled}
  };

  if(!result) {
    platform::LogError() << "OKex order info request failed";
    return;
  }

  for(size_t i = 0; i < doc.size(); i ++) {
    if(!doc[i].is_object() || !doc[i].has_key("data")) {
      continue;
    }
    auto &data = doc[i]["data"];
    if(!data.is_object() || !data.has_key("orders")) {
      continue;
    }

    auto &orders = data["orders"];
    for(size_t o = 0; o < orders.size(); o ++) {
      fin::TradeOrderHandle order = removeOrderRequest(OrderChannelType::ok_spot_orderinfo);
      if(!order) {
        platform::LogError() << "OKex order info desync: no order info requests pending, but received order info response";
        break; // We have no order info requests pending
      }

      if(!orders[o].is_object()) {
        continue;
      }

      std::string orderId = orders[o]["order_id"].as_string_ptr();
      std::string avgPrice(orders[o]["avg_price"].as_string_ptr());
      std::string dealAmount(orders[o]["deal_amount"].as_string_ptr());
      int status = orders[o]["status"].as_int32();

      if(statuses.find(status) == statuses.end()) {
        platform::LogWarning() << "OKex order status " << status << " can not be mapped to one of the internal statuses";
        continue;
      }

      if(order->getStatus().getOrderId() != orderId) {
        platform::LogWarning() << "OKex order info desync: got order ID " << orderId << ", pending order ID is " << order->getStatus().getOrderId();
        continue;
      }

      auto newStatus(order->getStatus());
      newStatus.setState(statuses[status]);
      newStatus.setFilledPrice(avgPrice);
      newStatus.setFilledAmount(dealAmount);
      updateOrderStatus(order, newStatus, fin::ProfilingTag(timestamp));
    }
  }
}

void TradeAdapter::onBalanceResponse(const char *data, size_t size, unsigned long timestamp) {
  // Do nothing, we handle balances in another function
}

void TradeAdapter::onTradesResponse(const char *data, size_t size, unsigned long timestamp) {
}

void TradeAdapter::onPingTimeout() {
  try {
    throw std::runtime_error("OKex trading connector ping timeout");
  } catch(...) {
    setConnectorError(std::current_exception());
  }
}

void TradeAdapter::onResponseTimeout() {
  try {
    throw std::runtime_error("OKex trading connector response timeout");
  } catch(...) {
    setConnectorError(std::current_exception());
  }
}

void TradeAdapter::onClose() {
  if(m_started) {
    try {
      throw std::runtime_error("OKex trading connector connection closed");
    } catch(...) {
      setConnectorError(std::current_exception());
    }
  }
}

bool TradeAdapter::parseResponse(char *str, document &doc) {
  doc.deserialize_in_place(str);

  if(!doc.is_array()) {
    return false;
  }

  for(size_t i = 0; i < doc.size(); i ++) {
    if(!doc[i].is_object()) {
      return false;
    }

    if(doc[i].has_key("data")) {
      auto &data = doc[i]["data"];
      if(data.is_object() && data.has_key("result")) {
        auto &result = data["result"];
        if(result.is_bool()) {
          return result.as_bool();
        }
      }

      if(data.has_key("error_code")) {
        if(data["error_code"].is_int()) {
          return false;
        }
      }
    }
  }

  return false;
}

void TradeAdapter::addOrderRequest(OrderChannelType channel, const fin::TradeOrderHandle &order) {
  m_ordersQueue[channel].push(order);
}

void TradeAdapter::orderResponseTimeout() {
}

fin::TradeOrderHandle TradeAdapter::removeOrderRequest(OrderChannelType channel) {
  fin::TradeOrderHandle order;
  lock();
  if(m_ordersQueue[channel].empty()) {
    platform::LogError() << "OKex: request desync";
  } else {
    order = m_ordersQueue[channel].front();
    m_ordersQueue[channel].pop();
  }
  unlock();
  return order;
}

void TradeAdapter::onReadyState(const platform::HttpResponse *response) {
  std::string limits = response->getBody();
  std::stringstream input(limits);
  fin::TradeExchangeConstraints tradeLimits;
  std::vector<std::string> values;
  bool header = true;

  for(StringLine::Iterator line(input); line != StringLine::Iterator(); ++ line) {
    // Skip header
    if(header) {
      header = false;
      continue;
    }

    size_t ptr = line->find('\n');

    if(ptr != std::string::npos) {
      ptr ++;
    } else {
      ptr = 0;
    }

    do {
      size_t endptr = line->find(',', ptr);
      if(endptr == std::string::npos) {
        endptr = line->size();
      }

      values.push_back(std::string(&(*line)[ptr], &(*line)[endptr]));
      ptr = endptr + 1;
    } while(ptr < line->size());

    fin::InstrumentHandle instrument = m_exchangeDictionary.instrumentFromExchange(values[1].c_str());
    if(instrument) {
      tradeLimits.amountMin.assign(values[2]);
      tradeLimits.amountQuantum.assign(values[3]);
      tradeLimits.priceQuantum.assign(values[4]);
      addConstraints(instrument, tradeLimits);
    }
    values.resize(0);
  }
}

void TradeAdapter::fetchLimits() {
  using namespace platform;

  if(!m_limitsUrl.empty()) {
    HttpRequest *request = HttpClient::instance().createRequest(m_limitsUrl);
    request->setReadyStateHandler(this);
    request->setResponseTimeout(2000);
    request->send();
  }
}

}
}

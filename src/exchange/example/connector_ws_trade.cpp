#include <stdio.h>
#include <future>
#include <map>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <fin/profiling.h>
#include <platform/log.h>
#include <platform/sign_util.h>
#include "config.h"
#include "connector_ws_trade.h"

namespace connector {
namespace example {

using namespace platform;

//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Trade Websocket API //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

WSTradeConnector::WSTradeConnector()
  : m_pingTimer(TimerService::instance().createTimer([this](Timer*){ this->pingTimeout(); }))
  , m_pingTimeout(1000)
  , m_pingInterval(30000)
  , m_lastPing(0)
{
  setUrl(connector::example::WS_URL);
}

WSTradeConnector::~WSTradeConnector() {
}

void WSTradeConnector::setPingTimeout(long pingTimeoutMs) {
  m_pingTimeout = pingTimeoutMs;
}

void WSTradeConnector::setPingInterval(long pingIntervalMs) {
  m_pingInterval = pingIntervalMs;
}

void WSTradeConnector::setAPIKey(const std::string &apiKey) {
  m_apiKey = apiKey;
}

void WSTradeConnector::setSecretKey(const std::string &secretKey) {
  m_secretKey = secretKey;
}

void WSTradeConnector::start() {
  using namespace std::placeholders;
  m_started = true;
  m_wsConnection.reset(WebSocketClient::instance().createConnection());
  m_wsConnection->setConnectionHandler(this);

  // We want to wait until connection is completed
  m_connected = std::promise<bool>();

  m_wsConnection->connect(m_url.c_str());

  bool success = m_connected.get_future().get();

  if(!success) {
    throw std::runtime_error("Connect to Okex WS API failed! (url: " + m_url + ").");
  }
}

void WSTradeConnector::stop() {
  if(!m_started) {
    return;
  }

  m_started = false;
  try {
    // Clear waiting future in case connect operation is in progress
    m_connected.set_value(false);
  } catch(std::future_error &e)
  { }

  m_wsConnection.reset();
}

void WSTradeConnector::ping() {
  static const std::string message = "{\"event\":\"ping\"}";

  if(!m_wsConnection) {
    return; // Throw?
  }

  m_pingTimer->start(std::chrono::milliseconds(m_pingTimeout));
  m_wsConnection->write(message.c_str(), message.size());
}

bool WSTradeConnector::login() {
  static const char *requestFormat = "{\"event\":\"login\",\"parameters\":%s}";
  std::map<std::string, const char*> params;

  params.emplace(std::make_pair("api_key", m_apiKey.c_str()));
  return sendRequest(requestFormat, params);
}

bool WSTradeConnector::placeOrder(const char *symbol, const char *type, const char *price, const char *amount) {
  static const char *requestFormat = "{\"event\":\"addChannel\",\"channel\":\"ok_spot_order\",\"parameters\":%s}";
  std::map<std::string, const char*> params;

  params.emplace(std::make_pair("api_key", m_apiKey.c_str()));
  params.emplace(std::make_pair("symbol", symbol));
  params.emplace(std::make_pair("type", type));
  params.emplace(std::make_pair("price", price));
  params.emplace(std::make_pair("amount", amount));
  return sendRequest(requestFormat, params);
}

bool WSTradeConnector::cancelOrder(const char *symbol, const char *orderId) {
  static const char *requestFormat = "{\"event\":\"addChannel\",\"channel\":\"ok_spot_cancel_order\",\"parameters\":%s}";
  std::map<std::string, const char*> params;

  params.emplace(std::make_pair("api_key", m_apiKey.c_str()));
  params.emplace(std::make_pair("symbol", symbol));
  params.emplace(std::make_pair("order_id", orderId));
  return sendRequest(requestFormat, params);
}

bool WSTradeConnector::getUserAccountInfo() {
  static const char *requestFormat = "{\"event\":\"addChannel\",\"channel\":\"ok_spot_userinfo\",\"parameters\":%s}";
  std::map<std::string, const char*> params;

  params.emplace(std::make_pair("api_key", m_apiKey.c_str()));
  return sendRequest(requestFormat, params);
}

bool WSTradeConnector::getOrderInfo(const char *symbol, const char *orderId) {
  static const char *requestFormat = "{\"event\":\"addChannel\",\"channel\":\"ok_spot_cancel_order\",\"parameters\":%s}";
  std::map<std::string, const char*> params;

  params.emplace(std::make_pair("api_key", m_apiKey.c_str()));
  params.emplace(std::make_pair("symbol", symbol));
  params.emplace(std::make_pair("order_id", orderId));
  return sendRequest(requestFormat, params);
}

bool WSTradeConnector::getTrades(const char *symbol) {
  static const char *requestFormat = "{\"event\":\"addChannel\",\"channel\":\"ok_sub_spot_X_order\",\"parameters\":%s}";
  std::map<std::string, const char*> params;

  params.emplace(std::make_pair("api_key", m_apiKey.c_str()));
  params.emplace(std::make_pair("symbol", symbol));
  return sendRequest(requestFormat, params);
}

bool WSTradeConnector::getBalance(const char *symbol) {
  static const char *requestFormat = "{\"event\":\"addChannel\",\"channel\":\"ok_spot_X_balance\",\"parameters\":%s}";
  std::map<std::string, const char*> params;

  params.emplace(std::make_pair("api_key", m_apiKey.c_str()));
  params.emplace(std::make_pair("symbol", symbol));
  return sendRequest(requestFormat, params);
}

void WSTradeConnector::setUrl(const std::string &url) {
  m_url = url;
}

void WSTradeConnector::signParameters(std::string &out, const std::map<std::string, const char*> &params) {
  std::string paramString;
  std::vector<char> signature;

  for(const auto &kv : params) {
    if(!paramString.empty()) {
      paramString += "&";
    }
    paramString += kv.first + "=" + kv.second;
  }

  paramString += "&secret_key=" + m_secretKey;

  signature.reserve(32);
  SignUtil::md5Sign(signature, paramString);
  SignUtil::hex<SignUtil::CAPITAL>(out, signature);
}

void WSTradeConnector::createRequestParameters(std::string &out, const std::map<std::string, const char*> &params) {
  out = "{";
  bool first = true;

  std::string signature;

  signParameters(signature, params);

  for(const auto &kv : params) {
    if(!first) {
      out += ",";
    } else {
      first = false;
    }
    out += "\"" + kv.first + "\":\"" + kv.second + "\"";
  }

  out += ",\"sign\":\"" + signature + "\"}";
}

bool WSTradeConnector::sendRequest(const char *format, const std::map<std::string, const char*> &params) {
  if(!m_wsConnection) {
    return false; // Throw?
  }

  std::string paramString;
  createRequestParameters(paramString, params);
  char *request = NULL;
  int len = asprintf(&request, format, paramString.c_str());

  if(len >= 0) {
    m_wsConnection->write(request, len);
    free(request);
  }
  return true;
}

const char *WSTradeConnector::strNStr(const char *start, const std::string &str, size_t size) {
  return (const char *)memmem(start, size, str.c_str(), str.size());
}

int WSTradeConnector::onDataReady(platform::WebSocketConnection *, platform::WSMessage msg) {
  using namespace std;
  static string channel = "channel";
  static string event = "event";
  static string login = "login";
  static string ping = "ping";
  static string ok_spot_orderinfo = "ok_spot_orderinfo";
  static string ok_spot_order = "ok_spot_order";
  static string ok_spot_cancel_order = "ok_spot_cancel_order";
  static string ok_spot_userinfo = "ok_spot_userinfo";
  static string _order = "_order";
  static string _balance = "_balance";

  const char *start = strNStr(msg.data, channel.c_str(), msg.size);
  if(!start) {
    start = strNStr(msg.data, event.c_str(), msg.size);
  }

  if(!start) {
    return 0;
  }

  char const *end = strNStr(start, ",", msg.size - (start - msg.data));
  if(!end) {
    end = msg.data + msg.size;
  }

  size_t len = end - start;
  if(strNStr(start, "login", len)) {
    onLogin(msg.data, msg.size, msg.timestamp);
  } else if(strNStr(start, ok_spot_orderinfo, len)) { // order info
    onOrderInfo(msg.data, msg.size, msg.timestamp);
  } else if(strNStr(start, ok_spot_order, len)) { // order placement
    onOrderPlaced(msg.data, msg.size, msg.timestamp);
  } else if(strNStr(start, ok_spot_cancel_order, len)) { // cancel order
    onOrderCancelled(msg.data, msg.size, msg.timestamp);
  } else if(strNStr(start, ok_spot_userinfo, len)) { // user info
    onUserAccountInfo(msg.data, msg.size, msg.timestamp);
  } else if(strNStr(start, _order, len)) { // trade record
    onTrades(msg.data, msg.size, msg.timestamp);
  } else if(strNStr(start, _balance, len)) { // balance
    onBalance(msg.data, msg.size, msg.timestamp);
  } else if(strNStr(start, ping, len)) { // ping
    m_pingTimer->stop();
  }

  m_requestTimestamps.pop();

  return 0;
}

void WSTradeConnector::onLogin(const char *data, size_t size, unsigned long timestamp) {
  onLoginResponse(data, size, timestamp);
}

void WSTradeConnector::onOrderPlaced(const char *data, size_t size, unsigned long timestamp) {
  onOrderPlacedResponse(data, size, timestamp);
}

void WSTradeConnector::onOrderCancelled(const char *data, size_t size, unsigned long timestamp) {
  onOrderCancelledResponse(data, size, timestamp);
}

void WSTradeConnector::onUserAccountInfo(const char *data, size_t size, unsigned long timestamp) {
  onUserAccountInfoResponse(data, size, timestamp);
}

void WSTradeConnector::onOrderInfo(const char *data, size_t size, unsigned long timestamp) {
  onOrderInfoResponse(data, size, timestamp);
}

void WSTradeConnector::onBalance(const char *data, size_t size, unsigned long timestamp) {
  onBalanceResponse(data, size, timestamp);
}

void WSTradeConnector::onTrades(const char *data, size_t size, unsigned long timestamp) {
  onTradesResponse(data, size, timestamp);
}

void WSTradeConnector::onPong(const char *data, size_t size, unsigned long timestamp) {
  m_pingTimer->stop();
}

int WSTradeConnector::onConnected(platform::WebSocketConnection *) {
  // Set promise and release waiting threads
  std::queue<unsigned long> newQueue;
  m_requestTimestamps.swap(newQueue);
  m_requestTimeoutTriggered = false;
  m_connected.set_value(true);
  return 0;
}

int WSTradeConnector::onConnectFailed(platform::WebSocketConnection *) {
  // Set promise and release waiting threads
  m_connected.set_value(false);
  m_started = false;
  return 0;
}

int WSTradeConnector::onClose(platform::WebSocketConnection *) {
  if(m_started) {
    m_wsConnection->disconnect(); // Tear down the connection
    m_started = false;
    std::queue<unsigned long> newQueue;
    m_requestTimestamps.swap(newQueue);
  }
  onClose();
  return 0;
}

int WSTradeConnector::onMessageSent(platform::WebSocketConnection *, platform::WSMessage msg) {
  m_requestTimestamps.push(msg.timestamp);
  return 0;
}

void WSTradeConnector::checkTimers() {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  unsigned long ts = now.tv_sec * 1000000000 + now.tv_nsec;

  if((ts - m_lastPing) / 1000000 > (unsigned long)m_pingInterval) {
    ping();
    m_lastPing = ts;
  }

  if(!m_requestTimeoutTriggered && !m_requestTimestamps.empty()) {
    m_requestTimeoutTriggered = true;
    if((ts - m_requestTimestamps.front()) > (unsigned long)m_responseTimeout) {
      this->onResponseTimeout();
    }
  }
}

void WSTradeConnector::pingTimeout() {
  onPingTimeout();
}

}
}

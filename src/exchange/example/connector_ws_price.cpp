#include <future>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <platform/log.h>
#include <fin/profiling.h>
#include "config.h"
#include "connector_ws_price.h"

namespace connector {
namespace example {

using namespace platform;

//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// OKex Spot Price Websocket API //////////////////////////////// 
//////////////////////////////////////////////////////////////////////////////////////////////

WSPriceConnector::WSPriceConnector()
  : m_pingTimer(TimerService::instance().createTimer([this](Timer*){ this->pingTimeout(); }))
  , m_pingTimeout(2500)
  , m_dataTimeout(1000)
  , m_pingSent(0)
  , m_pingInterval(30000000000)
  , m_lastData(0)
{
  setUrl(connector::example::WS_URL);
}

WSPriceConnector::~WSPriceConnector() {
  if(m_started) {
    stop();
  }
}

/**
 * Sets WebSocket connection URL
 * @param url URL to connect to
 */
void WSPriceConnector::setUrl(const std::string &url)
{
  m_url = url;
}

/**
 * Sets response timeout to ping request
 * @param pingTimeout timeout in milliseconds
 */
void WSPriceConnector::setPingTimeout(long pingTimeout)
{
  m_pingTimeout = pingTimeout;
}

/**
 * When no data is received for given time issues a ping request
 * @param timeout timeout in milliseconds
 */
void WSPriceConnector::setDataTimeout(long timeout)
{
  m_dataTimeout = timeout;
}

/**
 * Starts WebSocket connection and waits until it succeeds or fails.
 * Throws std::runtime_error if connect fails.
 */
void WSPriceConnector::start()
{
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

/**
 * Send ping request to test if connection is alive
 * @see https://github.com/okcoin-okex/API-docs-OKEx.com/blob/master/API-For-Spot-EN/WEBSOCKET%20API%20for%20SPOT.md#how-to-know-whether-connection-is-lost
 */
void WSPriceConnector::ping() {
  static const std::string message = "{\"event\":\"ping\"}";

  if(!m_wsConnection) {
    return; // Throw?
  }

  m_wsConnection->write(message.c_str(), message.size());

  // Start ping timeout timer
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  unsigned long ts = now.tv_sec * 1000000000 + now.tv_nsec;
  m_pingSent = ts;
  m_pingTimer->start(std::chrono::milliseconds(m_pingTimeout));
}

void WSPriceConnector::pingTimeout() {
  onPingTimeout();
}

void WSPriceConnector::dataTimeout() {
  ping();
}

int WSPriceConnector::onDataReady(WebSocketConnection *conn, WSMessage msg) {
  if(msg.data && !strncmp(msg.data, "{\"event\":\"pong\"}", msg.size)) {
    // Ping response received, clear timeout
    m_pingTimer->stop();
    return 0;
  }

  //m_dataTimer->start(m_dataTimeoutDuration);
  onData(msg.data, msg.size, msg.timestamp);
  m_lastData = msg.timestamp;

  return 0;
}

void WSPriceConnector::checkTimers() {
  if((m_lastData == 0) || (m_pingSent >= m_lastData)) {
    return;
  }

  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  unsigned long ts = now.tv_sec * 1000000000 + now.tv_nsec;
  if((ts - m_lastData) / 1000000 > (unsigned long)m_dataTimeout) {
    dataTimeout();
    m_lastData = ts;
  } else if((m_pingSent + m_pingInterval * 1000000) >= ts) {
    ping();
  }
}

int WSPriceConnector::onConnected(WebSocketConnection *conn) {
  // Set promise and release waiting threads
  m_connected.set_value(true);
  return 0;
}

int WSPriceConnector::onConnectFailed(WebSocketConnection *conn) {
  // Set promise and release waiting threads
  m_connected.set_value(false);
  m_started = false;
  return 0;
}

int WSPriceConnector::onClose(WebSocketConnection *conn) {
  if(m_started) {
    m_wsConnection->disconnect(); // Tear down the connection
    m_started = false;
    m_subscribed = false;
  }
  onClose();
  return 0;
}

/**
 * Drops connection to WebSocket server and stop the connector
 */
void WSPriceConnector::stop() {
  if(!m_started) {
    return;
  }

  m_started = false;
  m_subscribed = false;
  try {
    // Clear waiting future in case connect operation is in progress
    m_connected.set_value(false);
  } catch(std::future_error &e)
  { }

  m_wsConnection.reset();
}

/**
 * Subscribes to the given instrument
 * @param instrument - trading symbol in OKex format (usdt_btc, qtum_usdt, hsr_usdt, etc.)
 * @see https://github.com/okcoin-okex/API-docs-OKEx.com/blob/master/API-For-Spot-EN/WEBSOCKET%20API%20for%20SPOT.md#spot-price-api
 */
void WSPriceConnector::subscribe(const char *instrument) {
  static const char format[] = "{\"event\":\"addChannel\",\"channel\":\"ok_sub_spot_%s_depth\"}";
  static const char formatSize = sizeof(format);

  if(!m_wsConnection) {
    return; // Throw?
  }

  size_t len = strlen(instrument) + formatSize + 1;
  char msg[len];

  int msgSize = snprintf(msg, len, format, instrument);

  m_wsConnection->write(msg, msgSize);

  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  m_lastData = now.tv_sec * 1000000000 + now.tv_nsec;
}

}
}
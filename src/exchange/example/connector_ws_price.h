#pragma once

#include <mutex>
#include <atomic>
#include <future>
#include <string>
#include <platform/websocket.h>
#include <platform/http.h>
#include <platform/timer.h>
#include <set>

namespace connector {
namespace example {

/**
 * This class implements OKex WebSocket Spot Price API
   * @see https://github.com/okcoin-okex/API-docs-OKEx.com/blob/master/API-For-Spot-EN/WEBSOCKET%20API%20for%20SPOT.md
   */
class WSPriceConnector 
  : public platform::WebSocketConnectionHandler {
public:
  WSPriceConnector();
  virtual ~WSPriceConnector();

  void setPingTimeout(long pingTimeoutMs); //!< Set timeout for ping response
  void setDataTimeout(long timeoutMs); //!< Set data timeout to issue ping request

  void start(); //!< Open WebSocket connection
  void stop(); //!< Stop WebSocket connection
  void subscribe(const char *instrument); //!< Subscribe to updates for given symbol (in OKex format)
  void ping(); //!< Test if connection is alive

  virtual void onData(const char *data, size_t size, unsigned long timestamp) = 0; //!< Data callback, receives unprocessed events and market updates
  virtual void onClose() = 0; //!< Connection close callback, called when connection tears down
  virtual void onPingTimeout() = 0; //!< Called after a timeout, when we do not get pong response

private:
  void setUrl(const std::string &url); //!< Set URL for WebSocket connection (can be either OKex or OKcoin URL)
  WSPriceConnector(const WSPriceConnector &) = delete;
  void operator =(const WSPriceConnector &) = delete;

  virtual int onDataReady(platform::WebSocketConnection *, platform::WSMessage msg) override;
  virtual int onConnected(platform::WebSocketConnection *) override;
  virtual int onConnectFailed(platform::WebSocketConnection *) override;
  virtual int onClose(platform::WebSocketConnection *) override;
  virtual void checkTimers() override;
  void pingTimeout();
  void dataTimeout();

  std::promise<bool> m_connected;
  std::atomic<bool>  m_started;
  std::atomic<bool>  m_subscribed;
  std::unique_ptr<platform::WebSocketConnection> m_wsConnection;
  std::unique_ptr<platform::Timer> m_pingTimer;
  long m_pingTimeout;
  long m_dataTimeout;
  unsigned long m_pingSent;
  long m_pingInterval;
  unsigned long m_lastData;
  std::string m_url;
};

}
}
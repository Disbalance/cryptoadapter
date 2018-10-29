#pragma once

#include <atomic>
#include <future>
#include <queue>
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
class WSTradeConnector 
  : public platform::WebSocketConnectionHandler {
public:
  WSTradeConnector();
  virtual ~WSTradeConnector();

  void setPingTimeout(long pingTimeoutMs); //!< Set timeout for ping response
  void setPingInterval(long pingTimeoutMs); //!< Set interval between ping requests - 30 seconds or more is allowed
  void setAPIKey(const std::string &apiKey); //!< Set API key for trading
  void setSecretKey(const std::string &secretKey); //!< Set secret key for signing

  void start(); //!< Open WebSocket connection
  void stop(); //!< Stop WebSocket connection
  void ping(); //!< Test if connection is alive

  bool login();
  bool placeOrder(const char *symbol, const char *type, const char *price, const char *amount);
  bool cancelOrder(const char *symbol, const char *orderId);
  bool getUserAccountInfo();
  bool getOrderInfo(const char *symbol, const char *orderId);
  bool getTrades(const char *symbol);
  bool getBalance(const char *symbol);

  virtual void onLoginResponse(const char *data, size_t size, unsigned long timestamp) = 0; //!< Response to login request
  virtual void onOrderPlacedResponse(const char *data, size_t size, unsigned long timestamp) = 0; //!< Data callback, receives unprocessed events and market updates
  virtual void onOrderCancelledResponse(const char *data, size_t size, unsigned long timestamp) = 0; //!< Data callback, receives unprocessed events and market updates
  virtual void onUserAccountInfoResponse(const char *data, size_t size, unsigned long timestamp) = 0; //!< Data callback, receives unprocessed events and market updates
  virtual void onOrderInfoResponse(const char *data, size_t size, unsigned long timestamp) = 0; //!< Data callback, receives unprocessed events and market updates
  virtual void onBalanceResponse(const char *data, size_t size, unsigned long timestamp) = 0; //!< Data callback, receives unprocessed events and market updates
  virtual void onTradesResponse(const char *data, size_t size, unsigned long timestamp) = 0; //!< Data callback, receives unprocessed events and market updates
  virtual void onPingTimeout() = 0; //!< Called after a timeout, when we do not get pong response
  virtual void onResponseTimeout() = 0; //!< Called after a timeout, when we do not get pong response
  virtual void onClose() = 0; //!< Connection close callback, called when connection tears down

private:
  void setUrl(const std::string &url); //!< Set URL for WebSocket connection (can be either OKex or OKcoin URL)
  void signParameters(std::string &, const std::map<std::string, const char*> &);
  void createRequestParameters(std::string &, const std::map<std::string, const char*> &);
  bool sendRequest(const char *, const std::map<std::string, const char*> &);
  WSTradeConnector(const WSTradeConnector &) = delete;
  void operator =(const WSTradeConnector &) = delete;

  void onLogin(const char *data, size_t size, unsigned long timestamp);
  void onOrderPlaced(const char *data, size_t size, unsigned long timestamp);
  void onOrderCancelled(const char *data, size_t size, unsigned long timestamp);
  void onUserAccountInfo(const char *data, size_t size, unsigned long timestamp);
  void onOrderInfo(const char *data, size_t size, unsigned long timestamp);
  void onBalance(const char *data, size_t size, unsigned long timestamp);
  void onTrades(const char *data, size_t size, unsigned long timestamp);
  void onPong(const char *data, size_t size, unsigned long timestamp);

  const char *strNStr(const char *start, const std::string &str, size_t size);
  virtual int onDataReady(platform::WebSocketConnection *, platform::WSMessage msg) override;
  virtual int onConnected(platform::WebSocketConnection *) override;
  virtual int onConnectFailed(platform::WebSocketConnection *) override;
  virtual int onClose(platform::WebSocketConnection *) override;
  virtual int onMessageSent(platform::WebSocketConnection *, platform::WSMessage msg) override;
  virtual void checkTimers() override;
  void pingTimeout();
  void responseTimeout();

  typedef void (WSTradeConnector::*ResponseCallback)(const char *data, size_t size, unsigned long timestamp);

  std::promise<bool> m_connected;
  std::atomic<bool>  m_started;
  std::unique_ptr<platform::WebSocketConnection> m_wsConnection;
  std::unique_ptr<platform::Timer> m_pingTimer;
  std::unique_ptr<platform::Timer> m_dataTimer;
  long m_pingTimeout;
  long m_responseTimeout;
  long m_pingInterval;
  unsigned long m_lastPing;
  std::string m_url;
  std::string m_apiKey;
  std::string m_secretKey;
  std::queue<unsigned long> m_requestTimestamps;
  bool m_requestTimeoutTriggered;
};

}
}

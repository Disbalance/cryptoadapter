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

//! Base class for user-defined context to keep in response handlers
class RequestContext {
public:
  virtual ~RequestContext() { }
};

/**
 * This class implements ZB HTTP REST API
 * Okex:
 * @see https://www.zb.com/i/developer/restApi#market
 */
class RESTPriceConnector {
public:
  RESTPriceConnector();
  virtual ~RESTPriceConnector() { }

  void setTimeout(long timeoutMs);

  //! Called when the last request times out. Receives symbol passed in the request, and the userdata
  virtual void onTimeout(std::string symbol, RequestContext *userdata) = 0;
  //! Called when HTTP error is detected. Receives symbol passed in the request, and the userdata
  virtual void onHTTPError(std::string symbol, const platform::HttpResponse *r, RequestContext *userdata) = 0;
  //! Called when we receive response to getDepth request
  virtual void onDepthResponse(std::string data, const char *symbol, RequestContext *userdata = nullptr) = 0;
  //! Called when we receive response to getTicker request
  virtual void onTickerResponse(std::string data, const char *symbol, RequestContext *userdata = nullptr) = 0;
  //! Called when we receive response to getTrades request
  virtual void onTradesResponse(std::string data, const char *symbol, RequestContext *userdata = nullptr) = 0;
  //! Called when we receive response to getCandleStick request
  virtual void onKlineResponse(std::string data, const char *symbol, RequestContext *userdata = nullptr) = 0;

  bool getDepth(const char *symbol, int size = 200, RequestContext *userdata = nullptr); //!< Get market depth
  bool getTicker(const char *symbol, RequestContext *userdata = nullptr); //!< Get ticker
  bool getTrades(const char *symbol, long since, RequestContext *userdata = nullptr); //! Get list of trades
  bool getKline(const char *symbol, const char *type, int size, long since, RequestContext *userdata = nullptr); //!< Get candlesticks

private:
  void setBaseUrl(const char *); //!< Set platform base URL

  std::string m_baseUrl;
  long m_timeout;
};

}
}

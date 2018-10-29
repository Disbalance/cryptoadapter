#include <future>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <platform/log.h>
#include <fin/profiling.h>
#include "config.h"
#include "connector_rest_price.h"

namespace connector {
namespace example {

using namespace platform;

//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// REST Price API helper classes //////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

// Base response handler class for ZBRequests
class ResponseHandler 
  : public HttpReadyStateHandler
{
public:
  virtual ~ResponseHandler() { }
  ResponseHandler(RESTPriceConnector *api, const char *instrument, RequestContext *userData)
    : m_api(api)
    , m_instrument(instrument)
    , m_userData(userData)
  { }

  virtual void onReadyState(const HttpResponse *r) override
  {
    auto api = m_api;
    auto userData = m_userData;
    auto instrument(std::move(m_instrument));
    if(!r->isTimedOut()) {
      if(r->getStatus() == 200) {
        this->onResponse(r);
      } else {
/*
	    std::cout << "URL: " << r->getRequest().getUrl() << std::endl;
	    std::cout << "Method: " << (int) r->getRequest().getMethod() << std::endl;
	    for (const auto &header: r->getRequest().getHeaders()) {
	    	std::cout << "Header: " << header.first << ":" << header.second << std::endl;
	    }
	    std::cout << "Body: " << r->getBody() << std::endl;
*/
//		userData
        api->onHTTPError(std::move(instrument), r, userData);
      }
    } else {
      std::cout << "Connection timed out: " << r->getRequest().getUrl() << std::endl;
      api->onTimeout(std::move(instrument), userData);
    }
    delete this;
  }

  virtual void onResponse(const HttpResponse *r) = 0;

protected:
  RESTPriceConnector *m_api;
  std::string m_instrument;
  RequestContext *m_userData;
};

// Response handler for Depth request
class DepthResponseHandler
  : public ResponseHandler 
{
public:
  DepthResponseHandler(RESTPriceConnector *api, const char *instrument, RequestContext *userData)
    : ResponseHandler(api, instrument, userData)
  { }
  virtual void onResponse(const HttpResponse *r) {
    //! Called when we receive response to getDepth request
    m_api->onDepthResponse(std::move(r->getBody()), m_instrument.c_str(), m_userData);
  }
};

// Response handler for Ticker request
class TickerResponseHandler
  : public ResponseHandler 
{
public:
  TickerResponseHandler(RESTPriceConnector *api, const char *instrument, RequestContext *userData)
    : ResponseHandler(api, instrument, userData)
  { }
  virtual void onResponse(const HttpResponse *r) {
    //! Called when we receive response to getTicker request
    m_api->onTickerResponse(std::move(r->getBody()), m_instrument.c_str(), m_userData);
  }
};

// Response handler for Trades request
class TradesResponseHandler
  : public ResponseHandler 
{
public:
  TradesResponseHandler(RESTPriceConnector *api, const char *instrument, RequestContext *userData)
    : ResponseHandler(api, instrument, userData)
  { }
  virtual void onResponse(const HttpResponse *r) {
    //! Called when we receive response to getTrades request
    m_api->onTradesResponse(std::move(r->getBody()), m_instrument.c_str(), m_userData);
  }
};

// Response handler for Kline request
class KlineResponseHandler
  : public ResponseHandler
{
public:
  KlineResponseHandler(RESTPriceConnector *api, const char *instrument, RequestContext *userData)
    : ResponseHandler(api, instrument, userData)
  { }
  virtual void onResponse(const HttpResponse *r) {
    //! Called when we receive response to Kline request
    m_api->onKlineResponse(std::move(r->getBody()), m_instrument.c_str(), m_userData);
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// REST Price API REST API ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////


RESTPriceConnector::RESTPriceConnector()
  : m_timeout(2000)
{
  setBaseUrl(connector::example::REST_URL);
}

/**
 * Set HTTP connection timeout
 * @param timeoutMs timeout in milliseconds
 */
void RESTPriceConnector::setTimeout(long timeoutMs)
{
  std::cout << "Setting timeout to " << timeoutMs << std::endl;
  m_timeout = timeoutMs;
}

/**
 * Sets API root URL
 * @param url base URL for API requests
 */
void RESTPriceConnector::setBaseUrl(const char *url) {
  m_baseUrl = url;
}

/**
 * Fetch full market depth
 * @param symbol symbol to get market depth for (in OKex format)
 * @param size depth of the market (result size) (Positions are from 1-50, if there is a combination of depth, only return 5 positions depth)
 * @param userdata userdata to pass to the handle. Attaching void* tags may allow to identify exact request the response belongs to.
 * @see https://www.zb.com/i/developer/restApi#depth
 */
bool RESTPriceConnector::getDepth(const char *symbol, int size, RequestContext *userdata) {
  auto request = HttpClient::instance().createRequest(m_baseUrl + "depth?market=" + symbol +
                                                                  "&size=" + boost::lexical_cast<std::string>(size));
  request->setReadyStateHandler(new DepthResponseHandler(this, symbol, userdata));
  request->setResponseTimeout(m_timeout);
  request->send();
  return true;
}

/**
 * Fetches ticker
 * @param symbol symbol to get ticker for (in ZB format)
 * @param userdata userdata to pass to the handle. Attaching void* tags may allow to identify exact request the response belongs to.
 */
bool RESTPriceConnector::getTicker(const char *symbol, RequestContext *userdata) {
  auto request = HttpClient::instance().createRequest(m_baseUrl + "ticker?market=" + symbol);
  request->setReadyStateHandler(new TickerResponseHandler(this, symbol, userdata));
  request->setResponseTimeout(m_timeout);
  request->send();
  return true;
}

/**
 * Fetches trades
 * @param symbol symbol to get trades for (in ZB format)
 * @param since timestamp to start with (50 items after designate transaction ID)
 * @param userdata userdata to pass to the handle. Attaching void* tags may allow to identify exact request the response belongs to.
 * @see https://www.zb.com/i/developer/restApi#trades
 */
bool RESTPriceConnector::getTrades(const char *symbol, long since, RequestContext *userdata) {
  auto request = HttpClient::instance().createRequest(m_baseUrl + "trades?market=" + symbol +
                                                                  "&since=" + boost::lexical_cast<std::string>(since));
  request->setReadyStateHandler(new TradesResponseHandler(this, symbol, userdata));
  request->setResponseTimeout(m_timeout);
  request->send();
  return true;
}

/**
 * Fetches kline
 * @param symbol symbol to get kline for (in ZB format)
 * @param type candlestick interval type (1min, 3min, 5min, 15min, 30min, 1hour, 2hour, 4hour, 6hour, 12hour, day, 3day, week)
 * @param size the size of the result
 * @param since timestamp to start with 
 * @param userdata userdata to pass to the handle. Attaching void* tags may allow to identify exact request the response belongs to.
 * @see https://www.zb.com/i/developer/restApi#mkline
 */
bool RESTPriceConnector::getKline(const char *symbol, const char *type, int size, long since, RequestContext *userdata) {
  auto request = HttpClient::instance().createRequest(m_baseUrl + "kline?market=" + symbol + "&type=" + type +
                                                                  "&size=" + boost::lexical_cast<std::string>(size) +
                                                                  "&since=" + boost::lexical_cast<std::string>(since));
  request->setReadyStateHandler(new KlineResponseHandler(this, symbol, userdata));
  request->setResponseTimeout(m_timeout);
  request->send();
  return true;
}

}
}

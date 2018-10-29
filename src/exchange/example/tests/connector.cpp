#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <iostream>
#include "example/connector_rest_price.h"
#include "example/connector_ws_price.h"

#include "platform/timer.h"

int htStatus;
std::string exch_data;


class WSPriceConnector
  : public connector::example::WSPriceConnector
{
  void onData(const char *data, size_t size, unsigned long networkTime) override
  {
    exch_data = data;
    std::cout << std::string(data, size) << ": " << networkTime << '\n';
  };

  virtual void onClose() override {
    std::cout << "Connection closed!" << std::endl;
  }

};

class RESTPriceConnector
  : public connector::example::RESTPriceConnector
{
public:
  virtual void onDepthResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata = nullptr) override
  {
    exch_data = data;
    std::cout << "onDepthResponse: " << data << std::endl;
  }
  virtual void onTickerResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata = nullptr) override
  {
    exch_data = data;
    std::cout << "onTickerResponse: " << data << std::endl;
  }
  virtual void onTradesResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata = nullptr) override
  {
    exch_data = data;
    std::cout << "onTradesResponse: " << data << std::endl;
  }
  virtual void onKlineResponse(std::string data, const char *symbol, connector::example::RequestContext *userdata = nullptr) override
  {
    exch_data = data;
    std::cout << "onKlineResponse: " << data << std::endl;
  }
  
  virtual void onTimeout(std::string symbol, connector::example::RequestContext *userdata = nullptr) override
  {
    std::cout << "onTimeout" << std::endl;
  }
  virtual void onHTTPError(std::string symbol, const platform::HttpResponse *r, connector::example::RequestContext *userdata = nullptr) override
  {
    std::cout << "onHTTPError: " << "Status " << r->getStatus() << std::endl;
    std::cout << "Method: " << (int) r->getRequest().getMethod() << std::endl;
	std::cout << "URL: " << r->getRequest().getUrl() << std::endl;
	for (const auto &header: r->getRequest().getHeaders()) {
		std::cout << "Header: " << header.first << ":" << header.second << std::endl;
	}
	for (const auto &header: r->getHeaders()) {
		std::cout << "Header: " << header.first << ":" << header.second << std::endl;
	}
  	htStatus = r->getStatus();
  }
};
/*
int main(int argc, char *argv[]) {
  if(argc != 3) {
    std::cout << "Command line rgumments are required. \n"
              << "Usage:\n\n"
              << "\t" << argv[0] << " running_seconds channel\n\n"
              << "Example: \n\n"
              << "\t" << argv[0] << " 60 bch_btc\n\n";
    return -1;
  }

  platform::WebSocketClient wsclient;
  platform::HttpClient httpclient;
  platform::TimerService timerSvc;

  WSPriceConnector connectorWSAPI;
  RESTPriceConnector connectorAPI;

  connectorAPI.getDepth("bch_btc", 200);

  connectorWSAPI.start();
  connectorWSAPI.subscribe(argv[2]);

  platform::Timer *timer = timerSvc.createTimer([&connectorWSAPI, &timer](platform::Timer*) {
                                                  connectorWSAPI.ping();
                                                  timer->start(std::chrono::seconds(5));
                                                });

  timer->start(std::chrono::seconds(5));
  sleep(std::stoi(std::string(argv[1])));
}
*/

/*
TODO:
in special cases check responce not only for it's stream length
maybe for minimum one existed pair of keys and values
*/

BOOST_AUTO_TEST_SUITE(TestExampleRESTPriceConnector)
  BOOST_AUTO_TEST_CASE(getKlineTest){
    platform::HttpClient httpclient;
    RESTPriceConnector connectorAPI;
    //connectorAPI.fetchSymbols("");
    connectorAPI.getKline("btc_usdt", "1hour", 100, 1540792500000);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    unsigned int data_length = exch_data.length();
    std::cout << "[DEBUG]:\nData length method: " << data_length << std::endl;
    BOOST_REQUIRE(data_length > 0);
  }
  BOOST_AUTO_TEST_CASE(getTradesTest){
  platform::HttpClient httpclient;
  RESTPriceConnector connectorAPI;
  connectorAPI.getTrades("btc_usdt", 25);
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  unsigned int data_length = exch_data.length();
  std::cout << "[DEBUG]:\nData length method: " << data_length << std::endl;
  BOOST_REQUIRE(data_length > 0);
 }
  BOOST_AUTO_TEST_CASE(getTickerTest){
  platform::HttpClient httpclient;
  RESTPriceConnector connectorAPI;
  connectorAPI.getTicker("btc_usdt");
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  unsigned int data_length = exch_data.length();
  std::cout << "[DEBUG]:\nData length method: " << data_length << std::endl;
  BOOST_REQUIRE(data_length > 0);
 }
  BOOST_AUTO_TEST_CASE(getDepthTest){
  platform::HttpClient httpclient;
  RESTPriceConnector connectorAPI;
  connectorAPI.getDepth("btc_usdt", 100);
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  unsigned int data_length = exch_data.length();
  std::cout << "[DEBUG]:\nData length method: " << data_length << std::endl;
  BOOST_REQUIRE(data_length > 0);
 }
BOOST_AUTO_TEST_SUITE_END()

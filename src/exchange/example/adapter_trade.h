#pragma once

#include <stdexcept>
#include <list>
#include <atomic>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <platform/http.h>
#include <fin/instrument_registry.h>
#include <fin/exchange.h>
#include <fin/exchange_dictionary.h>
#include <pjson.h>
#include "connector_ws_trade.h"

#ifndef WAIT_LOOP_DURATION
#define WAIT_LOOP_DURATION  10000
#endif

namespace adaptor {
namespace example {

//! Interface for trade exchange connector
class TradeAdapter
  : public fin::BaseTradeExchangeConnector
  , public connector::example::WSTradeConnector
  , public platform::HttpReadyStateHandler
{
public:
  TradeAdapter(fin::interface::TradeExchangeObserver *);
  virtual ~TradeAdapter() { }

  virtual bool placeOrder(fin::TradeOrderHandle) override;
  virtual bool cancelOrder(fin::TradeOrderHandle) override;
  virtual bool getOrderStatus(fin::TradeOrderHandle) override;
  virtual bool getOrdersList() override;
  virtual bool getBalance() override;
  virtual void config(std::string) override;
  virtual void start() override;
  virtual void stop() override;

protected:
  enum class OrderChannelType {
    ok_spot_order = 0,
    ok_spot_cancel_order,
    ok_spot_orderinfo,
    order_channel_last
  };

  virtual void onLoginResponse(const char *data, size_t size, unsigned long timestamp) override;
  virtual void onOrderPlacedResponse(const char *data, size_t size, unsigned long timestamp) override;
  virtual void onOrderCancelledResponse(const char *data, size_t size, unsigned long timestamp) override;
  virtual void onUserAccountInfoResponse(const char *data, size_t size, unsigned long timestamp) override;
  virtual void onOrderInfoResponse(const char *data, size_t size, unsigned long timestamp) override;
  virtual void onBalanceResponse(const char *data, size_t size, unsigned long timestamp) override;
  virtual void onTradesResponse(const char *data, size_t size, unsigned long timestamp) override;
  virtual void onPingTimeout() override;
  virtual void onResponseTimeout() override;
  virtual void onClose() override;
  virtual void onReadyState(const platform::HttpResponse *response) override;
  void orderResponseTimeout();
  bool parseResponse(char *string, pjson::document &doc);
  void fetchLimits();

  void addOrderRequest(OrderChannelType channel, const fin::TradeOrderHandle &);
  fin::TradeOrderHandle removeOrderRequest(OrderChannelType channel);

  inline void lock() {
    int waitCycle = 1;
    while(__builtin_expect(m_lock.test_and_set(std::memory_order_acquire), 0)) {
      if(waitCycle % WAIT_LOOP_DURATION == 0) {
        std::this_thread::yield();
      }
      waitCycle ++;
    }
  }
  inline void unlock() {
    m_lock.clear();
  }

private:
  fin::ExchangeDictionary m_exchangeDictionary;
  typedef std::queue<fin::TradeOrderHandle> OrdersQueue;
  std::map<OrderChannelType, OrdersQueue> m_ordersQueue;
  size_t m_orderResponseTimeout;
  std::atomic_flag m_lock;
  std::string m_limitsUrl;
  std::atomic<bool> m_started;
};

#undef WAIT_LOOP_DURATION

}
}

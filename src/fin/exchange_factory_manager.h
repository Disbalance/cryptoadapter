/***************************************************
 * exchange_pool.h
 * Created on Sat, 29 Sep 2018 17:27:36 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <map>
#include <set>
#include <list>
#include "cstringmap.h"
#include "exchange.h"
#include "market.h"
#include "exchange_factory.h"

namespace fin {

class ExchangeFactoryManager {
public:
  ExchangeFactoryManager();
  ~ExchangeFactoryManager();

  void registerExchangeFactory(interface::TradeExchangeFactory *);

  std::list<interface::TradeExchangeFactory*> getFactories();

  interface::TradeExchangeConnector *createTradeExchangeConnector(const char *type, interface::TradeExchangeObserver *);
  interface::StockDataConnector *createStockDataConnector(const char *type, interface::StockDataObserver *);

  static ExchangeFactoryManager &instance();

private:
  std::map<std::string, interface::TradeExchangeFactory*> m_exchanges;

  static ExchangeFactoryManager *s_instance;
};

}

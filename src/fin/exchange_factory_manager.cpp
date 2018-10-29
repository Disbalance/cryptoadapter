/***************************************************
 * exchange_pool.cpp
 * Created on Sat, 29 Sep 2018 17:33:30 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "exchange_factory_manager.h"

namespace fin {

ExchangeFactoryManager *ExchangeFactoryManager::s_instance = NULL;

ExchangeFactoryManager::ExchangeFactoryManager() {
  if(s_instance) {
    throw std::runtime_error("ExchangeFactoryManager instance already exists!");
  }
  s_instance = this;
}

ExchangeFactoryManager::~ExchangeFactoryManager() {
  s_instance = NULL;
}

ExchangeFactoryManager &ExchangeFactoryManager::instance() {
  if(!s_instance) {
    throw std::runtime_error("ExchangeFactoryManager instance does not exist!");
  }
  return *s_instance;
}

void ExchangeFactoryManager::registerExchangeFactory(interface::TradeExchangeFactory *factory) {
  std::string name(factory->getName());
  auto i = m_exchanges.find(name.c_str());

  if(i != m_exchanges.end()) {
    std::string message = std::string("Exchange '") + name + std::string("' already exists!");
    throw std::runtime_error(message.c_str());
  }

  m_exchanges.emplace(std::make_pair(name.c_str(), factory));
}

std::list<interface::TradeExchangeFactory*> ExchangeFactoryManager::getFactories() {
  std::list<interface::TradeExchangeFactory*> result;

  for(auto factory : m_exchanges) {
    result.push_back(factory.second);
  }

  return result;
}

interface::TradeExchangeConnector *ExchangeFactoryManager::createTradeExchangeConnector(const char* type, interface::TradeExchangeObserver *observer) {
  auto t = m_exchanges.find(type);

  if(t == m_exchanges.end()) {
    std::string message = std::string("Exchange '") + type + std::string("' does not exist!");
    throw std::runtime_error(message.c_str());
  }

  interface::TradeExchangeConnector *exchange = t->second->createTradeExchangeConnector(observer);
  return exchange;
}

interface::StockDataConnector *ExchangeFactoryManager::createStockDataConnector(const char* type, interface::StockDataObserver *observer) {
  auto t = m_exchanges.find(type);

  if(t == m_exchanges.end()) {
    std::string message = std::string("Exchange '") + type + std::string("' does not exist!");
    throw std::runtime_error(message.c_str());
  }

  interface::StockDataConnector *stock = t->second->createStockDataConnector(observer);
  return stock;
}

}

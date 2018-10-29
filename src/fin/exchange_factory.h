/***************************************************
 * exchange_factory.h
 * Created on Fri, 28 Sep 2018 07:53:46 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include "market.h"
#include "exchange.h"

//! Classes and functions related to finances
namespace fin {
//! Interfaces
namespace interface {

//! Trade exchange factory interface
class TradeExchangeFactory {
public:
  TradeExchangeFactory(const std::string &name); //!< Constructor
  virtual ~TradeExchangeFactory() { }
  virtual TradeExchangeConnector *createTradeExchangeConnector(TradeExchangeObserver *) = 0; //!< Implement specific function to create trading connector
  virtual StockDataConnector *createStockDataConnector(StockDataObserver *) = 0; //!< Implement specific function to create market data connector
  const std::string &getName() const; //!< Returns connector type (factory name)

private:
  std::string m_name;
};

}
}

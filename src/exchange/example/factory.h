#pragma once
#include <fin/exchange_factory.h>

namespace adaptor {
namespace example {

/**
 * Implements TradeExchangeFactory interface for okex adaptors
 * @see fin::interface::TradeExchangeFactory
 */
class Factory
  : public fin::interface::TradeExchangeFactory {
public:
  Factory();
  //! Implements createTradeExchangeConnector method of the interface
  virtual fin::interface::TradeExchangeConnector *createTradeExchangeConnector(fin::interface::TradeExchangeObserver *);
  //! Implements createStockDataConnector method of the interface
  virtual fin::interface::StockDataConnector *createStockDataConnector(fin::interface::StockDataObserver *);
};

}
}

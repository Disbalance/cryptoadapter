#include "factory.h"
#include "adapter_price.h"

namespace adaptor {
namespace example {

Factory::Factory()
  : fin::interface::TradeExchangeFactory("example") {
}

/**
 * Creates OKex trade exchange connector
 * @param observer TradeExchangeObserver for the connector
 * @see fin::interface::TradeExchangeFactory::createTradeExchangeConnector(fin::interface::TradeExchangeObserver *)
 */
fin::interface::TradeExchangeConnector *Factory::createTradeExchangeConnector(fin::interface::TradeExchangeObserver *observer) {
  return NULL;
}

/**
 * Creates OKex marketing data connector
 * @param observer StockDataObserver for the connector
 * @see fin::interface::TradeExchangeFactory::createStockDataConnector(fin::interface::StockDataObserver *)
 */
fin::interface::StockDataConnector *Factory::createStockDataConnector(fin::interface::StockDataObserver *observer) {
  return new PriceAdapter(observer);
}

}
}

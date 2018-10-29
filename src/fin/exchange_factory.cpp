/***************************************************
 * exchange_factory.cpp
 * Created on Wed, 03 Oct 2018 11:19:59 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "exchange_factory.h"
#include "exchange_factory_manager.h"

namespace fin {
namespace interface {

TradeExchangeFactory::TradeExchangeFactory(const std::string &name)
  : m_name(name)
{
  ExchangeFactoryManager::instance().registerExchangeFactory(this);
}

const std::string &TradeExchangeFactory::getName() const
{
  return m_name;
}

}
}

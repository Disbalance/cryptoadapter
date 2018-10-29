/***************************************************
 * exchange.cpp
 * Created on Thu, 27 Sep 2018 11:30:34 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#include "exchange.h"
#include "platform/log.h"
#include "instrument_registry.h"

namespace fin {

using namespace interface;

BaseTradeExchangeConnector::BaseTradeExchangeConnector(TradeExchangeObserver *observer)
  : m_observer(observer)
  , m_makerFee(0)
  , m_takerFee(0)
  , m_defaultCommissionStrategy(CommissionStrategy::getByName("external"))
{
  InstrumentRegistry &registry = InstrumentRegistry::instance();
  auto symbols = registry.getSymbols();
  auto instruments = registry.getInstruments();
  for(auto symbol : symbols) {
    addSymbol(symbol);
  }
  for(auto instrument : instruments) {
    addInstrument(instrument);
  }
}

/** Create a trade order on given trade exchange.
 * @return handle (effectively smart_ptr) to created trade order
 */
TradeOrderHandle BaseTradeExchangeConnector::createOrder()
{
  return std::make_shared<TradeOrder>(*this); // Copy elision applies
}

/** Signal about updates in trade order status as order changes.
 * Call this function in your adaptor if you get a response that indicates some sort of change in order state
 * (order placement succeeded, order information request, etc.).
 * @param order Trade order to change the status for
 * @param status New order status
 * @param tag Profiling tag
 */
void BaseTradeExchangeConnector::updateOrderStatus(const TradeOrderHandle &order, OrderStatus &status, ProfilingTag tag)
{
  if(m_observer) {
    order->swapStatus(status); // Swap new and old status
    m_observer->orderStatusChanged(order, status, tag); // Also pass old status
  }
}

/** Get locally stored balance for given symbol.
 * This function returns the balance that was received during last update for given currency.
 * @param symbol The symbol to get balance for
 * @return currently stored balance for given currency, or 0
 */
double BaseTradeExchangeConnector::getBalance(SymbolHandle symbol)
{
  auto balance = m_balances.find(symbol);
  if(balance != m_balances.end()) {
    return balance->second.first - balance->second.second;
  } else {
    m_balances[symbol].first.store(0, std::memory_order_release);
    m_balances[symbol].second.store(0, std::memory_order_release);
  }

  return 0;
}

/** Reserve certain amount on trading balance for certain currency.
 * This is the default implementation for the interface function, which suits most adaptors.
 * It's unlikely that you will have to re-implement it.
 * @param symbol The symbol to reserve balance for
 * @param quantity The amount to reserve
 * @return true if succeeds, false otherwise
 */
bool BaseTradeExchangeConnector::reserveBalance(SymbolHandle symbol, double quantity)
{
  auto i = m_balances.find(symbol);
  if(i == m_balances.end()) {
    return false;
  }
  double currentReserve = i->second.second;
  double currentBalance;
  double newReserve;
  bool success;
  do {
    currentBalance = getBalance(symbol);//i->second.first;
    newReserve = currentReserve + quantity;
    if((currentBalance < newReserve) || (newReserve < 0)) {
      return false;
    }
    success = i->second.second.compare_exchange_weak(currentReserve, newReserve, 
                                                     std::memory_order_release, 
                                                     std::memory_order_acquire);
  } while(!success);

  currentBalance = getBalance(symbol);//i->second.first;
  if(currentBalance < newReserve) {
    unreserveBalance(symbol, quantity);
    return false;
  }

  return true;
}

/** Unreserve certain amount on trading balance for certain currency.
 * This is the default implementation for the interface function, which suits most adaptors.
 * It's unlikely that you will have to re-implement it.
 * @param symbol The symbol to unreserve balance for
 * @param quantity The amount to unreserve
 * @return true if succeeds, false otherwise (should always return true)
 */
bool BaseTradeExchangeConnector::unreserveBalance(SymbolHandle symbol, double quantity)
{
  auto i = m_balances.find(symbol);
  if(i == m_balances.end()) {
    return false;
  }

  double currentReserve = i->second.second;
  double newReserve;
  bool success;

  do {
    newReserve = currentReserve - quantity;
    success = i->second.second.compare_exchange_strong(currentReserve, newReserve, 
                                                       std::memory_order_release, 
                                                       std::memory_order_acquire);
  } while(!success);

  return true;
}

/** Reserve orderbook item by specified price and direction (ask/bid).
 * This is the default implementation for the interface function, which suits most adaptors.
 * It's unlikely that you will have to re-implement it.
 * @param instrument Currency pair to reserve DoM entry for
 * @param direction ask or bid line is being reserved
 * @param price price of the item being reserved
 * @param currentAmount current amount of the element being reserved
 * @param amountToReserve amount to reserve
 * @return true if succeeds, false otherwise
 */
bool BaseTradeExchangeConnector::reserveItem(InstrumentHandle instrument, OrderDir direction, const FixedNumber &price, 
                                             const FixedNumber &currentAmount, const FixedNumber &amountToReserve) {
  auto i = m_orderBookReserve.find(instrument);
  if(i == m_orderBookReserve.end()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(i->second.reserveLock);
  if(currentAmount > 0) { // Reserving Ask line, check if we don't exceed the amount
    if((i->second.reserve[(int)direction][price] + amountToReserve.toDouble()) > currentAmount.toDouble()) {
      return false;
    }
  }
  else { // Reserving Bid line, check if we don't exceed the amount
    if((i->second.reserve[(int)direction][price] + amountToReserve.toDouble()) < currentAmount.toDouble()) {
      return false;
    }
  }

  i->second.reserve[(int)direction][price] += amountToReserve.toDouble();
  return true;
}
 
/** Get reserve for specified orderbook item (instrument + price + ask/bid).
 * @return reserved amount
 */
double BaseTradeExchangeConnector::getItemReserve(InstrumentHandle instrument, OrderDir direction, const FixedNumber &price) {
  auto i = m_orderBookReserve.find(instrument);
  if(i == m_orderBookReserve.end()) {
    return 0;
  }

  std::lock_guard<std::mutex> lock(i->second.reserveLock);
  auto j = i->second.reserve[(int)direction].find(price);

  if(j != i->second.reserve[(int)direction].end()) {
    return j->second;
  }

  return 0;
}

/** Unreserve orderbook item by specified price.
 * @return true if succeeds, false otherwise
 */
bool BaseTradeExchangeConnector::unreserveItem(InstrumentHandle instrument, OrderDir direction, const FixedNumber &price, const FixedNumber &amountToReserve) {
  auto i = m_orderBookReserve.find(instrument);
  if(i == m_orderBookReserve.end()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(i->second.reserveLock);
  i->second.reserve[(int)direction][price] -= amountToReserve.toDouble();

  // Due to double inaccuracy there can be discrepancy between reserved amount and result after
  // unreserve, check if it's close enough to count as 0
  if(i->second.reserve[(int)direction][price] > -0.0000000000001 && i->second.reserve[(int)direction][price] < 0.0000000000001) {
    i->second.reserve[(int)direction].erase(price);
  }

  return true;
}
 
/** Adds symbol to the list of currently stored trade symbols.
 */
void BaseTradeExchangeConnector::addSymbol(SymbolHandle symbol) {
  m_balances[symbol].first.store(0);
  m_balances[symbol].second.store(0);
}

/** Adds instrument to the list of currently stored trading instruments.
 */
void BaseTradeExchangeConnector::addInstrument(InstrumentHandle instrument) {
  m_orderBookReserve[instrument].reserve[0].clear();
  m_orderBookReserve[instrument].reserve[1].clear();
}

/** Update local balance as it is returned from the exchange.
 * Call this function to send the updated balance upstream.
 * @param symbol The symbol to get balance for
 * @param value The value for balance
 * @param tag Profiling tag
 */
void BaseTradeExchangeConnector::updateBalance(SymbolHandle symbol, FixedNumber value, ProfilingTag tag)
{
  auto balance = m_balances.find(symbol);
  if(balance != m_balances.end()) {
    balance->second.first.store(value.toDouble(), std::memory_order_release);
  }

  if(m_observer) {
    m_observer->balanceReceived(symbol, std::move(value), tag);
  }
}

/** Set connector error, it is also possible to pass the exception.
 * Call this function if your adaptor wants to signal some sort of error happened during normal work.
 * @param exception Pointer to current exception
 */
void BaseTradeExchangeConnector::setConnectorError(std::exception_ptr exception)
{
  if(m_observer) {
    m_observer->tradingConnectorError(exception);
  }
}

/** Gets taker fee, which is configured in the configuration json file by adding "taker-fee" in the connector config.
 * @return exchange taker fee
 */
double BaseTradeExchangeConnector::getTakerFee() const {
  return m_takerFee;
}

/** Sets taker fee. Usually it is configured using "taker-fee" key in the connector config, but you also can override
 * this setting and set it manually from your connector, if the API supports it.
 * @param fee Taker fee to set
 */
void BaseTradeExchangeConnector::setTakerFee(double fee) {
  m_takerFee = fee;
}

/** Get constraints for certain instrument.
 * @param handle Instrument handle to set the constraints for
 * @return The constraints for specified instrument
 */
TradeExchangeConstraints BaseTradeExchangeConnector::getConstraints(InstrumentHandle handle) const {
  const auto &constraints = m_constraints.find(handle);
  if(constraints == m_constraints.end()) {
    TradeExchangeConstraints constraint;
    constraint.commissionStrategy = m_defaultCommissionStrategy;
    return constraint;
  }

  return constraints->second;
}

/** Set constraints for certain instrument.
 * @param handle Instrument handle to set the constraints for
 * @param constraints The constraints to set
 */
void BaseTradeExchangeConnector::addConstraints(InstrumentHandle handle, const TradeExchangeConstraints &constraints) {
  m_constraints[handle] = constraints;
  if(!constraints.commissionStrategy) {
    m_constraints[handle].commissionStrategy = m_defaultCommissionStrategy;
  }
}

/** Set commission strategy that will be used for instruments where specific commission charging strategy
 * is not specified.
 * @param strategy set default commission charging strategy
 */
void BaseTradeExchangeConnector::setDefaultCommissionStrategy(CommissionStrategy *strategy) {
  m_defaultCommissionStrategy = strategy;
}

/** Gets maker fee, which is configured in the configuration json file by adding "maker-fee" in the connector config.
 * @return exchange maker fee
 */
double BaseTradeExchangeConnector::getMakerFee() const {
  return m_makerFee;
}

/** Sets maker fee. Usually it is configured using "maker-fee" key in the connector config, but you also can override
 * this setting and set it manually from your connector, if the API supports it.
 * @param fee Maker fee to set
 */
void BaseTradeExchangeConnector::setMakerFee(double fee) {
  m_makerFee = fee;
}

/** Returns the connector name as it appears in the config file.
 * @return connector name
 */
const char *BaseTradeExchangeConnector::getName() {
  return m_name.c_str();
}

/** Sets given connector name.
 * @return connector name
 */
void BaseTradeExchangeConnector::setName(const std::string &name) {
  m_name = name;
}


}

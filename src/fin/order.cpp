/***************************************************
 * order.cpp
 * Created on Wed, 26 Sep 2018 15:46:20 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#include <stdexcept>
#include "order.h"
#include "exchange.h"

namespace fin {

TradeOrder::TradeOrder(interface::TradeExchangeConnector &exchange)
  : m_exchange(exchange)
{ }

interface::TradeExchangeConnector &TradeOrder::getConnector()
{
  return m_exchange;
}

bool TradeOrder::place()
{
  return m_exchange.placeOrder(getHandle());
}

bool TradeOrder::updateStatus()
{
  return m_exchange.getOrderStatus(getHandle());
}

bool TradeOrder::cancel()
{
  return m_exchange.cancelOrder(getHandle());
}

TradeOrderHandle TradeOrder::getHandle()
{
  return shared_from_this();
}

InstrumentHandle TradeOrder::getInstrument() const
{
  return m_instrument;
}

void TradeOrder::setInstrument(InstrumentHandle h)
{
  m_instrument = h;
}

OrderDir TradeOrder::getDirection() const
{
  return m_direction;
}

void TradeOrder::setDirection(OrderDir dir)
{
  m_direction = dir;
}

const FixedNumber &TradeOrder::getAmount() const
{
  return m_amount;
}

void TradeOrder::setAmount(FixedNumber amount)
{
  m_amount.assign(std::move(amount));
}

const FixedNumber &TradeOrder::getPrice() const
{
  return m_price;
}

void TradeOrder::setPrice(FixedNumber price)
{
  m_price.assign(std::move(price));
}

void TradeOrder::setUserdata(const TradeOrderUserdataPtr &userdata)
{
  m_userdata = userdata;
}

TradeOrderUserdataPtr TradeOrder::getUserdata()
{
  return m_userdata;
}

const OrderStatus &TradeOrder::getStatus() const
{
  return m_status;
}

void TradeOrder::swapStatus(OrderStatus &status)
{
  m_status.swap(status);
}

ExecutionType TradeOrder::getExecutionType() const
{
  return m_executionType;
}

void TradeOrder::setExecutionType(ExecutionType type)
{
  m_executionType = type;
}

std::ostream &operator <<(std::ostream &out, const TradeOrder &order)
{
  static std::vector<std::string> orderStatus = {
    "None", //!< Undefined (just created)
    "Unknown", //!< Order placement issued, but the result is unknown (for instance, we got no response)
    "Placed", //!< Order was placed
    "Filled", //!< Order was filled
    "PartialFilled", //!< Order was filled partially
    "PartialCancelled", //!< Order was partially cancelled
    "Cancelled", //!< Order was cancelled
    "Failed", //!< Order placement failed
  };
  static std::vector<std::string> direction = { "Bid", "Ask" };
  static std::vector<std::string> executionType = { "LIMIT", "IOC", "MARKET" };

  out << "Trade order(" << &order 
      << "):exchange:" << order.m_exchange.getName()
      << ";symbol:" << order.m_instrument->first->name << ":" << order.m_instrument->second->name
      << ";type:" << direction[(int)order.m_direction]
      << ";execution:" << executionType[(int)order.m_executionType]
      << ";amount:" << order.m_amount
      << ";price:" << order.m_price
      << ";status:" << orderStatus[(int)order.m_status.getState()];
  return out;
}



OrderStatus::State OrderStatus::getState() const
{
  return m_state;
}

const FixedNumber &OrderStatus::getFilledAmount() const
{
  return m_filledAmount;
}

const FixedNumber &OrderStatus::getFilledPrice() const
{
  return m_filledPrice;
}

long OrderStatus::getCreatedTimestamp() const
{
  return m_createdTimestamp;
}

long OrderStatus::getFinishedTimestamp() const
{
  return m_finishedTimestamp;
}

long OrderStatus::getCancelledTimestamp() const
{
  return m_cancelledTimestamp;
}

void OrderStatus::setState(State state) {
  m_state = state;
}

void OrderStatus::setFilledAmount(const FixedNumber &amount) {
  m_filledAmount.assign(amount);
}

void OrderStatus::setFilledPrice(const FixedNumber &price) {
  m_filledPrice.assign(price);
}

void OrderStatus::setCreatedTimestamp(long ts) {
  m_createdTimestamp = ts;
}

void OrderStatus::setFinishedTimestamp(long ts) {
  m_finishedTimestamp = ts;
}

void OrderStatus::setCancelledTimestamp(long ts) {
  m_cancelledTimestamp = ts;
}

OrderStatus::OrderStatus()
  : m_state(OrderStatus::None)
  , m_filledAmount((double)0)
  , m_filledPrice((double)0)
  , m_createdTimestamp(0)
  , m_finishedTimestamp(0)
  , m_cancelledTimestamp(0)
{ }

OrderStatus::OrderStatus(const OrderStatus &old)
  : m_state(old.m_state)
  , m_filledAmount(old.m_filledAmount)
  , m_filledPrice(old.m_filledPrice)
  , m_createdTimestamp(old.m_createdTimestamp)
  , m_finishedTimestamp(old.m_finishedTimestamp)
  , m_cancelledTimestamp(old.m_cancelledTimestamp)
{ }

OrderStatus::OrderStatus(OrderStatus &&old)
  : m_state(old.m_state)
  , m_filledAmount(std::move(old.m_filledAmount))
  , m_filledPrice(std::move(old.m_filledPrice))
  , m_createdTimestamp(old.m_createdTimestamp)
  , m_finishedTimestamp(old.m_finishedTimestamp)
  , m_cancelledTimestamp(old.m_cancelledTimestamp)
{ }

void OrderStatus::swap(OrderStatus &old)
{
  if(&old == this) {
    return;
  }
  auto state = m_state;
  m_state = old.m_state;
  old.m_state = state;
  m_filledAmount.swap(old.m_filledAmount);
  m_filledPrice.swap(old.m_filledPrice);
  auto createdTimestamp = m_createdTimestamp;
  m_createdTimestamp = old.m_createdTimestamp;
  old.m_createdTimestamp = createdTimestamp;
  auto finishedTimestamp = m_finishedTimestamp;
  m_finishedTimestamp = old.m_finishedTimestamp;
  old.m_finishedTimestamp = finishedTimestamp;
  auto cancelledTimestamp = m_cancelledTimestamp;
  m_cancelledTimestamp = old.m_cancelledTimestamp;
  old.m_cancelledTimestamp = cancelledTimestamp;
  m_orderId.swap(old.m_orderId);
}

void OrderStatus::setOrderId(std::string id)
{
  m_orderId.swap(id);
}

const std::string &OrderStatus::getOrderId() const
{
  return m_orderId;
}

}

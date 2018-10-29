/**
 * @file
 * @brief Trade order related types
 *
 */
#pragma once

#include <future>
#include <memory>
#include "numeric.h"
#include "instrument.h"

//!< Classes and functions related to finances
namespace fin {

class TradeOrder;
class OrderStatus;

namespace interface {
class TradeExchangeConnector;
}

//! Userdata that can be attached to a trade order object
class TradeOrderUserdata {
public:
  virtual ~TradeOrderUserdata() { }
};

//! Handle (shared pointer) to trade order
typedef std::shared_ptr<TradeOrder> TradeOrderHandle;

//! Shared pointer to trade order userdata
typedef std::shared_ptr<TradeOrderUserdata> TradeOrderUserdataPtr;

//! Class for tracking order status
class OrderStatus {
public:
  //! Order status enum
  enum State {
    None = 0, //!< Undefined (just created)
    Unknown, //!< Order placement issued, but the result is unknown (for instance, we got no response)
    Placed, //!< Order was placed
    Filled, //!< Order was filled
    PartialFilled, //!< Order was filled partially
    PartialCancelled, //!< Order was partially cancelled
    Cancelled, //!< Order was cancelled
    Failed, //!< Order placement failed
  };

  State getState() const; //!< Get order state
  const FixedNumber &getFilledAmount() const; //!< Get fulfilled amount
  const FixedNumber &getFilledPrice() const; //!< Get fulfill price
  long getCreatedTimestamp() const; //!< Get timestamp when the order was created (placed), or 0
  long getFinishedTimestamp() const; //!< Get timestamp when the order was finished (or 0)
  long getCancelledTimestamp() const; //!< Get timestamp when the order was cancelled (or 0)
  const std::string &getOrderId() const; //!< Get order ID

  OrderStatus(OrderStatus &&); //!< Move constructor
  OrderStatus(const OrderStatus &); //!< Copy constructor
  OrderStatus &operator =(const OrderStatus &) = delete;

  void setState(State); //!< Set order state
  void setFilledAmount(const FixedNumber &); //!< Set fulfilled amount
  void setFilledPrice(const FixedNumber &); //!< Set fulfill price
  void setCreatedTimestamp(long); //!< Set timestamp when the order was created (placed), or 0
  void setFinishedTimestamp(long); //!< Set timestamp when the order was finished (or 0)
  void setCancelledTimestamp(long); //!< Set timestamp when the order was cancelled (or 0)
  void setOrderId(std::string); //!< Set order ID

private:
  OrderStatus();

  void swap(OrderStatus &);

  State m_state;
  FixedNumber m_filledAmount;
  FixedNumber m_filledPrice;
  long m_createdTimestamp;
  long m_finishedTimestamp;
  long m_cancelledTimestamp;
  std::string m_orderId;

  friend class TradeOrder;
  friend class interface::TradeExchangeConnector;
};

//! Order execution type
enum class ExecutionType {
  LIMIT = 0, //!< Limited price execution
  IOC, //!< Immediate or cancel
  MARKET //!< Market execution
};

//! Class representing trade order
class TradeOrder
  : public std::enable_shared_from_this<TradeOrder> {
public:
  bool place(); //!< Place order; updates order status when operation completed
  bool cancel(); //!< Cancel order; updates order status when operation completed
  bool updateStatus(); //!< Get order status; updates order status when status is updated

  InstrumentHandle getInstrument() const; //!< Get trading instrument
  void setInstrument(InstrumentHandle); //!< Set trading instrument
  OrderDir getDirection() const; //!< Get order direction
  void setDirection(OrderDir); //!< Set order direction
  const FixedNumber &getAmount() const; //!< Get order amount
  void setAmount(FixedNumber amount); //!< Set order amount
  const FixedNumber &getPrice() const; //!< Get order price
  void setPrice(FixedNumber amount); //!< Set order price
  const OrderStatus &getStatus() const; //!< Get order status
  void swapStatus(OrderStatus &); //!< Get order status
  ExecutionType getExecutionType() const; //!< Get execution type
  void setExecutionType(ExecutionType type); //!< Set execution type

  void setUserdata(const TradeOrderUserdataPtr &); //!< Set userdata
  TradeOrderUserdataPtr getUserdata(); //!< Get userdata

  TradeOrderHandle getHandle(); //!< Get shared pointer to this order
  TradeOrder(interface::TradeExchangeConnector &); // Create order on specified stock exchange
  interface::TradeExchangeConnector &getConnector(); // Get trade exchange connector

private:
  InstrumentHandle m_instrument;
  OrderDir    m_direction;
  ExecutionType m_executionType;
  FixedNumber m_amount;
  FixedNumber m_price;
  OrderStatus m_status;
  interface::TradeExchangeConnector &m_exchange;
  TradeOrderUserdataPtr m_userdata;

  friend class interface::TradeExchangeConnector;
  friend std::ostream &operator <<(std::ostream &, const TradeOrder &);
};

std::ostream &operator <<(std::ostream &, const TradeOrder &);

}

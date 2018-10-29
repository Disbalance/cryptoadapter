/**
 * @file
 * @brief Interfaces for trade exchange adapters
 *
 */
#pragma once

#include <stdexcept>
#include <future>
#include <string>
#include <map>
#include "numeric.h"
#include "instrument.h"
#include "commission_strategy.h"
#include "profiling.h"
#include "order.h"

//! Classes and functions related to finances
namespace fin {

struct TradeExchangeConstraints {
  FixedNumber priceMax;
  FixedNumber priceMin;
  FixedNumber priceQuantum;
  FixedNumber amountMax;
  FixedNumber amountMin;
  FixedNumber amountQuantum;
  FixedNumber totalMax;
  FixedNumber totalMin;
  FixedNumber totalQuantum;
  CommissionStrategy *commissionStrategy;
};

//! Interfaces
namespace interface {

//! Observer interface for trade exchange events
class TradeExchangeObserver {
public:
  virtual ~TradeExchangeObserver() { }
  virtual void orderStatusChanged(TradeOrderHandle tradeOrder, const OrderStatus &oldStatus, ProfilingTag tag) = 0; //!< Called when order status changes.
  virtual void balanceReceived(SymbolHandle, FixedNumber, ProfilingTag) = 0; //!< Called when balance is updated from the trade exchange.
  virtual void tradingConnectorError(std::exception_ptr) = 0; //!< Called when connector detects error (network failure, etc).
};

//! Interface for trade exchange connector
class TradeExchangeConnector {
public:
  virtual ~TradeExchangeConnector() { }

  virtual bool placeOrder(TradeOrderHandle) = 0; //!< Implement connector-specific function to place order.
  virtual bool cancelOrder(TradeOrderHandle) = 0; //!< Implement connector-specific function to cancel order.
  virtual bool getOrderStatus(TradeOrderHandle) = 0; //!< Implement connector-specific function to update order status.
  virtual bool getOrdersList() = 0; //!< Implement connector-specific function to get the list of orders and feed them to updateOrderStatus.
  virtual bool getBalance() = 0; //!< Implement connector-specific function to request balance update.
  virtual void config(std::string) = 0; //!< Implement connector-specific config function. Receives json string as it appears in config file.
  virtual void start() = 0; //!< Implement connector-specific function to start connector.
  virtual void stop() = 0; //!< Implement connector-specific function to stop connector.

  virtual double getBalance(SymbolHandle symbol) = 0; //!< Get current (not updated) balance for given symbol.
  virtual bool reserveBalance(SymbolHandle symbol, double quantity) = 0; //!< Reserve amount from the balance for given symbol.
  virtual bool unreserveBalance(SymbolHandle symbol, double quantity) = 0; //!< Reserve amount from the balance for given symbol.
  virtual bool reserveItem(InstrumentHandle instrument, OrderDir direction, const FixedNumber &price, 
                           const FixedNumber &currentAmount, const FixedNumber &amountToReserve) = 0; //!< Add a reserve on a specified orderbook item.
  virtual bool unreserveItem(InstrumentHandle instrument, OrderDir direction, 
                           const FixedNumber &price, const FixedNumber &amountToReserve) = 0; //!< Remove a reserve on a certain orderbook item.
  virtual double getItemReserve(InstrumentHandle instrument, OrderDir direction, const FixedNumber &price) = 0; //!< Get reserve put on certain orderbook item.
  virtual double getMakerFee() const = 0; //!< Get maker fee (can be set in the configuration file).
  virtual void setMakerFee(double) = 0; //!< Set maker fee.
  virtual double getTakerFee() const = 0; //!< Get taker fee (can be set in the configuration file).
  virtual void setTakerFee(double) = 0; //!< Set taker fee.
  virtual TradeExchangeConstraints getConstraints(InstrumentHandle) const = 0; //!< Get trade limits on given instrument.

  virtual const char *getName() = 0; //!< Get exchange name.
  virtual void setName(const std::string &name) = 0; //!< Set exchange name.
  virtual TradeOrderHandle createOrder() = 0; //!< Create order using this connector.
};

} // End of namespace interface

/** Base class for a common trade exchange connector.
 * Implements common methods for most trade exchange connectors.
 * It is mostly sufficient to inherit this class when you start writing your own connector.
 */
class BaseTradeExchangeConnector 
  : public interface::TradeExchangeConnector {
public:
  BaseTradeExchangeConnector(interface::TradeExchangeObserver *);
  virtual ~BaseTradeExchangeConnector() { }

  virtual double getBalance(SymbolHandle symbol) override; //!< Get current (not updated) balance for given symbol.
  virtual bool reserveBalance(SymbolHandle symbol, double quantity) override; //!< Reserve amount from the balance for given symbol.
  virtual bool unreserveBalance(SymbolHandle symbol, double quantity) override; //!< Reserve amount from the balance for given symbol.
  virtual bool reserveItem(InstrumentHandle instrument, OrderDir direction, const FixedNumber &price, 
                           const FixedNumber &currentAmount, const FixedNumber &amountToReserve) override; //!< Reserve DoM entry.
  virtual bool unreserveItem(InstrumentHandle instrument, OrderDir direction, const FixedNumber &price, 
                           const FixedNumber &amountToReserve) override; //!< Unreserve DoM entry
  virtual double getItemReserve(InstrumentHandle instrument, OrderDir direction, const FixedNumber &price) override; //!< Get reserve put on certain orderbook item.
  virtual double getMakerFee() const override; //!< Get maker fee (can be set in the configuration file).
  virtual void setMakerFee(double) override; //!< Set maker fee.
  virtual double getTakerFee() const override; //!< Get taker fee (can be set in the configuration file).
  virtual void setTakerFee(double) override; //!< Set taker fee.
  TradeExchangeConstraints getConstraints(InstrumentHandle) const override; //!< Get trade limits on given instrument.

  virtual const char *getName() override; //!< Get exchange name as it appears in the configuration file.
  virtual void setName(const std::string &name) override; //!< Set exchange name.
  virtual TradeOrderHandle createOrder() override; //!< Create order using this connector.

  void addSymbol(SymbolHandle symbol); //!< Adds symbol to the list of trading symbols.
  void addInstrument(InstrumentHandle instrument); //!< Adds instrument to the list of trading instruments.

protected:
  //! Update order status, as it received from the trade exchange.
  void updateOrderStatus(const TradeOrderHandle &, OrderStatus &, ProfilingTag = ProfilingTag());
  //! Update balance for given symbol as it received from trade exchange.
  void updateBalance(SymbolHandle, FixedNumber, ProfilingTag = ProfilingTag());
  //! Indicate failure inside connector.
  void setConnectorError(std::exception_ptr);
  //! Add constraints to certain instrument.
  void addConstraints(InstrumentHandle, const TradeExchangeConstraints &);
  //! Set default commission charge strategy
  void setDefaultCommissionStrategy(CommissionStrategy *);

private:
  typedef std::pair<std::atomic<double>, std::atomic<double>> BalanceWithReservation;

  struct OrderBookReserveEntry {
    std::mutex reserveLock;
    std::map<FixedNumber, double> reserve[2];
  };

  interface::TradeExchangeObserver *m_observer;
  std::map<SymbolHandle, BalanceWithReservation> m_balances;
  std::map<InstrumentHandle, TradeExchangeConstraints> m_constraints;
  std::map<InstrumentHandle, OrderBookReserveEntry> m_orderBookReserve;
  std::string m_name;
  double m_makerFee;
  double m_takerFee;
  CommissionStrategy *m_defaultCommissionStrategy;
};

}

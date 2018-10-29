/***************************************************
 * commission_strategy.h
 * Created on Thu, 18 Oct 2018 13:47:45 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <map>
#include <memory>
#include "instrument.h"

namespace fin {

class CommissionStrategy {
public:
  virtual ~CommissionStrategy() { }
  virtual double getBaseRatio(OrderDir direction, double fee) = 0;
  virtual double getQuoteRatio(OrderDir direction, double fee) = 0;

  static CommissionStrategy *getByName(std::string name);

private:
  static std::map<std::string, std::shared_ptr<CommissionStrategy>> s_commissionStrategies;
};

class QuoteCommissionStrategy
  : public CommissionStrategy {
public:
  virtual ~QuoteCommissionStrategy() { }
  virtual double getBaseRatio(OrderDir direction, double fee) { return 1; }
  virtual double getQuoteRatio(OrderDir direction, double fee) { return direction == OrderDir::Buy ? 1 + fee : 1 - fee; }
};

class IncomeCommissionStrategy
  : public CommissionStrategy {
public:
  virtual ~IncomeCommissionStrategy() { }
  virtual double getBaseRatio(OrderDir direction, double fee) { return direction == OrderDir::Sell ? 1 : 1 - fee; }
  virtual double getQuoteRatio(OrderDir direction, double fee) { return direction == OrderDir::Buy ? 1 : 1 - fee; }
};

class ExternalCommissionStrategy
  : public CommissionStrategy {
public:
  virtual ~ExternalCommissionStrategy() { }
  virtual double getBaseRatio(OrderDir direction, double fee) { return 1; }
  virtual double getQuoteRatio(OrderDir direction, double fee) { return 1; }
};

}

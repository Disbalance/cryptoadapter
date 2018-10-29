/***************************************************
 * commission_strategy.cpp
 * Created on Thu, 18 Oct 2018 16:08:56 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <algorithm>
#include "commission_strategy.h"

namespace fin {

std::map<std::string, std::shared_ptr<CommissionStrategy>> CommissionStrategy::s_commissionStrategies = {
  {"quote", std::shared_ptr<CommissionStrategy>(new QuoteCommissionStrategy())},
  {"income", std::shared_ptr<CommissionStrategy>(new IncomeCommissionStrategy())},
  {"external", std::shared_ptr<CommissionStrategy>(new ExternalCommissionStrategy())}
};

CommissionStrategy *CommissionStrategy::getByName(std::string name) {
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  auto i = s_commissionStrategies.find(name);

  if (i == s_commissionStrategies.end()) {
    return NULL;
  }

  return i->second.get();
}

}

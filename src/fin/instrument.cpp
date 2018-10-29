/***************************************************
 * instrument.cpp
 * Created on Mon, 01 Oct 2018 17:56:11 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "instrument.h"

namespace fin {

std::ostream &operator<<(std::ostream &out, const fin::Symbol &s) {
  return out << s.symbol;
}

std::ostream &operator<<(std::ostream &out, const fin::Instrument &i) {
  return out << i.first->symbol << ":" << i.second->symbol;
}

std::ostream &operator<<(std::ostream &out, fin::SymbolHandle handle) {
  if(!handle) {
    return out << "<NoSymbol>";
  }
  return out << *handle;
}

std::ostream &operator<<(std::ostream &out, fin::InstrumentHandle handle) {
  if(!handle) {
    return out << "<NoInstrument>";
  }
  return out << *handle;
}

}

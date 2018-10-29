/**
 * @file
 * @brief Trading symbols and instrument related types
 *
 */
#pragma once

#include <iostream>
#include <string>
#include <map>

//!< Classes and functions related to finances
namespace fin {

//! Order direction (bid or ask)
enum class OrderDir {
  Bid = 0, //!< Bid (buy)
  Buy = Bid, //!< Buy (bid)
  Ask, //!< Ask (sell)
  Sell = Ask //!< Sell (ask)
};

//! Trading symbol (currency) representation
struct Symbol {
  std::string symbol; //!< Symbol abbreviation
  std::string name; //!< Symbol name (optional)

  Symbol(std::string symbolArg, std::string nameArg)
    : symbol(symbolArg)
    , name(nameArg)
  { }
};

//! Handle to the trading symbol stored in the system
typedef Symbol *SymbolHandle;

//! Trading instrument (curency pair)
typedef std::pair<SymbolHandle, SymbolHandle> Instrument;

//! Handle to the trading symbol stored in the system
typedef Instrument *InstrumentHandle;

//! Missing symbol handle
const SymbolHandle NoSymbol = nullptr;

//! Missing instrument handle
const InstrumentHandle NoInstrument = nullptr;

std::ostream &operator<<(std::ostream &out, const fin::Symbol &); //!< Output operator for Symbol class
std::ostream &operator<<(std::ostream &out, const fin::Instrument &); //!< Output operator for Instrument
std::ostream &operator<<(std::ostream &out, fin::SymbolHandle); //!< Output operator for symbol handle
std::ostream &operator<<(std::ostream &out, fin::InstrumentHandle); //!< Output operator for instrument handle

}


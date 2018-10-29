#include "mixed_orderbook.h"

#include <boost/range/adaptor/reversed.hpp>

#include <fin/instrument.h>

namespace fin {

namespace {

template<typename Book>
void clearBook(Book &book, MixedOrderBook::ExchangeType exchange)
{
  auto& indexedByExchanges = book.getData().template get<MixedOrderBook::ExchangeIdx>();
  auto p = indexedByExchanges.equal_range(exchange);
  indexedByExchanges.erase(p.first, p.second);
}

template<typename Book>
void clearBook(Book &book)
{
  book.getData().template get<MixedOrderBook::ExchangeIdx>().clear();
}

template<typename Book>
void updateBook(Book &book, MixedOrderBook::Item item)
{
  auto &data = book.getData();
  if(item.amount == 0) {
    auto found = data.template get<MixedOrderBook::ExchangePriceIdx>().find(boost::make_tuple(item.exchange, item.price));
    if(found != data.template get<MixedOrderBook::ExchangePriceIdx>().end()) {
      data.erase(found);
    }
  }
  else {
    data.insert(item);
  }
}

template<typename Book>
MixedOrderBook::PriceType getBestPrice(Book &book)
{
  auto &data = book.getData();
  auto begin = data.template get<MixedOrderBook::PriceIdx>().begin();
  if(begin != data.template get<MixedOrderBook::PriceIdx>().end()) {
    return begin->price;
  }
  return (double)0;
}

template<typename StreamType>
StreamType& dumpBook(const MixedOrderBook& book, StreamType& ostr, size_t n)
{
  auto instr = book.getInstrument();

  ostr << instr << " best bid/ask : " 
    << book.getBestBidPrice().toString() << "/" << book.getBestAskPrice().toString() << '\n';

  ostr << "asks:\n";
  size_t i = 0;
  for(auto item : /*boost::adaptors::reverse(*/book.getAsksSortedByPriceWithFee()) {
    ostr << i << "\t item:" << item << "\n";
    if(++i > n)
      break;

  }
  ostr << "bids:\n";
  i = 0;
  for(auto item : /*boost::adaptors::reverse(*/book.getBidsSortedByPriceWithFee()){
    ostr << i << "\t item:" << item << "\n";
    if(++i > n)
      break;
  }
  return ostr; 
}

} //namespace


void MixedOrderBook::update(ExchangeType exchange, OrderBookEntry entry, double fee)
{
  if(entry.instrument != this->m_instrument) {
    return;
  }

  if(entry.direction == OrderDir::Bid) {
    auto priceWithFee = FixedNumber(entry.price.toDouble() * (1 - fee));
    Item item(exchange, entry, priceWithFee, OrderDir::Bid, m_instrument);
    updateBook(m_bidsBook, item);
  }
  else {
    auto priceWithFee = FixedNumber(entry.price.toDouble() / (1 - fee));
    Item item(exchange, entry, priceWithFee, OrderDir::Ask, m_instrument);
    updateBook(m_asksBook, item);
  }
}

void MixedOrderBook::setInstrument(InstrumentHandle instrument)
{
  m_instrument = instrument;
}

InstrumentHandle MixedOrderBook::getInstrument() const
{
  return m_instrument;
}

void MixedOrderBook::clear(ExchangeType exchange)
{
    clearBook(m_bidsBook, exchange);
    clearBook(m_asksBook, exchange);
}

void MixedOrderBook::clear()
{
    clearBook(m_bidsBook);
    clearBook(m_asksBook);
}

MixedOrderBook::PriceType MixedOrderBook::getBestBidPrice() const
{
  return getBestPrice(m_bidsBook);
}

MixedOrderBook::PriceType MixedOrderBook::getBestAskPrice() const
{
  return getBestPrice(m_asksBook);
}

std::ostream& operator<<(std::ostream& ostr, const MixedOrderBook& book)
{
  return dumpBook(book, ostr, 10);
}

MixedOrderBook::Item::Item(const Item &c)
  : exchange(c.exchange)
  , price(c.price)
  , priceWithFee(c.priceWithFee)
  , amount(c.amount)
  , instrument(c.instrument)
  , type(c.type)
  , m_reserved(c.m_reserved.load())
{ }

MixedOrderBook::Item &MixedOrderBook::Item::operator=(const Item &c) {
  exchange = c.exchange;
  price = c.price;
  priceWithFee = c.priceWithFee;
  amount = c.amount;
  instrument = c.instrument;
  type = c.type;
  m_reserved = c.m_reserved.load();
  return *this;
}

bool MixedOrderBook::Item::reserve(AmountType amountToReserve) const {
  return exchange->reserveItem(instrument, this->type, price, amount, FixedNumber(amountToReserve.toDouble()));
}

bool MixedOrderBook::Item::unreserve(AmountType amountToReserve) const {
  return exchange->unreserveItem(instrument, this->type, price, FixedNumber(amountToReserve.toDouble()));
}

void MixedOrderBook::Item::unreserveAll() const
{
  double reserve = exchange->getItemReserve(instrument, type, price);
  unreserve(reserve);
}

MixedOrderBook::AmountType MixedOrderBook::Item::reserved() const {
  double reserve = exchange->getItemReserve(instrument, type, price);
  return reserve;
}

std::ostream& operator<<(std::ostream& ostr, const MixedOrderBook::Item& item)
{
  return ostr 
    << item.price.toString() << " " 
    << item.priceWithFee.toString() << " " 
    << (item.exchange ? item.exchange->getName() : "null") << " " 
    << item.amount.toString();
}

}

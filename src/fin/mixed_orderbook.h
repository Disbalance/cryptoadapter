/***************************************************
 * mixed_orderbook.h
 * Created on Mon, 01 Oct 2018 12:31:42 +0000 by sandro
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include "orderbook.h"
#include "exchange.h"
#include <atomic>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <iosfwd>

namespace fin {

/**
 *
 */
class MixedOrderBook {
public:
  using ExchangeType = interface::TradeExchangeConnector*;
  using PriceType = FixedNumber;
  using AmountType = FixedNumber;

  struct ExchangePriceIdx{};
  struct PriceIdx{};
  struct PriceWithFeeIdx{};
  struct ExchangeIdx{};

  struct Item {
    ExchangeType exchange;
    PriceType price;
    PriceType priceWithFee;
    AmountType amount;
    InstrumentHandle instrument;
    OrderDir type;
    long timestamp;
    Item() = default;
    Item(const Item &);
    Item(ExchangeType exchange, const OrderBookEntry &entry, FixedNumber priceWithFee, OrderDir type, InstrumentHandle instrument)
      : exchange(exchange)
      , price(entry.price)
      , priceWithFee(priceWithFee)
      , amount(entry.amount)
      , instrument(instrument)
      , type(type)
      , timestamp(entry.timestamp)
    { } 
    Item &operator=(const Item &);

    bool reserve(AmountType amountToReserve) const;
    AmountType reserved() const;
    void unreserveAll() const;
    bool unreserve(AmountType amountToUnreserve) const;

  private:
    mutable std::atomic<double> m_reserved;
  };

private:
  template<typename SortingOrder>
  class MixedOrderBookSide {
  public:
    using Container = boost::multi_index_container<
                              Item, 
                              boost::multi_index::indexed_by<
                                boost::multi_index::ordered_unique<
                                  boost::multi_index::tag<ExchangePriceIdx>,
                                  boost::multi_index::composite_key<
                                    Item,
                                    boost::multi_index::member<Item, ExchangeType, &Item::exchange>,
                                    boost::multi_index::member<Item, PriceType, &Item::price> 
                                  >
                                >,
                                boost::multi_index::ordered_non_unique<
                                  boost::multi_index::tag<PriceIdx>,
                                  boost::multi_index::member<
                                    Item, 
                                    PriceType, &Item::price>,
                                  SortingOrder
                                >,
                                boost::multi_index::ordered_non_unique<
                                  boost::multi_index::tag<PriceWithFeeIdx>,
                                  boost::multi_index::member<
                                    Item, 
                                    PriceType, &Item::priceWithFee>,
                                  SortingOrder
                                >,
                                boost::multi_index::hashed_non_unique<
                                  boost::multi_index::tag<ExchangeIdx>,
                                  boost::multi_index::member<
                                    Item, 
                                    ExchangeType, &Item::exchange>
                                >
                              >
                            >;

    using SortedByPriceType = typename Container::template index<PriceIdx>::type;
    using SortedByPriceWithFeeType = typename Container::template index<PriceWithFeeIdx>::type;

    const SortedByPriceType& getSortedByPrice() const
    {
      return m_data.template get<PriceIdx>();
    }
    const SortedByPriceWithFeeType& getSortedByPriceWithFee() const
    {
      return m_data.template get<PriceWithFeeIdx>();
    }

    Container& getData()
    {
      return m_data;
    }

    const Container& getData() const
    {
      return m_data;
    }
    void clear()
    {
      m_data.template get<ExchangeIdx>().clear();
    }

  private:
    Container m_data;
  };

  using AsksOrderBook = MixedOrderBookSide<std::less<PriceType>> ;
  using BidsOrderBook = MixedOrderBookSide<std::greater<PriceType>>; 

public:
  using AsksContainer = AsksOrderBook::Container;
  using BidsContainer = BidsOrderBook::Container; 

  using AsksSortedByPriceWithFeeType = AsksOrderBook::SortedByPriceWithFeeType;
  using BidsSortedByPriceWithFeeType = BidsOrderBook::SortedByPriceWithFeeType; 

  MixedOrderBook(InstrumentHandle instrument = NoInstrument)
    : m_instrument(instrument)
  {}

  void setInstrument(InstrumentHandle instrument);
  InstrumentHandle getInstrument() const;
  //void update(ExchangeType exchange, OrderBookEntry entry);
  void update(ExchangeType exchange, OrderBookEntry entry, double fee);
  void clear(ExchangeType exchange);

  //this function filters instrument!!!
  template<typename Iterator>
  void batchUpdate(ExchangeType exchange, Iterator first, Iterator last, double fee)
  {
    while(first != last) {
      if(m_instrument == first->instrument) {
        update(exchange, *first, fee);
      }
      ++first;
    }
  }

  template<typename Iterator>
  void snapshot(ExchangeType exchange, Iterator first, Iterator last)
  {
    clear(exchange);
    batchUpdate(exchange, first, last);
  }

  void clear();

  AsksContainer& getAsks()
  {
    return m_asksBook.getData();
  }

  BidsContainer& getBids()
  {
    return m_bidsBook.getData();
  }

  const AsksContainer& getAsks() const
  {
    return m_asksBook.getData();
  }

  const BidsContainer& getBids() const
  {
    return m_bidsBook.getData();
  }

  const AsksSortedByPriceWithFeeType& getAsksSortedByPriceWithFee() const
  {
    return m_asksBook.getSortedByPriceWithFee();
  }
  const BidsSortedByPriceWithFeeType& getBidsSortedByPriceWithFee() const
  {
    return m_bidsBook.getSortedByPriceWithFee();
  }

  PriceType getBestBidPrice() const;

  PriceType getBestAskPrice() const;

  std::ostream& dump(std::ostream& ostr, size_t n = 10) const;
  void dumpToLog(size_t n = 10) const;
  
private:
  InstrumentHandle m_instrument;

  AsksOrderBook m_asksBook;
  BidsOrderBook m_bidsBook;
  friend std::ostream& operator<<(std::ostream& ostr, const Item& item);
};

std::ostream& operator<<(std::ostream& ostr, const MixedOrderBook& book);

std::ostream& operator<<(std::ostream& ostr, const MixedOrderBook::Item& item);

}

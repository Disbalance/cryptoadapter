/**
 * @file
 * @brief Class for fixed point and fixed accuracy representation
 *
 */
#pragma once

#include <stdexcept>
#include <climits>
#include <string>
#include <vector>
#include <boost/optional.hpp>

#define DEFAULT_ACCURACY  8

//!< Classes and functions related to finances
namespace fin {

class TenPowers {
public:
  TenPowers();
  inline long long get(long p) {
    if(p < 0) return 0;
    if(__builtin_expect(p >= (long)m_pow10Size, 0)) {
      throw std::out_of_range("pow10 argument out of range!");
    }
    return m_pow10Ptr[p];
  }

private:
  std::vector<long long> m_pow10;
  long long *m_pow10Ptr;
  size_t m_pow10Size;
};

//! Number with specified fixed accuracy
class FixedNumber {
public:
  FixedNumber(); //!< Default constructor
  FixedNumber(int, int accuracy = DEFAULT_ACCURACY); //!< Construct from int
  FixedNumber(double, int accuracy = DEFAULT_ACCURACY); //!< Construct from double with specified accuracy
  FixedNumber(const std::string &); //!< Construct from string
  FixedNumber(const char *); //!< Construct from string
  FixedNumber(FixedNumber &&) = default; //!< Copy constructor
  FixedNumber(const FixedNumber &) = default; //!< Copy constructor

  FixedNumber &assign(int, int accuracy = DEFAULT_ACCURACY); //!< Assign from int
  FixedNumber &assign(double, int accuracy = DEFAULT_ACCURACY); //!< Assign from double with specified accuracy
  FixedNumber &assign(const std::string &); //!< Assign from string
  FixedNumber &assign(const char *); //!< Assign from string
  FixedNumber &assign(const FixedNumber &); //!< Copy from other fixed number

  void swap(FixedNumber &); //!< Swap two values

  double toDouble() const; //!< Convert to double
  std::string toString() const; //!< Convert to string

  FixedNumber &setAccuracy(int accuracy);

  // Assignment operator
  FixedNumber &operator =(const FixedNumber &) = default; //!< Assignment operator overload

  // Comparison operators
  bool operator <(const FixedNumber &) const; //!< Comparison operator overload
  bool operator >(const FixedNumber &) const; //!< Comparison operator overload
  bool operator ==(const FixedNumber &) const; //!< Comparison operator overload
  bool operator !=(const FixedNumber &) const; //!< Comparison operator overload

  bool operator <(double) const; //!< Comparison operator overload
  bool operator >(double) const; //!< Comparison operator overload
  bool operator ==(double) const; //!< Comparison operator overload
  bool operator !=(double) const; //!< Comparison operator overload

  bool operator <(int) const; //!< Comparison operator overload
  bool operator >(int) const; //!< Comparison operator overload
  bool operator ==(int) const; //!< Comparison operator overload
  bool operator !=(int) const; //!< Comparison operator overload

  // Arithmetic operators
  FixedNumber operator -() const;
  FixedNumber operator +(const FixedNumber &) const;
  FixedNumber operator -(const FixedNumber &) const;
  FixedNumber &operator +=(const FixedNumber &);
  FixedNumber &operator -=(const FixedNumber &);
  FixedNumber operator *(const FixedNumber &) const;
  FixedNumber operator /(const FixedNumber &) const;
  FixedNumber &operator *=(const FixedNumber &);
  FixedNumber &operator /=(const FixedNumber &);

  friend std::ostream &operator <<(std::ostream &out, const fin::FixedNumber &num);

private:
  void initFromString(const char *);
  void initFromDouble(double, int accuracy = DEFAULT_ACCURACY);

  double m_doubleValue;
  long m_int;
  long m_base;
  long m_exp;

  static TenPowers power10;
};

}

fin::FixedNumber operator "" _d(const char *, size_t);
fin::FixedNumber operator "" _d(unsigned long long);
fin::FixedNumber operator "" _d(long double);


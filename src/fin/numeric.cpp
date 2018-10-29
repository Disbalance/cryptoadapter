/***************************************************
 * numeric.cpp
 * Created on Wed, 26 Sep 2018 14:04:42 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#define GNU_SOURCE_
#include <string.h>
#include "numeric.h"

namespace fin {

/**
 * Constructs default FixedNumber object with value 0
 */
FixedNumber::FixedNumber()
  : m_doubleValue(0)
  , m_int(0)
  , m_base(0)
  , m_exp(0)
{ }

/**
 * Constructs FixedNumber object from an int
 * @param number The number to construct from
 * @param accuracy The number of digits after decimal point
 */
FixedNumber::FixedNumber(int number, int accuracy)
  : m_doubleValue(number)
{
  m_int = number;
  m_base = 0;
  m_exp = accuracy;
}

/**
 * Constructs FixedNumber object from a double with specified accuracy
 * @param number The number to construct from
 * @param accuracy The number of digits after decimal point
 */
FixedNumber::FixedNumber(double number, int accuracy)
  : m_doubleValue(number)
{
  initFromDouble(number, accuracy);
}

/**
 * Constructs FixedNumber object from a string
 * @param number The string to construct from
 */
FixedNumber::FixedNumber(const std::string &number)
{
  initFromString(number.c_str());
}

/**
 * Constructs FixedNumber object from a string
 * @param number The string to construct from
 */
FixedNumber::FixedNumber(const char *number)
{
  initFromString(number);
}

/**
 * Assigns FixedNumber object from an int
 * @param val The number to construct from
 * @param accuracy The number of digits after decimal point
 */
FixedNumber &FixedNumber::assign(int val, int accuracy)
{
  m_doubleValue = val;
  initFromDouble(val);
  return *this;
}

/**
 * Assigns FixedNumber object from a double with specified accuracy
 * @param val The number to construct from
 * @param accuracy The number of digits after decimal point
 */
FixedNumber &FixedNumber::assign(double val, int accuracy)
{
  m_doubleValue = val;
  initFromDouble(val);
  return *this;
}

/**
 * Assigns FixedNumber object from a string
 * @param val The string to construct from
 */
FixedNumber &FixedNumber::assign(const char *val)
{
  initFromString(val);
  return *this;
}

/**
 * Assigns FixedNumber object from a string
 * @param val The string to construct from
 */
FixedNumber &FixedNumber::assign(const std::string &val)
{
  initFromString(val.c_str());
  return *this;
}

/**
 * Assigns a FixedNumber object from a copy
 * @param val The FixedNumber object to copy from
 */
FixedNumber &FixedNumber::assign(const FixedNumber &val)
{
  m_doubleValue = val.m_doubleValue;
  m_int = val.m_int;
  m_base = val.m_base;
  m_exp = val.m_exp;
  return *this;
}

/**
 * Swap current object value with another FixedNumber object
 * @param val The FixedNumber object to swap with
 */
void FixedNumber::swap(FixedNumber &val) {
  double doubleValue = m_doubleValue;
  m_doubleValue = val.m_doubleValue;
  val.m_doubleValue = doubleValue;
  long int_part = m_int;
  m_int = val.m_int;
  val.m_int = int_part;
  long base = m_base;
  m_base = val.m_base;
  val.m_base = base;
  int exp = m_exp;
  m_exp = val.m_exp;
  val.m_exp = exp;
}

void FixedNumber::initFromString(const char *str) {
  long len;
  const char *num = str;
  const char *decimal = strchrnul(num, '.');
  len = strlen(decimal);
  m_exp = len - 1;

  if(m_exp >= 0) {
    sscanf(num, "%ld.%ld", &m_int, &m_base);
    if(m_int < 0) {
      m_base = -m_base;
    }
    m_doubleValue = ((double)m_int) + ((double)m_base) / ((double)power10.get(m_exp));
  } else {
    sscanf(num, "%ld", &m_int);
    m_base = 0;
    m_doubleValue = ((double)m_int);
  }
}

FixedNumber &FixedNumber::setAccuracy(int accuracy) {
  if(m_exp < 0) {
    m_base = 0;
    m_exp = accuracy;
  }
  else if(accuracy < 0) {
    m_base = 0;
    m_exp = accuracy;
  }
  else if(accuracy < m_exp) {
    m_base /= power10.get(m_exp - accuracy);
  }
  else if(accuracy > m_exp) {
    m_base *= power10.get(accuracy - m_exp);
  }
  return *this;
}

void FixedNumber::initFromDouble(double val, int accuracy) {
  m_doubleValue = val;
  m_int = val;
  m_exp = accuracy;
  m_base = (double)(val - m_int) * (double)power10.get(m_exp);

  while(m_exp > 0 && m_base % 10 == 0) {
    m_base /= 10;
    m_exp --;
  }
}

/**
 * Convert FixedNumber to double
 */
double FixedNumber::toDouble() const
{
  return m_doubleValue;
}

/**
 * Convert FixedNumber to std::string
 */
std::string FixedNumber::toString() const
{
  char buffer[64];
  if(m_exp >= 0) {
    sprintf(buffer, "%ld.%0*ld", m_int, (int)m_exp, (m_base < 0 ? -m_base : m_base));
  } else {
    sprintf(buffer, "%ld", m_int);
  }
  return std::string(buffer);
}

bool FixedNumber::operator <(const FixedNumber &b) const
{
  long number1, number2;
  if(b.m_exp > m_exp) {
    int diff = b.m_exp - m_exp;
    number1 = m_base * power10.get(diff);
    number2 = b.m_base;
  } else {
    int diff = m_exp - b.m_exp;
    number1 = m_base;
    number2 = b.m_base * power10.get(diff);
  }
  return (m_int < b.m_int) || ((m_int == b.m_int) && (number1 < number2));
}

bool FixedNumber::operator >(const FixedNumber &b) const
{
  long number1, number2;
  if(b.m_exp > m_exp) {
    int diff = b.m_exp - m_exp;
    number1 = m_base * power10.get(diff);
    number2 = b.m_base;
  } else {
    int diff = m_exp - b.m_exp;
    number1 = m_base;
    number2 = b.m_base * power10.get(diff);
  }
  return (m_int > b.m_int) || ((m_int == b.m_int) && (number1 > number2));
}

bool FixedNumber::operator ==(const FixedNumber &b) const
{
  long number1, number2;
  if(b.m_exp > m_exp) {
    int diff = b.m_exp - m_exp;
    number1 = m_base * power10.get(diff);
    number2 = b.m_base;
  } else {
    int diff = m_exp - b.m_exp;
    number1 = m_base;
    number2 = b.m_base * power10.get(diff);
  }
  return (m_int == b.m_int) && (number1 == number2);
}

bool FixedNumber::operator !=(const FixedNumber &b) const
{
  return !(*this == b);
}

bool FixedNumber::operator <(double val) const
{
  return m_doubleValue < val;
}

bool FixedNumber::operator >(double val) const
{
  return m_doubleValue > val;
}

bool FixedNumber::operator ==(double val) const
{
  return m_doubleValue == val;
}

bool FixedNumber::operator !=(double val) const
{
  return !(*this == val);
}

bool FixedNumber::operator <(int val) const
{
  return m_int < val;
}

bool FixedNumber::operator >(int val) const
{
  return (m_int > val) || ((m_int == val) && (m_base > 0));
}

bool FixedNumber::operator ==(int val) const
{
  return (m_int == val) && (m_base == 0);
}

bool FixedNumber::operator !=(int val) const
{
  return !(*this == val);
}

FixedNumber FixedNumber::operator -() const
{
  FixedNumber result;
  result.m_int = -m_int;
  result.m_base = -m_base;
  result.m_exp = m_exp;
  return result;
}

FixedNumber FixedNumber::operator +(const FixedNumber &op) const
{
  FixedNumber result(*this);
  result += op;
  return result;
}

FixedNumber FixedNumber::operator -(const FixedNumber &op) const
{
  FixedNumber result(*this);
  result -= op;
  return result;
}

FixedNumber &FixedNumber::operator +=(const FixedNumber &op)
{
  int exp = std::max(m_exp, op.m_exp);

  m_int += op.m_int;
  m_base = (m_base * power10.get(exp - m_exp)) + (op.m_base * power10.get(exp - op.m_exp));

  if(m_int < 0 && m_base < -power10.get(exp)) {
    -- m_int;
    m_base += power10.get(exp);
  }
  else if(m_int > 0 && m_base < 0) {
    -- m_int;
    m_base += power10.get(exp);
  }
  else if(m_int > 0 && m_base > power10.get(exp)) {
    ++ m_int;
    m_base -= power10.get(exp);
  }
  else if(m_int < 0 && m_base > 0) {
    ++ m_int;
    m_base -= power10.get(exp);
  }
  m_exp = exp;
  return *this;
}

FixedNumber &FixedNumber::operator -=(const FixedNumber &op)
{
  m_int -= op.m_int;

  long exp = std::max(m_exp, op.m_exp);
  m_base = (m_base * power10.get(exp - m_exp)) - (op.m_base * power10.get(exp - op.m_exp));

  if(m_int < 0 && m_base < -power10.get(exp)) {
    -- m_int;
    m_base += power10.get(exp);
  }
  else if(m_int > 0 && m_base < 0) {
    -- m_int;
    m_base += power10.get(exp);
  }
  else if(m_int > 0 && m_base > power10.get(exp)) {
    ++ m_int;
    m_base -= power10.get(exp);
  }
  else if(m_int < 0 && m_base > 0) {
    ++ m_int;
    m_base -= power10.get(exp);
  }
  m_exp = exp;
  return *this;
}

FixedNumber FixedNumber::operator *(const FixedNumber &op) const
{
  FixedNumber result(*this);
  return result *= op;
}

FixedNumber FixedNumber::operator /(const FixedNumber &op) const
{
  FixedNumber result(*this);
  return result /= op;
}

FixedNumber &FixedNumber::operator *=(const FixedNumber &op)
{
  FixedNumber result;
  result.m_int = m_int * op.m_int;
  result.m_base = m_int * op.m_base * power10.get(m_exp) + op.m_int * m_base * power10.get(op.m_exp) + m_base * op.m_base;
  result.m_exp = m_exp + op.m_exp < 0 ? -1 : m_exp + op.m_exp;
  long divisor = (result.m_exp < 0 ? 1 : power10.get(result.m_exp));
  result.m_int += result.m_base / divisor;
  result.m_base %= divisor;
  m_int = result.m_int;
  m_exp = result.m_exp;
  m_base = result.m_base;
  return *this;
}

inline FixedNumber &FixedNumber::operator /=(const FixedNumber &op)
{
  long exp1 = m_exp < 0 ? 0 : m_exp;
  long exp2 = op.m_exp < 0 ? 0 : op.m_exp;
  long exp = (exp1 + exp2);
  long mine = m_int * power10.get(exp) + m_base * power10.get(exp2);
  long theirs = op.m_int * power10.get(exp2) + op.m_base;
  long result = (mine + (theirs >> 1)) / theirs;
  ldiv_t n = ldiv(result, power10.get(exp1));
  m_int = n.quot;
  m_base = n.rem;
  return *this;
}

TenPowers::TenPowers()
{
  long long power = 1;
  for(int i = 0; i < 19; i ++) {
    m_pow10.push_back(power);
    power *= 10;
  }
  m_pow10Ptr = m_pow10.data();
  m_pow10Size = m_pow10.size();
}

std::ostream &operator <<(std::ostream &out, const fin::FixedNumber &num)
{
  return out << num.toString();
}

TenPowers FixedNumber::power10;

}

fin::FixedNumber operator "" _d(const char *op, size_t size) {
  return fin::FixedNumber(op);
}

fin::FixedNumber operator "" _d(unsigned long long op) {
  return fin::FixedNumber((double)op);
}

fin::FixedNumber operator "" _d(long double op) {
  return fin::FixedNumber((double)op);
}

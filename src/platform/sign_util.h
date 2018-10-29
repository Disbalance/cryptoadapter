/***************************************************
 * sign.h
 * Created on Mon, 08 Oct 2018 15:11:06 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <string>
#include <vector>
#include <openssl/hmac.h>
#include <openssl/md5.h>

namespace platform {

class SignUtil {
public:
  typedef const EVP_MD *Encoder;

  static void hmacSign(std::vector<char> &buffer, const std::string &data, const std::string &key, const Encoder encoder);
  template<const char DIGITS[17]>
  static void hex(std::string &ret, const std::vector<char> &buffer);
  static void base64Encode(std::string &dst, const std::vector<char> &src);
  static void md5Sign(std::vector<char> &buffer, const std::string &data);

  static const char CAPITAL[17];
  static const char SMALL[17];
};

}

using platform::SignUtil;


/***************************************************
 * sign_util.cpp
 * Created on Mon, 08 Oct 2018 15:16:09 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "sign_util.h"

namespace platform {

const char SignUtil::CAPITAL[17] = "0123456789ABCDEF";
const char SignUtil::SMALL[17] = "0123456789abcdef";

void SignUtil::hmacSign(std::vector<char> &buffer, const std::string &data, const std::string &key, const SignUtil::Encoder encoder)
{
  buffer.resize(EVP_MAX_MD_SIZE);
  unsigned int digestLen = buffer.size();

  HMAC(encoder, key.c_str(), key.size(),
    (const unsigned char *)data.c_str(), data.size(), (unsigned char*)buffer.data(), &digestLen);
  buffer.resize(digestLen);
}

void SignUtil::md5Sign(std::vector<char> &buffer, const std::string &data)
{
  buffer.resize(16);

  MD5((const unsigned char *)data.c_str(), data.size(), (unsigned char *)buffer.data());
}

template<const char DIGITS[17]>
void SignUtil::hex(std::string &ret, const std::vector<char> &buffer) {
  ret.resize(buffer.size() * 2, 0);

  for (unsigned int i = 0; i < buffer.size(); i++) {
    ret[i * 2] = DIGITS[(buffer[i]&0xf0) >> 4];
    ret[i * 2 + 1] = DIGITS[(buffer[i]&0xf)];
  }
}

static const char base64Code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"
                                 "0123456789+/";

void SignUtil::base64Encode(std::string &dst, const std::vector<char> &src) {
  unsigned int s, i;
  char curval = 0;

  dst.resize(0);

  for (s = 0; s < src.size(); s ++) {
    switch(s % 3) {
    case 0:
      curval = ((src[s] & (0xff << 2)) >> 2);
      dst += base64Code[(int)curval];
      curval = ((src[s] & (0xff >> 6)) << 4);
      break;
    case 1:
      curval |= ((src[s] & (0xff << 4)) >> 4);
      dst += base64Code[(int)curval];
      curval = ((src[s] & (0xff >> 4)) << 2);
      break;
    case 2:
      curval |= ((src[s] & (0xff << 6)) >> 6);
      dst += base64Code[(int)curval];
      curval = ((src[s] & (0xff >> 2)) << 0);
      dst += base64Code[(int)curval];
    }
  }

  if(src.size() % 3) {
    dst += base64Code[(int)curval];

    for(i = 0; i < (3 - (src.size() % 3)); i ++) {
      dst += '=';
    }
  }
}

template void SignUtil::hex<SignUtil::CAPITAL>(std::string &ret, const std::vector<char> &buffer);
template void SignUtil::hex<SignUtil::SMALL>(std::string &ret, const std::vector<char> &buffer);

}

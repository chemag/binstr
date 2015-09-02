// Copyright Google Inc. Apache 2.0.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif

#include "binstr.h"

#include <stdarg.h>  // for va_list
#include <stdint.h>  // for *int_*
#include <stdio.h>  // for sscanf
#include <stdlib.h>  // for strtoull

#include <iostream>
#include <sstream>
#include <string>

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif  // MAX
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif  // MIN

static void trim(std::string *s);

// set <len> bits at <bitindex> position in buf using the
// <len> right-most (least-significant) bits in <val>
static void bit_set(char *buf, int bitindex, uint8_t val, int len) {
  if (len == 0)
    return;
  uint8_t mask;
  uint8_t *byte = (uint8_t *)(buf + bitindex / 8);
  int lshift = 8 - len - (bitindex % 8);
  if (lshift >= 0)
    mask = ((1 << len) - 1) << lshift;
  else
    mask = ((1 << len) - 1) >> (-lshift);
  *byte &= ~mask;
  if (lshift >= 0)
    *byte |= (val << lshift) & mask;
  else
    *byte |= (val >> (-lshift)) & mask;
  if (lshift < 0) {
    byte = (uint8_t *)(buf + bitindex / 8 + 1);
    lshift += 8;
    mask = ((1 << len) - 1) << lshift;
    *byte &= ~mask;
    *byte |= val << lshift;
  }
}

static int sscanf_item(const std::string &item, int bitlen,
                       char *buf, int buflen, int bitindex) {
  int biti = 0;
  if (item.length() > 1 &&
      item.at(0) == '0' &&
      (item.at(1) == 'x' ||
       item.at(1) == 'b' ||
       (item.at(1) >= '0' && item.at(1) <= '7'))) {
    int digitlen = 0;
    int start_index = 0;
    if (item.at(1) == 'x') {
      digitlen = 4;
      start_index = 2;
    } else if (item.at(1) == 'b') {
      digitlen = 1;
      start_index = 2;
    } else if (item.at(1) >= '0' && item.at(1) <= '7') {
      digitlen = 3;
      start_index = 1;
    }

    int actual_bitlen = (item.length() - start_index) * digitlen;
    if (bitlen == -1) {
      bitlen = actual_bitlen;
    } else if (bitlen > actual_bitlen) {
      while (bitlen > actual_bitlen) {
        // prepend with zeros
        bit_set(buf, bitindex+biti, 0, 1);
        biti += 1;
        bitlen -= 1;
      }
    }

    // parse bytes
    for (unsigned int i = start_index; i < item.length(); ++i) {
      if (digitlen == 4 && !isxdigit(item.at(i)))
        // hex parsing error
        return -1;
      if (digitlen == 1 && item.at(i) != '0' && item.at(i) != '1')
        // bin parsing error
        return -1;
      if (digitlen == 3 && (item.at(i) < '0' || item.at(i) > '7'))
        // oct parsing error
        return -1;
      if (bitindex + biti + digitlen > buflen * 8)
        // not enough space
        return -1;
      uint8_t val = isdigit(item.at(i)) ? (item.at(i) - '0') :
                    (tolower(item.at(i)) - 'a') + 10;
      int len = digitlen;
      if (bitlen < actual_bitlen) {
        // drop part/all of the digit
        int dropbits = MIN(digitlen, actual_bitlen - bitlen);
        len -= dropbits;
        bitlen += dropbits;
      }
      bit_set(buf, bitindex+biti, val, len);
      biti += len;
    }
  } else {
    // decimal bytes
    if (bitlen == -1)
      // decimal requires a fixed bit length
      return -1;
    if (bitlen < 0 || bitlen > 64)
      // strtoull only accepts 64-bit integers
      return -1;
    uint64_t value;
    value = strtoull(item.c_str(), NULL, 10);
    // set value (up to) 8-bits at a time
    for (int i = bitlen; i > 0; i -= 8) {
      int len = (i < 8) ? i : 8;
      uint8_t mask = 0xff >> (8 - len);
      int rshift = i - 8;
      uint8_t val = 0;
      if (rshift >= 0)
        val = (value >> rshift) & mask;
      else
        val = value & mask;
      bit_set(buf, bitindex+biti, val, len);
      biti += len;
    }
  }

  return biti;
}

int binstr_parse(const char *in, char *buf, int buflen) {
  int bitindex = 0;

  // break lines
  std::stringstream lss(in);
  std::string line;

  int line_number = 0;
  while (std::getline(lss, line, '\n')) {
    ++line_number;
    // trim lines
    trim(&line);
    if (line.empty())
      continue;
    std::stringstream iss(line);
    std::string item;
    // TODO(chema): delim must support any blank
    while (std::getline(iss, item, ' ')) {
      if (item.empty())
        continue;
      // a '#' causes the rest of the line to be a comment
      if (item.at(0) == '#')
        // comment
        break;
      // parse item
      int index = 0;
      // look for repeated items ("*<rep>*item")
      int rep = 1;
      if (sscanf(item.c_str(), "*%d*%n", &rep, &index) < 1)
        rep = 1;
      int bitlen = -1;
      // look for an explicit length ("{len}...")
      int i = 0;
      if (sscanf(item.substr(index).c_str(), "{%d}%n", &bitlen, &i) < 1)
        bitlen = -1;
      index += i;
      for (int j = 0; j < rep; ++j) {
        int ret = sscanf_item(item.substr(index).c_str(), bitlen, buf, buflen,
                              bitindex);
        if (ret < 0)
          // parse error
          return -1;
        bitindex += ret;
      }
    }
  }
  // zero all bits until the next byte
  bit_set(buf, bitindex, 0, 8 - (bitindex % 8));

  return bitindex;
}

int binstr_snprintf(const char *in, char *buf, int buflen, ...) {
  char format[2048];
  va_list va;

  va_start(va, buflen);
  int ret = vsnprintf(format, sizeof(format), in, va);
  va_end(va);

  if (ret < 0)
    // vsnprintf error
    return ret;

  if (ret > sizeof(format))
    // insufficient space in format
    return -1;

  return binstr_parse(format, buf, buflen);
}

// trim from start
static void ltrim(std::string *s) {
  int pos = s->find_first_not_of(" \n\r\t");
  if (pos == std::string::npos)
    s->erase();
  else
    s->erase(0, pos);
}

// trim from end
static void rtrim(std::string *s) {
  int pos = s->find_last_not_of(" \n\r\t");
  if (pos == std::string::npos)
    s->erase();
  else
    s->erase(pos + 1);
}

// trim from both ends
static void trim(std::string *s) {
  rtrim(s);
  ltrim(s);
}

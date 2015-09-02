// Copyright Google Inc. Apache 2.0.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif

#include "binstr.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

#include <gtest/gtest.h>


const char kBinDoc1[] = R"(
  # bitlen prefix larger than value: 00000111
  {8}0b111
  # bitlen prefix shorter than value: 00001111
  {8}0b1100001111
  {7}0b10101 {9}07070
  # hex numbers
  0x001111111111111111111111
  0x012345678
  0x33
  # a binary number
  0b10101010
  # an octal number
  07777
  0xf
  # combine things
  0x4 0x4 0b1111
  0b1 {6}0x55 0b1
  # a decimal number with specific length (0x12345678)
  {32}305419896
  # hex numbers with specific length
  {17}0x11111
  {23}0xeeeee
  # bin/octal numbers with specific length
  {7}0b10101 {9}06440
)";

const char kBinDocBin1[] =
    "\x07"
    "\x0f"
    "\x2a\x38"
    "\x00\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11"
    "\x01\x23\x45\x67"
    "\x83"
    "\x3a"
    "\xaf\xff"
    "\xf4\x4f"
    "\xab"
    "\x12\x34\x56\x78"
    "\x88\x88"
    "\x8e\xee\xee"
    "\x2b\x20";

const char kIpHeader[] = R"(
  # version header_length service_type total_length
  {4}0x4 {4}5 0x00 {16}1500
  # identification evil dnf mf offset
  {16}0xcafe 0b0 0b0 0b0 {13}0
  # ttl protocol checksum
  {8}255 {8}17 {16}0
  # source addr
  {32}0x12345678
  # dst addr
  {32}0x9abcdef0
)";

const char kIpHeaderBin[] =
    "\x45\x00\x05\xdc"
    "\xca\xfe\x00\x00"
    "\xff\x11\x00\x00"
    "\x12\x34\x56\x78"
    "\x9a\xbc\xde\xf0";

TEST(BinstrTest, binstr_parse) {
  struct {
    int line;
    const char *in;
    const char *expected_out;
    int expected_bitlen;
  } test_arr[] = {
    // comments
    { __LINE__, "# hello world", "", 0 },
    { __LINE__, "    # hello world", "", 0 },
    // hex representation
    { __LINE__, "0x33", "\x33", 8 },
    { __LINE__, "0x0011111111111111", "\x00\x11\x11\x11\x11\x11\x11\x11", 64 },
    { __LINE__, "0x012345678", "\x01\x23\x45\x67\x80", 36 },
    // binary representation
    { __LINE__, "0b10", "\x80", 2 },
    { __LINE__, "0b10101010", "\xaa", 8 },
    // octal representation
    { __LINE__, "077777777", "\xff\xff\xff", 24 },
    // combined representations
    { __LINE__, "0x4 0x4 0b1111 0x4", "\x44\xf4", 16 },
    { __LINE__, "0b1 00 0x55 0b1 07", "\x85\x5f", 16 },
    { __LINE__, "0b1  0b1    0b1 0b1", "\xf0", 4 },
    // hex representation with specific length
    { __LINE__, "{16}0x2ffff", "\xff\xff", 16 },
    { __LINE__, "{17}0x2ffff", "\x7f\xff\x80", 17 },
    { __LINE__, "{18}0x2ffff", "\xbf\xff\xc0", 18 },
    { __LINE__, "{19}0x2ffff", "\x5f\xff\xe0", 19 },
    { __LINE__, "{20}0x2ffff", "\x2f\xff\xf0", 20 },
    { __LINE__, "{21}0x2ffff", "\x17\xff\xf8", 21 },
    { __LINE__, "{22}0x2ffff", "\x0b\xff\xfc", 22 },
    { __LINE__, "{23}0x2ffff", "\x05\xff\xfe", 23 },
    { __LINE__, "{24}0x2ffff", "\x02\xff\xff", 24 },
    { __LINE__, "{25}0x2ffff", "\x01\x7f\xff\x80", 25 },
    // bin/octal representations with specific length
    { __LINE__, "{8}0b111", "\x07", 8 },
    { __LINE__, "{8}0b1100001111", "\x0f", 8 },
    // combined representations
    { __LINE__, "{7}0b10101 {9}07070", "\x2a\x38", 16 },
    // decimal representations with specific length
    { __LINE__, "{1}0 # 0", "\x00", 1 },
    { __LINE__, "{13}0 # 0", "\x00\x00", 13 },
    { __LINE__, "{32}4294967295 # 0xffffffff", "\xff\xff\xff\xff", 32 },
    { __LINE__, "{64}18446744073709551615 # 0xffffffffffffffff",
      "\xff\xff\xff\xff\xff\xff\xff\xff", 64 },
    { __LINE__, "{64}18364758544493064720 # 0xfedcba9876543210",
      "\xfe\xdc\xba\x98\x76\x54\x32\x10", 64 },
    { __LINE__, "{32}305419896 # 0x12345678", "\x12\x34\x56\x78", 32 },
    { __LINE__, "{31}305419896 # 0x12345678", "\x24\x68\xac\xf0", 31 },
    { __LINE__, "{30}305419896 # 0x12345678", "\x48\xd1\x59\xe0", 30 },
    { __LINE__, "{29}305419896 # 0x12345678", "\x91\xa2\xb3\xc0", 29 },
    { __LINE__, "{28}305419896 # 0x12345678", "\x23\x45\x67\x80", 28 },
    { __LINE__, "{27}305419896 # 0x12345678", "\x46\x8a\xcf\x00", 27 },
    { __LINE__, "{26}305419896 # 0x12345678", "\x8d\x15\x9e\x00", 26 },
    { __LINE__, "{25}305419896 # 0x12345678", "\x1a\x2b\x3c\x00", 25 },
    { __LINE__, "{24}305419896 # 0x12345678", "\x34\x56\x78\x00", 24 },
    { __LINE__, "{23}305419896 # 0x12345678", "\x68\xac\xf0\x00", 23 },
    { __LINE__, "{22}305419896 # 0x12345678", "\xd1\x59\xe0\x00", 22 },
    { __LINE__, "{21}305419896 # 0x12345678", "\xa2\xb3\xc0\x00", 21 },
    { __LINE__, "{20}305419896 # 0x12345678", "\x45\x67\x80\x00", 20 },
    { __LINE__, "{19}305419896 # 0x12345678", "\x8a\xcf\x00\x00", 19 },
    { __LINE__, "{18}305419896 # 0x12345678", "\x15\x9e\x00\x00", 18 },
    { __LINE__, "{17}305419896 # 0x12345678", "\x2b\x3c\x00\x00", 17 },
    { __LINE__, "{16}305419896 # 0x12345678", "\x56\x78\x00\x00", 16 },
    { __LINE__, "{15}305419896 # 0x12345678", "\xac\xf0\x00\x00", 15 },
    { __LINE__, "{14}305419896 # 0x12345678", "\x59\xe0\x00\x00", 14 },
    { __LINE__, "{13}305419896 # 0x12345678", "\xb3\xc0\x00\x00", 13 },
    { __LINE__, "{12}305419896 # 0x12345678", "\x67\x80\x00\x00", 12 },
    { __LINE__, "{11}305419896 # 0x12345678", "\xcf\x00\x00\x00", 11 },
    { __LINE__, "{10}305419896 # 0x12345678", "\x9e\x00\x00\x00", 10 },
    { __LINE__, "{9}305419896 # 0x12345678", "\x3c\x00\x00\x00", 9 },
    { __LINE__, "{8}305419896 # 0x12345678", "\x78\x00\x00\x00", 8 },
    { __LINE__, "{7}305419896 # 0x12345678", "\xf0\x00\x00\x00", 7 },
    { __LINE__, "{6}305419896 # 0x12345678", "\xe0\x00\x00\x00", 6 },
    { __LINE__, "{5}305419896 # 0x12345678", "\xc0\x00\x00\x00", 5 },
    { __LINE__, "{4}305419896 # 0x12345678", "\x80\x00\x00\x00", 4 },
    { __LINE__, "{3}305419896 # 0x12345678", "\x00\x00\x00\x00", 3 },
    { __LINE__, "{2}305419896 # 0x12345678", "\x00\x00\x00\x00", 2 },
    { __LINE__, "{1}305419896 # 0x12345678", "\x00\x00\x00\x00", 1 },
    { __LINE__, "{0}305419896 # 0x12345678", "\x00\x00\x00\x00", 0 },
    // bin, octal, and hex allow unlimited lengths
    { __LINE__, "{72}0xfedcba9876543210",
      "\x00\xfe\xdc\xba\x98\x76\x54\x32\x10", 72 },
    { __LINE__, "{80}0b1", "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 80 },
    { __LINE__, "{200}07",
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x07", 200 },
    // decimal strings are limited to 64 bits
    { __LINE__, "{65}36893488147419103231 # 0x1ffffffffffffffff", "", -1 },
    // combined representations
    { __LINE__, "0b1 {31}305419896 # 0x12345678", "\x92\x34\x56\x78", 32 },
    // repeated items
    { __LINE__, "*0*0xff", "", 0 },
    { __LINE__, "*-1*0xff", "", 0 },
    { __LINE__, "*8*0xff", "\xff\xff\xff\xff\xff\xff\xff\xff", 64 },
    { __LINE__, "*8*{7}0x7f", "\xff\xff\xff\xff\xff\xff\xff", 56 },
    { __LINE__, "*8*{6}0x3f", "\xff\xff\xff\xff\xff\xff", 48 },
  };

  char binbuf[1024];
  for (const auto &test_item : test_arr) {
    int bitlen = binstr_parse(test_item.in, binbuf, sizeof(binbuf));
    EXPECT_EQ(bitlen, test_item.expected_bitlen) <<
        "line " << test_item.line << ": \"" << test_item.in << "\"";
    EXPECT_EQ(0, memcmp(binbuf, test_item.expected_out, (bitlen + 7) / 8)) <<
        "line " << test_item.line << ": \"" << test_item.in << "\"";
  }
}

TEST(BinstrTest, binstr_parse_multiline) {
  char binbuf[1024];
  int bitlen = binstr_parse(kBinDoc1, binbuf, sizeof(binbuf));
  EXPECT_EQ(bitlen, 8 * (sizeof(kBinDocBin1) - 1));
  EXPECT_EQ(0, memcmp(binbuf, kBinDocBin1, (sizeof(kBinDocBin1) - 1)));
}

TEST(BinstrTest, binstr_ip_header) {
  char binbuf[1024];
  int bitlen = binstr_parse(kIpHeader, binbuf, sizeof(binbuf));
  EXPECT_EQ(bitlen, 8 * (sizeof(kIpHeaderBin) - 1));
  EXPECT_EQ(0, memcmp(binbuf, kIpHeaderBin, (sizeof(kIpHeaderBin) - 1)));
}

TEST(BinstrTest, binstr_snprintf) {
  char binbuf[1024];
  int bitlen;

  const char format_str[] = "0b1 {6}%d 0b1";
  // 1 000000 1
  bitlen = binstr_snprintf(format_str, binbuf, sizeof(binbuf), 0);
  EXPECT_EQ(bitlen, 8);
  EXPECT_EQ(binbuf[0], '\x81');
  // 1 111111 1
  bitlen = binstr_snprintf(format_str, binbuf, sizeof(binbuf), 0x3f);
  EXPECT_EQ(bitlen, 8);
  EXPECT_EQ(binbuf[0], '\xff');
  // 1 010101 1
  bitlen = binstr_snprintf(format_str, binbuf, sizeof(binbuf), 0x15);
  EXPECT_EQ(bitlen, 8);
  EXPECT_EQ(binbuf[0], '\xab');

  // check inttypes
  const char format_str2[] = "0b1 {6}%" PRIu32 " 0b1";
  uint32_t v = 0x3f;
  bitlen = binstr_snprintf(format_str2, binbuf, sizeof(binbuf), v);
  EXPECT_EQ(bitlen, 8);
  EXPECT_EQ(binbuf[0], '\xff');
}

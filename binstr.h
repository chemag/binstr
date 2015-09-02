#ifndef MCASTCAPTURE_UTILS_BINSTR_
#define MCASTCAPTURE_UTILS_BINSTR_

// binstr is a generic language to define binary strings (char *)
// using a combination of decimal, hexadecimal, octal, and binary
// components, explicit lengths, and repeated items. The goal is to
// be able to define a binary string using a user-friendly language.
//
// binstr also supports comments (any item starting with '#'),
// blank lines, and generic indentation.
//
// For example, an IPv4 header could be defined as follows:
//
//   const char kIpHeader[] = R"(
//     # version header_length service_type total_length
//     {4}0x4 {4}20 0x00 {16}1500
//     # identification evil dnf mf offset
//     {16}0xcafe 0b0 0b0 0b0 {13}0
//     # ttl protocol checksum
//     {8}255 {8}17 {16}0
//     # source addr
//     {32}0x12345678
//     # dst addr
//     {32}0x9abcdef0
//   )";
//
// And then read into an actual binary buffer using:
//
//   char binbuf[1024];
//   int bitlen = binstr_parse(kIpHeader, binbuf, sizeof(binbuf));
//
// Note that bitlen is the number of bits read, which may not be a multiple
// of 8.
//
// binstr_snprintf() adds a snprintf-like preprocessor.
//
// See go/binstr for more information.

// Parses a binstr string into a binary buffer.
// Returns the number of bits read (-1 if problems).
//
int binstr_parse(const char *in, char *buf, int buflen);

// Parses a binstr string with snprintf-like format characters.
// Returns the number of bits read (-1 if problems).
int binstr_snprintf(const char *in, char *buf, int buflen, ...)
    __attribute__((format(printf, 1, 4)));

#endif  // MCASTCAPTURE_UTILS_BINSTR_

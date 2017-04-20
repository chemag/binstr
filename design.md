# binstr: A Language for User-Friendly Definition of Binary Strings

Author: Chema Gonzalez, chema@, 20150421


# 1. Introduction

This document describes binstr, a language that allows user-friendly,
annotated definition of binary strings. By "user-friendly" we mean:

```
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
```

Instead of:

```
const char kIpHeaderBin[] =
   "\x45\x00\x05\xdc"
   "\xca\xfe\x00\x00"
   "\xff\x11\x00\x00"
   "\x12\x34\x56\x78"
   "\x9a\xbc\xde\xf0";
```

(Note that, in this case, you could define the ip header by using the
iphdr struct, but that requires having some formal structure of the
binary blob being represented, and being very careful about struct
packing options.)

In other words, we want to be able to specify binary strings using a
combination of decimal, octal, binary, and hexadecimal string, with
comments, explicit lengths, and others.

The main use so far case is defining binary strings (e.g. IP packets) in
unit tests. Note that binstr is agnostic respect to the binary string
being defined.


# 2. Discussion

The idea of binstr was born after spending an evening trying to make sense of
mpeg-ts packets defined using C strings composed of hexadecimal escape
sequences-only, and the realization that it was not the first time the author
had decoded that string.

binstr allows defining binary strings with the following features:

* inline comments: you can add an inline comment at any moment, explaining
  what a given byte sequence means.
* define binary sequences mixing hexadecimal, octal, binary, and decimal
  representations
* allow splitting binary sequences at any point (bits)
* sequences can have explicit lengths
* sequences can be repeated
* define binary sequences using ascii strings


# 3. Syntax

binstr allows defining a binary string using an annotated syntax in C string,
and then converting it into a plain C string.

## 3.1 Inline Comments

The first feature is adding inline comments, which can be used to annotate
the binstr.

```
const char *in = "# hello world";
char binbuf[1024];
int bitlen = binstr_parse[d][e][f](in, binbuf, sizeof(binbuf));
// returns zero, and does not modify binbuf
```

## 3.2 Binary, Octal, Hexadecimal Representations

binstr allows defining binary strings using different representation, and even
mixing the different representations. Note that we assume an implicit per-digit
length in both binary (1 bit per digit), octal (3 bits per digit), and
hexadecimal (4 bits per digit) representations. Also, note that any item larger
than 1 byte is written using big endianness.

```
char binbuf[1024];
const char *in = "0x33";  // hexadecimal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 8, and writes "\x33" at binbuf

const char *in = "0b10101010";  // binary
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 8, and writes "\xaa" at binbuf

const char *in = "077777777";  // octal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 24, and writes "\xff\xff\xff" at binbuf

const char *in = "0x4 0x4 0b11111 07";  // mix
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 16, and writes "\x44\xff" at binbuf
```

## 3.3 Decimal Representations and Explicit Lengths

All the items discussed so far (bin, oct, hex) have implicit lengths: Binary
items have an implicit length of 1 bit/digit, octal items have an implicit
length of 3 bits/digit, and hexadecimal items have an implicit length of 4
bits/digit.

binstr allows specifying the exact length of the item (explicit length, as
compared to implicit length). This is a must for decimal items, which unlike
hex/oct/bin have no implicit per-digit length, but also useful for any of the
other representations.

We have the issue of what happens when the implicit length is different from
the explicit ones.

Bin/oct/hex items have in theory infinite bits (you can just add the digit "0"
on the left as many times as you want). This makes it very easy to solve the
problem of an explicit larger than the implicit one: We just assume the item
has as many zeros as needed on the left side. It also suggest what to do when
the explicit length is not a multiple of 8: We just add extra bits if needed.

Now, in order to be coherent, when the explicit length is smaller than the
implicit one, we want to drop digits from the left side (MSB digits). Also, we
want to warn when some of the dropped digits are not zero (e.g., "{5}0x1f" is
equal to "{5}0x3f", but the second one drops non-zero MSB bits).

Note that there are no limitations for explicit lengths (except a temporary
issue with decimal representations, which are currently limited to 64 bits).

```
char binbuf[1024];

const char *in = "{13}0";  // decimal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 13, and writes "\x00\x00" at binbuf

const char *in = "{16}0x2ffff";  // hexadecimal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 16, and writes "\xff\xff" at binbuf

const char *in = "{17}0x2ffff";  // hexadecimal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 17, and writes "\x7f\xff\x80" at binbuf

const char *in = "{18}0x2ffff";  // hexadecimal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 18, and writes "\xbf\xff\xc0"[l][m] at binbuf

const char *in = "{22}0x2ffff";  // hexadecimal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 22, and writes "\x0b\xff\xfc" at binbuf
```


## 3.4 Repeated Sequences

binstr allows specifying parts of a binary string as a repeated sequence.

```
char binbuf[1024];

const char *in = "*8*0xff";  // hexadecimal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 64, and writes "\xff\xff\xff\xff\xff\xff\xff\xff" at binbuf

const char *in = "*8*{7}0xff";  // hexadecimal
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 56, and writes "\xff\xff\xff\xff\xff\xff\xff" at binbuf
```


## 3.5 Ascii Sequences

binstr allows specifying parts of a binary string as an ascii sequence.

```
char binbuf[1024];

const char *in = "\"AB\""[n][o];  // ascii
int bitlen = binstr_parse(in, binbuf, sizeof(binbuf));
// returns 16, and writes "\x41\x42" at binbuf
```


## 3.6 Putting Everything Together

We use some of the previous binstr mechanism to define an IP header in a
user-friendly way. Note that we are using C++ raw string literals for
convenience, but this is not actually needed.

```
const char kIpHeader[] = R"(
 # version header_length service_type total_length
 {4}0x4 {4}5 0x00 {16}1500  # 1500 = 0x5dc
 # identification evil dnf mf offset
 {16}0xcafe 0b0 0b0 0b0 {13}0
 # ttl protocol checksum
 {8}255 {8}17 {16}0
 # source addr
 {32}0x12345678
 # dst addr
 {32}0x9abcdef0
)";

char binbuf[1024];
int bitlen = binstr_parse(kIpHeader, binbuf, sizeof(binbuf));
// returns 20, and writes the following data at binbuf
//  "\x45\x00\x05\xdc"
//  "\xca\xfe\x00\x00"
//  "\xff\x11\x00\x00"
//  "\x12\x34\x56\x78"
//  "\x9a\xbc\xde\xf0";
```


# 4. Future Work

Some ideas to explore:

* adding support for endianness conversion.

* current parsing code is based on getline(), which is at its limit with
  the current (simple) features. Now, if we start adding mode features,
  we may switch to a flex/bison lexer/parser implementation.


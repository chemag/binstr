CPP=g++
CPPFLAGS=-g -O0 -pedantic -Wall -Werror -Wswitch-enum -Wextra \
    -Wno-unused-parameter
CPPFLAGS+=-std=c++0x
LDFLAGS=-lgtest_main -lgtest -lrt -lpthread

all: test

binstr.o: binstr.cc
	$(CPP) $(CPPFLAGS) $(LDFLAGS) -c -o binstr.o binstr.cc

binstr_test.o: binstr_test.cc
	$(CPP) $(CPPFLAGS) $(LDFLAGS) -c -o binstr_test.o binstr_test.cc

binstr_test: binstr_test.o binstr.o
	$(CPP) $(CPPFLAGS) $(LDFLAGS) binstr.o binstr_test.o -o binstr_test

test: binstr_test
	./binstr_test

clean:
	\rm -f *~ *.o binstr_test binstr_test.o binstr.o


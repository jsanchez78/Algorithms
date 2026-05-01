CXX = g++
CPPFLAGS = -std=c++20 -g -Wall
all: fenwick fenwick-one-based

fenwick: FenwickTree.cc
	   $(CXX) $(CPPFLAGS) $^ -o $@

fenwick-one-based: FenwickTreeOneBasedIndexing.cc
	   $(CXX) $(CPPFLAGS) $^ -o $@

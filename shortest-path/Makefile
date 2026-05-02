CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

test: test.cc dijkstras.cc
	$(CXX) $(CXXFLAGS) -o test test.cc

run: test
	./test

clean:
	rm -f test

.PHONY: run clean
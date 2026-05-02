CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

dijkstras: test.cc dijkstras.cc
	$(CXX) $(CXXFLAGS) -o dijkstras test.cc

dijkstras-dense: test-dense.cc dijkstras-dense-graphs.cc
	$(CXX) $(CXXFLAGS) -o dijkstras-dense test-dense.cc

test: dijkstras
	./dijkstras

test-dense: dijkstras-dense
	./dijkstras-dense

all: dijkstras dijkstras-dense

clean:
	rm -f dijkstras dijkstras-dense

.PHONY: test test-dense all clean
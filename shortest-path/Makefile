CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

dijkstras: test.cc dijkstras.cc
	$(CXX) $(CXXFLAGS) -o dijkstras test.cc

dijkstras-dense: test-dense.cc dijkstras-dense-graphs.cc
	$(CXX) $(CXXFLAGS) -o dijkstras-dense test-dense.cc

floyd-warshall: test-floyd-warshall.cc floyd-warshall.cc
	$(CXX) $(CXXFLAGS) -o floyd-warshall test-floyd-warshall.cc

astar: astar.cpp
	$(CXX) $(CXXFLAGS) -o astar astar.cpp

test: dijkstras
	./dijkstras

test-dense: dijkstras-dense
	./dijkstras-dense

test-floyd-warshall: floyd-warshall
	./floyd-warshall

test-astar: astar
	./astar

all: dijkstras dijkstras-dense floyd-warshall astar

floyd-warshall-wasm: floyd-warshall-wasm.cc
	em++ -std=c++17 -O2 -s EXPORTED_FUNCTIONS='["_init","_setEdge","_getDist","_getV","_getK","_getI","_getJ","_isDone","_step","_runAll"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -o floyd-warshall.js floyd-warshall-wasm.cc

dijkstras-wasm: dijkstras-wasm.cc
	em++ -std=c++17 -O2 -s EXPORTED_FUNCTIONS='["_init","_addEdge","_finalize","_step","_getDist","_getParent","_getCurNode","_isDone","_getV","_runAll"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -o dijkstras.js dijkstras-wasm.cc

clean:
	rm -f dijkstras dijkstras-dense floyd-warshall astar floyd-warshall.js floyd-warshall.wasm dijkstras.js dijkstras.wasm

.PHONY: test test-dense test-floyd-warshall test-astar all floyd-warshall-wasm dijkstras-wasm clean
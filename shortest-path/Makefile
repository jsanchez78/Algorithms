CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

dijkstras: test.cc dijkstras.cc
	$(CXX) $(CXXFLAGS) -o dijkstras test.cc

dijkstras-dense: test-dense.cc dijkstras-dense-graphs.cc
	$(CXX) $(CXXFLAGS) -o dijkstras-dense test-dense.cc

floyd-warshall: test-floyd-warshall.cc floyd-warshall.cc
	$(CXX) $(CXXFLAGS) -o floyd-warshall test-floyd-warshall.cc

test: dijkstras
	./dijkstras

test-dense: dijkstras-dense
	./dijkstras-dense

test-floyd-warshall: floyd-warshall
	./floyd-warshall

all: dijkstras dijkstras-dense floyd-warshall

floyd-warshall-wasm: floyd-warshall-wasm.cc
	em++ -std=c++17 -O2 -s EXPORTED_FUNCTIONS='["_init","_setEdge","_getDist","_getV","_getK","_getI","_getJ","_isDone","_step","_runAll"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -o floyd-warshall.js floyd-warshall-wasm.cc

clean:
	rm -f dijkstras dijkstras-dense floyd-warshall floyd-warshall.js floyd-warshall.wasm

.PHONY: test test-dense test-floyd-warshall all floyd-warshall-wasm clean
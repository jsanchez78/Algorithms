CXX      = g++
NVCC     = nvcc
CXXFLAGS = -std=c++26 -O3 -march=native -Wall -Wextra \
           -fno-omit-frame-pointer \   # needed for perf call graphs
           -falign-functions=64        # align hot functions to cache line
           # -freflection              # C++26 P2996 reflection — Clang trunk only
           #                           # enable when implementing gpu_reflect.hh
           #                           # requires: CXX = clang++ (not g++)

# CUDA flags: sm_89 = Ada Lovelace (RTX 4090); adjust for your GPU
NVCCFLAGS = -std=c++20 -O3 -arch=sm_89 --expt-relaxed-constexpr

LDFLAGS  = -lpthread

SRC      = src/main.cc
CUDA_SRC = cuda/kernels.cu
TARGET   = exchange

.PHONY: all clean perf

all: $(TARGET)

$(TARGET): $(SRC) $(CUDA_SRC)
	$(NVCC) $(NVCCFLAGS) -x cu $(CUDA_SRC) -c -o cuda/kernels.o
	$(CXX) $(CXXFLAGS) $(SRC) cuda/kernels.o -o $@ $(LDFLAGS) -lcuda -lcudart

# Build without CUDA (for machines without GPU)
no-cuda: $(SRC)
	$(CXX) $(CXXFLAGS) -DNO_CUDA $(SRC) -o $(TARGET) $(LDFLAGS)

# Record perf stat for one run (cache misses, branch mispredicts, IPC)
perf:
	perf stat -e cycles,instructions,cache-misses,branch-misses \
	          -e L1-dcache-load-misses,LLC-load-misses \
	          ./$(TARGET)

# Flame graph via perf record + stackcollapse (requires FlameGraph tools)
flamegraph:
	perf record -F 99 -g ./$(TARGET)
	perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg

clean:
	rm -f $(TARGET) cuda/kernels.o flame.svg perf.data

#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>
#include <atomic>
#include "types.hh"

// =============================================================================
// Performance Measurement
//
// Two complementary approaches:
//   1. RDTSC timestamps — nanosecond-resolution latency on the hot path.
//      No syscall, no context switch. Use __builtin_ia32_rdtsc() directly.
//   2. Linux perf_event_open — hardware counters (cache misses, branch
//      mispredictions, IPC) sampled from a dedicated monitoring thread.
//
// Latency histogram uses a fixed power-of-2 bucket array to avoid heap
// allocation and keep the recording path branch-free.
// =============================================================================

// -----------------------------------------------------------------------------
// RDTSC helpers — read CPU timestamp counter
// Serialize with LFENCE before read to prevent out-of-order execution
// from moving the timestamp outside the measured region.
// -----------------------------------------------------------------------------
[[nodiscard]] inline uint64_t rdtsc_start() {
    // LFENCE serializes prior loads; then RDTSC
    // TODO: __builtin_ia32_lfence(); return __builtin_ia32_rdtsc();
    return 0;
}

[[nodiscard]] inline uint64_t rdtsc_end() {
    // RDTSCP reads counter + processor ID atomically (no reorder across it)
    // TODO: uint32_t aux; return __builtin_ia32_rdtscp(&aux);
    return 0;
}

// Convert TSC ticks to nanoseconds using pre-measured TSC frequency
[[nodiscard]] inline uint64_t ticks_to_ns(uint64_t ticks, uint64_t tsc_hz) {
    // ticks * 1e9 / tsc_hz — use 128-bit intermediate to avoid overflow
    // TODO: implement with __uint128_t
    return 0;
}

// -----------------------------------------------------------------------------
// Latency Histogram
//
// Lock-free, single-writer histogram. Buckets are powers of 2 in nanoseconds:
//   bucket[i] covers [2^i ns, 2^(i+1) ns)
// Record path: one __builtin_clz + one atomic increment — branch-free.
// -----------------------------------------------------------------------------
static constexpr std::size_t HIST_BUCKETS = 64;

struct alignas(CACHE_LINE) Histogram {
    std::array<std::atomic<uint64_t>, HIST_BUCKETS> buckets{};
    std::string_view name;

    // Record a latency sample in nanoseconds.
    // bucket index = 63 - __builtin_clzll(ns | 1)  (branch-free log2)
    void record(uint64_t ns);

    // Print percentiles (p50, p99, p999) to stdout.
    void report() const;

    // Reset all buckets to zero.
    void reset();
};

// -----------------------------------------------------------------------------
// Perf Event Counter (Linux perf_event_open)
//
// Wraps a single hardware counter fd.
// Open at startup, read periodically from a monitoring thread.
// Do NOT read on the matching engine thread — ioctl has syscall overhead.
// -----------------------------------------------------------------------------
struct PerfCounter {
    int         fd{-1};
    std::string_view name;

    // Open a hardware counter. type/config from <linux/perf_event.h>
    // e.g. type=PERF_TYPE_HARDWARE, config=PERF_COUNT_HW_CACHE_MISSES
    bool open(uint32_t type, uint64_t config);

    // Read current counter value. Returns 0 on error.
    uint64_t read() const;

    void close();
};

// -----------------------------------------------------------------------------
// Exchange-wide metrics — one global instance
// Measure latency at each pipeline stage:
//   gateway_parse  : FIX receive → Order struct
//   sequence       : Order struct → SeqNum stamped
//   match          : SeqNum stamped → Execution emitted
//   end_to_end     : FIX receive → Execution emitted
// -----------------------------------------------------------------------------
struct ExchangeMetrics {
    Histogram gateway_parse;
    Histogram sequence;
    Histogram match;
    Histogram end_to_end;

    PerfCounter cache_misses;
    PerfCounter branch_mispredicts;
    PerfCounter instructions;
    PerfCounter cycles;

    // Initialize all perf counters. Call once at startup before engine starts.
    void init();

    // Print all histograms + perf counter deltas. Call from monitoring thread.
    void report() const;

    // Reset histograms between measurement windows.
    void reset();
};

// Global metrics instance — extern so all translation units share one copy
extern ExchangeMetrics g_metrics;

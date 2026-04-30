#pragma once
#include <atomic>
#include <array>
#include <optional>
#include <cstddef>
#include "types.hh"

// =============================================================================
// SPMC Lock-Free Ring Buffer
//
// Single Producer, Multiple Consumer.
// Producer writes with release semantics; consumers read with acquire.
// Capacity must be a power of 2 so modulo becomes a bitmask (no division).
//
// Memory order rationale:
//   - head_ (producer write index): relaxed load by producer (only writer),
//     release store after writing slot so consumers see the payload.
//   - tail_ (consumer read index): acquire load to see producer's payload,
//     release fetch_add to advance without stomping other consumers.
//   - No seq_cst anywhere — that would serialize all cores via MFENCE.
// =============================================================================

template <typename T, std::size_t Capacity>
    requires (Capacity > 0 && (Capacity & (Capacity - 1)) == 0) // power-of-2
class SPMCRingBuffer {
public:
    SPMCRingBuffer();

    // -------------------------------------------------------------------------
    // Producer side (single thread only)
    // Returns false if buffer is full — caller must handle backpressure.
    // Uses release store on head_ so consumers acquire the written slot.
    // -------------------------------------------------------------------------
    [[nodiscard("backpressure: dropped order if false")]] bool try_push(const T& item);

    // -------------------------------------------------------------------------
    // Consumer side (multiple threads)
    // CAS loop on tail_ to claim a slot before reading.
    // acquire load ensures we see the producer's release store.
    // Returns nullopt if buffer is empty.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::optional<T> try_pop();

    // Number of items currently in the buffer (approximate — no lock)
    std::size_t size() const;

private:
    static constexpr std::size_t MASK = Capacity - 1;

    // Separate cache lines for head and tail to eliminate false sharing
    // between producer and consumers.
    alignas(CACHE_LINE) std::atomic<std::size_t> head_{0}; // producer writes here
    alignas(CACHE_LINE) std::atomic<std::size_t> tail_{0}; // consumers read from here

    // Each slot is cache-line padded so adjacent slots don't share a line —
    // prevents consumers racing on the same cache line (false sharing on data).
    struct alignas(CACHE_LINE) Slot {
        T item;
    };
    std::array<Slot, Capacity> slots_;
};

// Convenience aliases for the two main message types
using OrderRing     = SPMCRingBuffer<Order,     1024>;
using ExecutionRing = SPMCRingBuffer<Execution, 1024>;

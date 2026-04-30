#pragma once
#include "spmc_ring.hh"
#include <atomic>

// =============================================================================
// Sequencer
//
// Assigns a monotonically increasing SeqNum to every inbound order before it
// enters the matching engine. This is the single point of total ordering.
//
// Design: single writer (sequencer thread) → OrderRing → matching engine.
// SeqNum is stamped with relaxed store (only one writer) then the ring push
// uses release so the engine sees the complete Order.
//
// In production (e.g. NYSE/NASDAQ style): the sequencer also persists the
// sequence log to a memory-mapped file for replay/recovery.
// =============================================================================

class Sequencer {
public:
    explicit Sequencer(OrderRing& outbound);

    // -------------------------------------------------------------------------
    // Stamp order with next seq_num and push to the outbound ring.
    // seq_num_ incremented with relaxed — single writer, no contention.
    // Ring push uses release so matching engine acquire-sees the full order.
    // Returns false if ring is full (backpressure to gateway).
    // -------------------------------------------------------------------------
    bool sequence(Order& order);

    // -------------------------------------------------------------------------
    // Current sequence number (for monitoring/recovery).
    // -------------------------------------------------------------------------
    SeqNum current_seq() const;

private:
    // Pad seq_num_ to its own cache line — written on every order,
    // don't want it sharing a line with anything the engine reads.
    alignas(CACHE_LINE) std::atomic<SeqNum> seq_num_{0};

    OrderRing& outbound_;
};

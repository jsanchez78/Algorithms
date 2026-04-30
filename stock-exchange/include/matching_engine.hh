#pragma once
#include "order_book.hh"
#include "spmc_ring.hh"
#include <unordered_map>

// =============================================================================
// Matching Engine
//
// Consumes sequenced orders from the inbound ring, matches against the order
// book, and publishes Execution reports to the outbound ring.
//
// Single-threaded by design — no locks needed. Sequencer guarantees total order.
// One MatchingEngine instance per symbol (or shard of symbols).
//
// Match algorithm: price-time priority
//   1. If incoming order crosses the spread → fill against resting orders
//   2. Partial fills reduce leaves_qty; full fills remove the resting order
//   3. Remaining qty (if any) rests in the book as a new limit order
// =============================================================================

class MatchingEngine {
public:
    MatchingEngine(OrderRing& inbound, ExecutionRing& outbound);

    // -------------------------------------------------------------------------
    // Process one order from the inbound ring.
    // Called in a tight loop by the engine thread — no blocking, no syscalls.
    // Returns false if inbound ring was empty (caller can yield/spin).
    // -------------------------------------------------------------------------
    [[nodiscard]] bool process_next();

    // -------------------------------------------------------------------------
    // Main engine loop — spins on inbound ring until stop() is called.
    // Pin this thread to an isolated core; disable hyperthreading on that core.
    // -------------------------------------------------------------------------
    void run();
    void stop();

private:
    // -------------------------------------------------------------------------
    // Route order to the correct handler based on OrdType.
    // Avoid virtual dispatch — use a jump table or if/else on the enum.
    // Branch predictor will learn the dominant path (limit >> market >> cancel).
    // [[likely]] on OrdType::Limit — overwhelmingly the common case in practice.
    // -------------------------------------------------------------------------
    void handle(const Order& order);

    // -------------------------------------------------------------------------
    // Try to match an incoming order against the opposite side of the book.
    // Iterates top-of-book levels while price crosses and qty remains.
    // Emits one Execution per fill via emit_fill().
    // -------------------------------------------------------------------------
    void match(Order& incoming, OrderBook& book);

    // -------------------------------------------------------------------------
    // Emit a fill execution report to the outbound ring.
    // Fills both the aggressor (incoming) and resting order sides.
    // -------------------------------------------------------------------------
    void emit_fill(const Order& aggressor, const Order& resting,
                   Price fill_price, Qty fill_qty);

    // -------------------------------------------------------------------------
    // Emit a reject (e.g. market order with empty book, invalid symbol).
    // [[unlikely]] — rejects are rare on a well-validated hot path.
    // -------------------------------------------------------------------------
    [[gnu::cold]] void emit_reject(const Order& order);

    OrderRing&     inbound_;
    ExecutionRing& outbound_;

    // One order book per symbol — looked up by symbol on each order
    std::unordered_map<Symbol, OrderBook,
        decltype([](const Symbol& s) {  // inline hasher for fixed array
            std::size_t h = 0;
            for (char c : s) h = h * 31 + static_cast<uint8_t>(c);
            return h;
        })> books_;

    bool running_{false};
};

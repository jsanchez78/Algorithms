#pragma once
#include <cstdint>
#include <cstddef>

// =============================================================================
// CUDA Kernels for HFT Exchange
//
// GPU is used for embarrassingly parallel, latency-tolerant workloads:
//   1. Risk/margin checks across a portfolio of positions (batch)
//   2. Market data fan-out: broadcast top-of-book snapshots to N subscribers
//   3. Order book statistics: VWAP, depth imbalance across many symbols
//
// NOT used for the matching engine hot path — GPU launch latency (~5-10µs)
// exceeds the matching engine's per-order budget (~100ns).
//
// Data transfer: pinned (page-locked) host memory + cudaMemcpyAsync to overlap
// compute with the next batch transfer.
// =============================================================================

// -----------------------------------------------------------------------------
// Device-side position record (fits in a register on GPU)
// -----------------------------------------------------------------------------
struct alignas(16) GpuPosition {
    int64_t  symbol_id;
    int64_t  qty;        // net position (positive = long)
    int64_t  price;      // average entry price (fixed-point)
    int64_t  margin;     // current margin requirement
};

// -----------------------------------------------------------------------------
// Kernel 1: Batch Risk Check
//
// For each pending order[i], check whether executing it would breach the
// position limit or margin limit for that client.
// One CUDA thread per order — fully independent, no inter-thread communication.
//
// Grid: (num_orders + BLOCK - 1) / BLOCK blocks, BLOCK threads each.
// Output: results[i] = 1 if order passes risk, 0 if rejected.
//
// __restrict__ tells the compiler arrays don't alias → enables vectorization.
// -----------------------------------------------------------------------------
__global__ void risk_check_kernel(
    const GpuPosition* __restrict__ positions, // current positions per client
    const int64_t*     __restrict__ order_qtys, // proposed qty change per order
    const int64_t*     __restrict__ order_prices,
    const int32_t*     __restrict__ client_ids,
    int32_t*           __restrict__ results,    // output: 1=pass, 0=fail
    int64_t            max_position,            // firm-wide position limit
    int64_t            max_margin,              // firm-wide margin limit
    int32_t            num_orders);

// -----------------------------------------------------------------------------
// Kernel 2: Market Data Fan-out
//
// Broadcast a top-of-book snapshot to N subscriber buffers simultaneously.
// Each thread writes to one subscriber's buffer — no contention.
//
// Grid: (num_subscribers + BLOCK - 1) / BLOCK blocks.
// -----------------------------------------------------------------------------
struct alignas(16) TopOfBook {
    int64_t bid_price;
    int64_t bid_qty;
    int64_t ask_price;
    int64_t ask_qty;
    int64_t seq_num;
};

__global__ void fanout_kernel(
    const TopOfBook*   __restrict__ snapshot,   // single source snapshot
    TopOfBook*         __restrict__ subscribers, // one slot per subscriber
    int32_t            num_subscribers);

// -----------------------------------------------------------------------------
// Kernel 3: Order Book Statistics (VWAP + depth imbalance)
//
// For each symbol, compute:
//   VWAP    = Σ(price_i * qty_i) / Σ(qty_i)  over top N levels
//   Imbalance = (bid_qty - ask_qty) / (bid_qty + ask_qty)
//
// Uses parallel reduction within each block (shared memory).
// One block per symbol; threads cover price levels within the symbol.
// -----------------------------------------------------------------------------
struct alignas(16) BookLevel {
    int64_t price;
    int64_t qty;
};

struct alignas(8) BookStats {
    int64_t vwap;       // fixed-point
    int32_t imbalance;  // scaled -1000..+1000
};

__global__ void book_stats_kernel(
    const BookLevel* __restrict__ bid_levels, // flattened: symbol * MAX_LEVELS + level
    const BookLevel* __restrict__ ask_levels,
    BookStats*       __restrict__ stats_out,  // one per symbol
    int32_t          num_symbols,
    int32_t          levels_per_symbol);

// -----------------------------------------------------------------------------
// Host-side launcher functions (called from C++ code)
// Manage pinned memory, async transfers, and stream synchronization.
// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// Allocate pinned host buffers and device buffers for risk checks.
// Call once at startup.
void cuda_risk_init(int32_t max_orders, int32_t max_clients);

// Submit a batch of orders for async risk check.
// Returns immediately; call cuda_risk_sync() to collect results.
void cuda_risk_submit(const int64_t* qtys, const int64_t* prices,
                      const int32_t* client_ids, int32_t* results,
                      int32_t num_orders);

// Block until the risk check batch completes.
void cuda_risk_sync();

void cuda_risk_destroy();

#ifdef __cplusplus
}
#endif

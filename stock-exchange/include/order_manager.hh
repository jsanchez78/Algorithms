#pragma once
#include "spmc_ring.hh"
#include <unordered_map>
#include <functional>

// =============================================================================
// Order Manager
//
// Tracks the lifecycle of every order from submission to terminal state.
// Sits between the gateway (inbound) and sequencer (outbound), and also
// consumes execution reports to update order state.
//
// Responsibilities:
//   - Validate inbound orders (price/qty sanity, duplicate order_id)
//   - Assign client-facing order_id
//   - Forward valid orders to sequencer
//   - Update order state on each execution report
//   - Notify client callbacks on fills/cancels/rejects
// =============================================================================

using ExecCallback = std::function<void(const Execution&)>;

class OrderManager {
public:
    OrderManager(OrderRing& to_sequencer, ExecutionRing& from_engine);

    // -------------------------------------------------------------------------
    // Submit a new order from the gateway.
    // Validates, assigns order_id, forwards to sequencer.
    // Returns assigned OrderId, or 0 on rejection.
    // -------------------------------------------------------------------------
    OrderId submit(Order& order);

    // -------------------------------------------------------------------------
    // Submit a cancel request.
    // Looks up original order, builds a Cancel order, forwards to sequencer.
    // -------------------------------------------------------------------------
    bool cancel(OrderId order_id);

    // -------------------------------------------------------------------------
    // Drain execution ring and update internal order state.
    // Call from a dedicated thread or inline with gateway event loop.
    // -------------------------------------------------------------------------
    void process_executions();

    // -------------------------------------------------------------------------
    // Register a callback invoked on every execution report.
    // Used by gateway to push fills back to the client session.
    // -------------------------------------------------------------------------
    void on_execution(ExecCallback cb);

private:
    // -------------------------------------------------------------------------
    // Validate order fields: price > 0, qty > 0, symbol non-empty, etc.
    // Returns false and sets status = Rejected if invalid.
    // [[likely]] on the valid branch — invalid orders are rare on hot path.
    // Use: if (validate(order)) [[likely]] { ... }
    // -------------------------------------------------------------------------
    [[nodiscard]] bool validate(const Order& order) const;

    // -------------------------------------------------------------------------
    // Generate next order_id — monotonic counter, relaxed increment (single writer).
    // -------------------------------------------------------------------------
    OrderId next_order_id();

    OrderRing&     to_sequencer_;
    ExecutionRing& from_engine_;
    ExecCallback   exec_cb_;

    std::unordered_map<OrderId, Order> live_orders_; // order_id → current state
    std::atomic<OrderId> next_id_{1};

    // -------------------------------------------------------------------------
    // TODO: Union-Find (DSU) for cross-margin risk groups
    //
    // WHAT IT MODELS:
    //   Instruments that share a margin pool are in the same "group".
    //   e.g. SPY and its constituent stocks, or an ETF and its options.
    //   When checking if a new order breaches margin, query the group's
    //   total exposure — not just the single instrument.
    //
    // WHY UNION-FIND:
    //   O(α(n)) ≈ O(1) per find/union after path compression + union by rank.
    //   Faster than a graph BFS/DFS to find connected components on each order.
    //
    // STRUCTURE:
    //   parent_[i] = parent of instrument i (self if root/representative)
    //   rank_[i]   = tree height upper bound (used to keep tree flat)
    //   margin_[i] = total margin used by the group (stored at root only)
    //
    // PSEUDOCODE:
    //
    //   find(x):                          // find root of x's group
    //     if parent_[x] != x:
    //       parent_[x] = find(parent_[x]) // path compression: flatten tree
    //     return parent_[x]
    //
    //   union(x, y):                      // merge two margin groups
    //     rx = find(x), ry = find(y)
    //     if rx == ry: return             // already same group
    //     if rank_[rx] < rank_[ry]: swap(rx, ry)
    //     parent_[ry] = rx               // attach smaller tree under larger
    //     margin_[rx] += margin_[ry]     // consolidate group margin
    //     if rank_[rx] == rank_[ry]: rank_[rx]++
    //
    //   group_margin(x):                  // total margin for x's group
    //     return margin_[find(x)]
    //
    //   check_margin(order):              // called in validate()
    //     group_root = find(instrument_id(order.symbol))
    //     projected  = group_margin(group_root) + margin_cost(order)
    //     return projected <= margin_limit_[client_id]
    //
    // INTEGRATION POINTS:
    //   - validate()           → call check_margin(order) before forwarding
    //   - process_executions() → update margin_[find(root)] on each fill
    //   - startup/config       → call union(x, y) for each correlated pair
    //
    // MEMBERS TO ADD:
    //   std::vector<int>   dsu_parent_;
    //   std::vector<int>   dsu_rank_;
    //   std::vector<Qty>   dsu_margin_;   // net margin at each group root
    //   std::unordered_map<Symbol, int> symbol_to_id_;  // symbol → DSU index
    //   std::unordered_map<ClientId, Qty> margin_limit_; // per-client limit
    // -------------------------------------------------------------------------
};

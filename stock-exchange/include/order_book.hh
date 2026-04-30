#pragma once
#include <map>
#include <unordered_map>
#include <vector>
#include "types.hh"

// =============================================================================
// Order Book
//
// Maintains resting limit orders for one symbol, sorted by price-time priority.
//   Bids: descending price (highest bid first)
//   Asks: ascending  price (lowest  ask first)
//
// Hot path: add/cancel/match — must stay branch-minimal.
// std::map gives O(log n) price level lookup; inner queue is FIFO (time priority).
// For production: replace map with a flat sorted array or skip list.
// =============================================================================

// =============================================================================
// TODO: Fenwick Tree (Binary Indexed Tree) for cumulative depth queries
//
// WHAT IT REPLACES:
//   Currently best_bid/best_ask scan the map top. Fenwick lets us answer
//   "total qty available from price 0 to price P" in O(log U) where U is
//   the price universe size — no map traversal needed.
//
// WHY IT'S FASTER:
//   std::map price lookup = O(log n) pointer-chasing (cache unfriendly).
//   Fenwick = O(log U) array index arithmetic — stays in L1 cache.
//
// STRUCTURE:
//   tree_[i] stores the sum of qty for a range of price buckets.
//   The range covered by index i = i & (-i)  (lowest set bit trick).
//
// PSEUDOCODE:
//
//   // Map real price to bucket index: bucket = price / TICK_SIZE
//   // TICK_SIZE = minimum price increment (e.g. 1 cent = 100 in fixed-point)
//
//   update(bucket, delta):           // add/remove qty at a price level
//     i = bucket
//     while i <= MAX_PRICE_BUCKETS:
//       tree_[i] += delta
//       i += i & (-i)               // move to parent in BIT
//
//   prefix_sum(bucket):             // total qty from bucket 1..bucket
//     sum = 0
//     i = bucket
//     while i > 0:
//       sum += tree_[i]
//       i -= i & (-i)               // strip lowest set bit → move to predecessor
//     return sum
//
//   qty_between(lo, hi):            // qty available in price range [lo, hi]
//     return prefix_sum(hi) - prefix_sum(lo - 1)
//
// INTEGRATION POINTS (marked TODO below):
//   - add()    → call update(price_to_bucket(order.price), +order.qty)
//   - cancel() → call update(price_to_bucket(order.price), -order.leaves_qty)
//   - match()  → call update(..., -fill_qty) after each fill
//   - VWAP     → iterate buckets with qty_between() instead of scanning levels
//
// SEPARATE TREES FOR EACH SIDE:
//   bid_tree_[MAX_BUCKETS]  — bids index by price ascending, query from top
//   ask_tree_[MAX_BUCKETS]  — asks index by price ascending, query from bottom
//
// MEMORY:
//   MAX_PRICE_BUCKETS = 1_000_000 (prices up to $100,000.00 at $0.01 tick)
//   Two arrays × 1M × 8 bytes = 16 MB — fits in L3 cache on modern CPUs.
// =============================================================================

// All orders at a single price level, in arrival order (FIFO)
struct PriceLevel {
    Price              price;
    std::vector<Order> orders; // front = oldest (highest priority)
    Qty total_qty() const;     // sum of leaves_qty across all orders
};

class OrderBook {
public:
    explicit OrderBook(Symbol symbol);

    // -------------------------------------------------------------------------
    // Add a new resting limit order to the appropriate side.
    // O(log n) map insertion + O(1) vector push_back.
    // -------------------------------------------------------------------------
    void add(const Order& order);

    // -------------------------------------------------------------------------
    // Cancel a resting order by id.
    // Looks up order_id → price level, removes from vector.
    // O(log n) map + O(n) scan of level — acceptable for cancel rate.
    // -------------------------------------------------------------------------
    bool cancel(OrderId order_id);

    // -------------------------------------------------------------------------
    // Best bid/ask prices. Returns 0 if side is empty.
    // Called on every incoming order — keep branch-free.
    // -------------------------------------------------------------------------
    Price best_bid() const;
    Price best_ask() const;

    // -------------------------------------------------------------------------
    // Top-of-book level for matching engine consumption.
    // Returns nullptr if side is empty.
    // -------------------------------------------------------------------------
    PriceLevel* top_bid();
    PriceLevel* top_ask();

    // Remove a fully-filled or cancelled level if empty
    void prune_empty_levels();

    const Symbol& symbol() const { return symbol_; }

private:
    Symbol symbol_;

    // Bids: highest price = best, so reverse (descending) map
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    // Asks: lowest price = best, ascending map
    std::map<Price, PriceLevel>                      asks_;

    // Fast cancel lookup: order_id → price (to find level in O(log n))
    std::unordered_map<OrderId, Price> bid_index_;
    std::unordered_map<OrderId, Price> ask_index_;

    // -------------------------------------------------------------------------
    // TODO: Fenwick Tree members
    //
    // Replace bids_/asks_ map lookups with these for O(log U) depth queries.
    //
    // static constexpr int MAX_PRICE_BUCKETS = 1'000'000;
    // static constexpr Price TICK_SIZE = 100; // $0.01 in fixed-point (price*10000)
    //
    // std::array<Qty, MAX_PRICE_BUCKETS + 1> bid_tree_{};
    // std::array<Qty, MAX_PRICE_BUCKETS + 1> ask_tree_{};
    //
    // int price_to_bucket(Price p) const { return static_cast<int>(p / TICK_SIZE); }
    //
    // void   fenwick_update(std::array<Qty, ...>& tree, int bucket, Qty delta);
    // Qty    fenwick_prefix(const std::array<Qty, ...>& tree, int bucket) const;
    // Qty    fenwick_range(const std::array<Qty, ...>& tree, int lo, int hi) const;
    // -------------------------------------------------------------------------
};

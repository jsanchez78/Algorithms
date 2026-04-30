#pragma once
#include <cstdint>
#include <array>

// Cache line size for alignment — prevents false sharing between threads
static constexpr std::size_t CACHE_LINE = 64;

// Fixed-length symbol to avoid heap allocation on hot path
using Symbol = std::array<char, 8>;

enum class Side    : uint8_t { Buy, Sell };
enum class OrdType : uint8_t { Limit, Market, Cancel };
enum class OrdStatus : uint8_t { New, PartialFill, Filled, Cancelled, Rejected };

// All prices are integer fixed-point (price * 10000) to avoid FP on hot path
using Price    = int64_t;
using Qty      = int64_t;
using OrderId  = uint64_t;
using SeqNum   = uint64_t;
using ClientId = uint32_t;
using Nanos    = uint64_t; // nanoseconds since epoch

// Inbound order from gateway/broker — pad to cache line to avoid false sharing
struct alignas(CACHE_LINE) Order {
    OrderId  order_id;
    ClientId client_id;
    Symbol   symbol;
    Price    price;
    Qty      qty;
    Qty      leaves_qty;   // remaining unfilled quantity
    Side     side;
    OrdType  type;
    OrdStatus status;
    Nanos    timestamp;
    SeqNum   seq_num;      // assigned by sequencer
    uint8_t  _pad[3];      // explicit padding to silence warnings
};

// Outbound execution report — one per fill
struct alignas(CACHE_LINE) Execution {
    OrderId  order_id;
    ClientId client_id;
    Symbol   symbol;
    Price    fill_price;
    Qty      fill_qty;
    Side     side;
    OrdStatus status;
    Nanos    timestamp;
    SeqNum   seq_num;
};

static_assert(sizeof(Order)     <= CACHE_LINE * 2);
static_assert(sizeof(Execution) <= CACHE_LINE * 2);

#pragma once
#include "order_manager.hh"
#include <cstdint>
#include <span>

// =============================================================================
// Gateway
//
// External-facing entry point. Accepts FIX protocol messages from brokers,
// parses them into Order structs, and hands off to OrderManager.
// Also receives Execution reports from OrderManager and encodes FIX responses.
//
// FIX parsing is the only place where we tolerate branch-heavy code —
// it runs before the hot path (sequencer → engine).
//
// In production: one Gateway instance per TCP session (broker connection).
// Use SO_BUSY_POLL + kernel bypass (DPDK/RDMA) to eliminate syscall latency.
// =============================================================================

// Raw FIX message buffer — caller owns the memory
struct FIXMessage {
    const uint8_t* data;
    std::size_t    len;
};

class Gateway {
public:
    explicit Gateway(OrderManager& om);

    // -------------------------------------------------------------------------
    // Parse a raw FIX buffer into an Order and submit to OrderManager.
    // FIX tag parsing: scan for tag=value\x01 delimiters.
    // Critical tags: 11 (ClOrdID), 54 (Side), 38 (Qty), 44 (Price), 55 (Symbol)
    // Returns assigned OrderId or 0 on parse/validation failure.
    // -------------------------------------------------------------------------
    OrderId on_fix_message(const FIXMessage& msg);

    // -------------------------------------------------------------------------
    // Encode an Execution report as a FIX ExecutionReport (MsgType=8).
    // Writes into caller-supplied buffer to avoid heap allocation.
    // Returns number of bytes written.
    // -------------------------------------------------------------------------
    std::size_t encode_execution(const Execution& exec,
                                 std::span<uint8_t> out_buf);

    // -------------------------------------------------------------------------
    // Encode an order acknowledgement (MsgType=8, OrdStatus=New).
    // -------------------------------------------------------------------------
    std::size_t encode_ack(const Order& order, std::span<uint8_t> out_buf);

    // -------------------------------------------------------------------------
    // Encode a reject (MsgType=8, OrdStatus=Rejected) with reason text.
    // -------------------------------------------------------------------------
    std::size_t encode_reject(const Order& order, std::string_view reason,
                              std::span<uint8_t> out_buf);

private:
    // -------------------------------------------------------------------------
    // Parse FIX tag-value pairs from raw buffer into an Order.
    // No heap allocation — writes directly into the Order struct.
    // -------------------------------------------------------------------------
    bool parse_fix(const FIXMessage& msg, Order& out) const;

    // -------------------------------------------------------------------------
    // Write FIX checksum tag (10=NNN\x01) at end of buffer.
    // Checksum = sum of all bytes mod 256, formatted as 3 digits.
    // -------------------------------------------------------------------------
    std::size_t append_checksum(std::span<uint8_t> buf, std::size_t len) const;

    OrderManager& om_;
};

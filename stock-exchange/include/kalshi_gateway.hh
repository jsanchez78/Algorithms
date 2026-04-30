#pragma once
#include "types.hh"
#include "signal.hh"
#include "probability_model.hh"
#include <string>
#include <string_view>
#include <functional>
#include <span>

// =============================================================================
// Kalshi Gateway — REST + WebSocket API Client
//
// Replaces the FIX gateway for Kalshi prediction markets.
//
// Kalshi API overview:
//   REST  (https://trading-api.kalshi.com/trade-api/v2)
//     POST /portfolio/orders          — place order
//     DELETE /portfolio/orders/{id}   — cancel order
//     GET  /portfolio/positions       — current positions
//     GET  /markets/{ticker}          — market info (yes_bid, yes_ask, volume)
//
//   WebSocket (wss://trading-api.kalshi.com/trade-api/ws/v2)
//     Subscribe to orderbook_delta, trade, ticker channels
//     Receive real-time top-of-book updates → feed into ProbabilityModel
//
// Auth: Bearer token in Authorization header (API key from Kalshi dashboard)
//
// Rate limits (as of 2024):
//   REST:  10 req/sec per endpoint
//   WS:    unlimited subscriptions, ~100ms update frequency
//
// LATENCY PROFILE (realistic expectations):
//   REST round-trip to Kalshi: ~50-200ms (internet, not colo)
//   WS feed latency: ~50-100ms
//   → Strategy must work on seconds/minutes timescale, not microseconds
//   → Your edge comes from the probability MODEL, not speed
// =============================================================================

// Kalshi market snapshot from REST or WS feed
struct KalshiMarket {
    std::string ticker;       // e.g. "KXBTCD-23DEC31-T30000"
    double      yes_bid;      // best bid for YES contract (0..1)
    double      yes_ask;      // best ask for YES contract (0..1)
    double      volume;       // 24h volume in contracts
    double      open_interest;
    int64_t     expiry_ts;    // unix timestamp of resolution
    std::string title;        // human-readable event description
};

// Order placement request to Kalshi REST API
struct KalshiOrderRequest {
    std::string ticker;
    std::string side;         // "yes" or "no"
    std::string type;         // "limit" or "market"
    int         count;        // number of contracts (integer, min 1)
    double      yes_price;    // limit price for YES (0.01..0.99, 2 decimal places)
};

// Order response from Kalshi
struct KalshiOrderResponse {
    std::string order_id;
    std::string status;       // "resting", "filled", "canceled"
    int         filled_count;
    double      avg_price;
};

// Callbacks for async events
using MarketUpdateCb = std::function<void(const KalshiMarket&)>;
using OrderFillCb    = std::function<void(const KalshiOrderResponse&)>;

class KalshiGateway {
public:
    KalshiGateway(std::string_view api_key, std::string_view api_secret);

    // -------------------------------------------------------------------------
    // REST: Place a limit order on Kalshi.
    // Serializes KalshiOrderRequest to JSON, POST to /portfolio/orders.
    // Returns order_id on success, empty string on failure.
    //
    // JSON body example:
    //   {"ticker":"KXBTCD-...", "side":"yes", "type":"limit",
    //    "count":10, "yes_price":0.65}
    //
    // TODO: implement with libcurl or cpp-httplib
    //   - Set Authorization: Bearer {api_key_} header
    //   - Parse JSON response for order_id and status
    // -------------------------------------------------------------------------
    std::string place_order(const KalshiOrderRequest& req);

    // -------------------------------------------------------------------------
    // REST: Cancel a resting order.
    // DELETE /portfolio/orders/{order_id}
    // TODO: implement
    // -------------------------------------------------------------------------
    bool cancel_order(const std::string& order_id);

    // -------------------------------------------------------------------------
    // REST: Fetch current positions (for risk/margin tracking).
    // GET /portfolio/positions
    // TODO: parse JSON array of {ticker, position, avg_price}
    //       feed into OrderManager's DSU margin groups
    // -------------------------------------------------------------------------
    void sync_positions();

    // -------------------------------------------------------------------------
    // REST: Fetch market snapshot for a ticker.
    // GET /markets/{ticker}
    // Returns current yes_bid, yes_ask, volume, expiry.
    // TODO: implement, feed result into ProbabilityModel::MarketFeatures
    // -------------------------------------------------------------------------
    KalshiMarket get_market(const std::string& ticker);

    // -------------------------------------------------------------------------
    // WebSocket: Subscribe to real-time orderbook + trade feed.
    // Connects to wss://trading-api.kalshi.com/trade-api/ws/v2
    //
    // Subscribe message:
    //   {"id":1, "cmd":"subscribe",
    //    "params":{"channels":["orderbook_delta","ticker"],
    //              "market_tickers":["KXBTCD-..."]}}
    //
    // On each update: parse delta, update local KalshiMarket snapshot,
    // invoke market_update_cb_ so SignalGenerator can re-evaluate.
    //
    // TODO: implement with libwebsockets or Boost.Beast
    //   - Run WS recv loop on dedicated thread
    //   - Parse JSON delta: {type, market_ticker, yes_bid, yes_ask}
    //   - Call market_update_cb_(updated_market)
    // -------------------------------------------------------------------------
    void subscribe(const std::string& ticker, MarketUpdateCb cb);
    void unsubscribe(const std::string& ticker);

    // -------------------------------------------------------------------------
    // Register callback for order fill notifications (via WS fills channel).
    // -------------------------------------------------------------------------
    void on_fill(OrderFillCb cb);

    // -------------------------------------------------------------------------
    // Main event loop: run WS recv + signal evaluation in one thread.
    //
    // Loop:
    //   1. Recv WS message → update KalshiMarket snapshot
    //   2. Build MarketFeatures from snapshot + external data
    //   3. signal_gen_.evaluate(features, bid, ask, model_)
    //   4. If signal: place_order() via REST
    //   5. Track open orders, cancel stale resting orders
    //
    // TODO: implement after WS and REST are working individually
    // -------------------------------------------------------------------------
    void run(ProbabilityModel& model, SignalGenerator& signal_gen);
    void stop();

private:
    // -------------------------------------------------------------------------
    // Build MarketFeatures from a KalshiMarket snapshot.
    // Fills in market_mid, spread, volume, time_to_expiry.
    // External features (poll_avg, sentiment) must be injected separately.
    // TODO: implement
    // -------------------------------------------------------------------------
    MarketFeatures build_features(const KalshiMarket& market) const;

    // -------------------------------------------------------------------------
    // Serialize KalshiOrderRequest to JSON string.
    // Avoid nlohmann/json on hot path — hand-roll the small fixed schema.
    // TODO: implement with snprintf into a stack buffer
    // -------------------------------------------------------------------------
    std::string serialize_order(const KalshiOrderRequest& req) const;

    // -------------------------------------------------------------------------
    // Parse JSON response body into KalshiOrderResponse.
    // TODO: implement — only need order_id, status, filled_count, avg_price
    // -------------------------------------------------------------------------
    KalshiOrderResponse parse_order_response(std::string_view json) const;

    std::string    api_key_;
    std::string    api_secret_;
    MarketUpdateCb market_update_cb_;
    OrderFillCb    fill_cb_;
    bool           running_{false};
};

#pragma once
#include "types.hh"
#include "probability_model.hh"
#include <cmath>
#include <optional>

// =============================================================================
// Signal Generator — Mispricing Detection + Position Sizing
//
// Given your model's fair value p and the market's implied probability q,
// decide: do we bet YES, bet NO, or do nothing?
//
// MATHEMATICAL FOUNDATIONS:
//
// 1. KELLY CRITERION (information theory / expected log utility)
//    Maximizes long-run geometric growth rate of bankroll.
//    For a binary bet at market price q, your edge is p - q.
//
//    Full Kelly fraction:
//      f* = (bp - q) / b
//    where b = net odds = (1 - q) / q  (how much you win per $1 risked)
//
//    Simplified for prediction markets (payout = $1):
//      f* = p - q          if betting YES  (buy at price q)
//      f* = (1-p) - (1-q)  if betting NO   (sell at price q, i.e. buy NO at 1-q)
//
//    CRITICAL: Use fractional Kelly (e.g. 0.25x) in practice.
//    Full Kelly has high variance and assumes your model is perfectly calibrated.
//    Half-Kelly halves variance for only a ~25% reduction in growth rate.
//
//    Kelly derivation (for your reference):
//      E[log(W)] = p*log(1 + f*b) + (1-p)*log(1 - f)
//      dE/df = 0 → f* = (pb - (1-p)) / b = p - (1-p)/b
//
// 2. EDGE THRESHOLD (hypothesis testing)
//    Don't trade unless |p - q| > threshold.
//    Threshold accounts for:
//      - Bid-ask spread cost: you pay spread/2 to enter and exit
//      - Model uncertainty: σ_model from Kalman covariance P
//      - Transaction fees: Kalshi charges ~2% of winnings
//
//    Minimum edge to break even:
//      edge_min = spread/2 + fee_rate + z_α * σ_model
//    where z_α = confidence multiplier (e.g. 1.96 for 95% confidence)
//
// 3. SHARPE RATIO (portfolio theory)
//    Evaluate signal quality: SR = E[edge] / σ[edge]
//    Track rolling Sharpe across resolved contracts.
//    SR > 1.0 is good, > 2.0 is excellent for a prediction market strategy.
//
// 4. MARTINGALE / OPTIONAL STOPPING THEOREM (probability theory)
//    Do NOT double down on losing positions (Martingale).
//    Optional stopping theorem: E[Xτ] = E[X₀] for a fair game regardless
//    of stopping rule — you cannot manufacture edge by bet sizing alone.
//    Edge must come from the probability model, not the betting strategy.
//
// 5. CORRELATION ADJUSTMENT (multivariate stats)
//    If you hold positions in correlated markets (e.g. "Biden wins Iowa" and
//    "Biden wins election"), your effective exposure is higher than face value.
//    Adjust Kelly fraction: f_adjusted = f* / (1 + ρ * n_correlated)
//    where ρ = correlation between markets, n = number of correlated positions.
// =============================================================================

enum class SignalDirection { Buy, Sell, None };

struct Signal {
    SignalDirection direction;
    double          fair_value;      // model's probability estimate
    double          market_mid;      // (best_bid + best_ask) / 2
    double          edge;            // |fair_value - market_mid|
    double          kelly_fraction;  // recommended bet size as fraction of bankroll
    double          confidence;      // model uncertainty (from Kalman P or Beta variance)
};

class SignalGenerator {
public:
    // kelly_fraction_cap: max fraction of bankroll per trade (e.g. 0.05 = 5%)
    // edge_threshold: minimum edge to generate a signal (e.g. 0.03 = 3 cents)
    // fee_rate: Kalshi fee as fraction of winnings (e.g. 0.02 = 2%)
    explicit SignalGenerator(double kelly_fraction_cap = 0.25,
                             double edge_threshold     = 0.03,
                             double fee_rate           = 0.02);

    // -------------------------------------------------------------------------
    // Evaluate a market and return a signal if edge exceeds threshold.
    // Returns nullopt if no trade warranted.
    // [[nodiscard]] — silently ignoring a signal is always a bug.
    // -------------------------------------------------------------------------
    [[nodiscard("signal must be acted on or explicitly discarded")]]
    std::optional<Signal> evaluate(const MarketFeatures& features,
                                   double best_bid,
                                   double best_ask,
                                   ProbabilityModel& model) const;

    // -------------------------------------------------------------------------
    // Kelly fraction for a YES bet (buy at ask price q).
    // f* = (p - q) / (1 - q)   — fraction of bankroll to risk
    // TODO: implement, clamp to [0, kelly_fraction_cap]
    // -------------------------------------------------------------------------
    [[nodiscard]] double kelly_yes(double p, double q) const;

    // -------------------------------------------------------------------------
    // Kelly fraction for a NO bet (buy NO at price 1-bid).
    // Equivalent to: p_no = 1-p, q_no = 1-bid
    // f* = (p_no - q_no) / (1 - q_no)
    // TODO: implement, clamp to [0, kelly_fraction_cap]
    // -------------------------------------------------------------------------
    [[nodiscard]] double kelly_no(double p, double bid) const;

    // -------------------------------------------------------------------------
    // Minimum edge required to trade, accounting for:
    //   - Half-spread cost: (ask - bid) / 2
    //   - Kalshi fee: fee_rate * expected_payout
    //   - Model uncertainty: z * confidence_std  (z=1.96 for 95%)
    // TODO: implement
    // -------------------------------------------------------------------------
    [[nodiscard]] double min_edge(double spread, double confidence_std) const;

    // -------------------------------------------------------------------------
    // Correlation-adjusted Kelly for a portfolio of N correlated positions.
    // f_adjusted = f* / (1 + avg_correlation * (n_positions - 1))
    // TODO: implement using DSU group from OrderManager
    // -------------------------------------------------------------------------
    [[nodiscard]] double correlation_adjusted_kelly(double raw_kelly,
                                                    double avg_correlation,
                                                    int    n_positions) const;

    // -------------------------------------------------------------------------
    // Update rolling Sharpe ratio after a contract resolves.
    // Call with realized_pnl after each resolution.
    // SR = mean_pnl / std_pnl  (rolling window of last N trades)
    // -------------------------------------------------------------------------
    void   record_outcome(double realized_pnl);
    [[nodiscard]] double rolling_sharpe() const;

private:
    double kelly_fraction_cap_;
    double edge_threshold_;
    double fee_rate_;

    // Rolling PnL window for Sharpe calculation (last 100 trades)
    static constexpr int WINDOW = 100;
    std::array<double, WINDOW> pnl_history_{};
    int    pnl_idx_{0};
    int    pnl_count_{0};
    double pnl_sum_{0.0};
    double pnl_sum_sq_{0.0};
};

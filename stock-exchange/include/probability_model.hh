#pragma once
#include <array>
#include <span>
#include <cstdint>
#include <cmath>
#include "types.hh"

// =============================================================================
// Probability Model — Fair Value Estimator for Kalshi Binary Contracts
//
// A Kalshi contract pays $1 if event E occurs, $0 otherwise.
// Your edge = |your_prob - market_implied_prob|.
// Market implied prob = (best_ask + best_bid) / 2  (mid-price).
//
// MATHEMATICAL FOUNDATIONS YOU CAN LEVERAGE:
//
// 1. LOGISTIC REGRESSION (multivariable stats)
//    P(E | x) = σ(wᵀx + b) = 1 / (1 + e^(-(wᵀx + b)))
//    x = feature vector (polls, sentiment, volume, time-to-expiry)
//    w = learned weights (offline gradient descent, online SGD updates)
//    σ = sigmoid — maps any real to (0,1), perfect for probabilities.
//    Gradient: ∂L/∂w = (σ(wᵀx) - y) * x  (cross-entropy loss)
//
// 2. KALMAN FILTER (diffeq + linear algebra)
//    Model market probability as a hidden state evolving over time:
//      xₖ = Axₖ₋₁ + Bwₖ          (state transition: prob drifts)
//      zₖ = Hxₖ + vₖ              (observation: noisy market price)
//    Kalman gives optimal (MMSE) estimate of true prob given noisy observations.
//    Use when market price is your primary signal but you know it's noisy.
//    Predict step:  x̂ₖ|ₖ₋₁ = Ax̂ₖ₋₁,  Pₖ|ₖ₋₁ = APₖ₋₁Aᵀ + Q
//    Update step:   Kₖ = Pₖ|ₖ₋₁Hᵀ(HPₖ|ₖ₋₁Hᵀ + R)⁻¹
//                   x̂ₖ = x̂ₖ|ₖ₋₁ + Kₖ(zₖ - Hx̂ₖ|ₖ₋₁)
//
// 3. BAYESIAN UPDATING (conditional probability)
//    P(E | new_data) = P(new_data | E) * P(E) / P(new_data)
//    Prior P(E) = base rate (historical resolution rate for this event type)
//    Likelihood P(new_data | E) = how likely is this poll/news given E is true
//    Posterior becomes new prior as data arrives — sequential updating.
//    Conjugate prior for Bernoulli: Beta(α, β) distribution.
//      α = prior wins + observed wins
//      β = prior losses + observed losses
//      Mean = α / (α + β)  ← your probability estimate
//
// 4. PRINCIPAL COMPONENT ANALYSIS (linear algebra)
//    If you have N correlated features (polls, markets, sentiment scores),
//    PCA finds the k < N orthogonal directions of maximum variance.
//    Reduces overfitting, removes multicollinearity.
//    Covariance matrix C = (1/n) XᵀX, eigendecompose: C = VΛVᵀ
//    Project features: x_reduced = Vₖᵀ x  (top-k eigenvectors)
//
// 5. ORNSTEIN-UHLENBECK PROCESS (diffeq — mean reversion)
//    Market prices on Kalshi tend to mean-revert to fundamentals.
//    dXₜ = θ(μ - Xₜ)dt + σdWₜ
//    θ = mean reversion speed, μ = long-run mean (your model's fair value)
//    σ = volatility, Wₜ = Wiener process
//    When |Xₜ - μ| is large → strong signal to fade the market.
//    Closed-form solution: Xₜ = μ + (X₀ - μ)e^(-θt) + σ∫e^(-θ(t-s))dWₛ
//
// 6. CROSS-ENTROPY CALIBRATION (information theory)
//    A well-calibrated model: among all predictions of p=0.7, ~70% resolve YES.
//    Measure calibration with Expected Calibration Error (ECE):
//      ECE = Σ |acc(Bₘ) - conf(Bₘ)| * |Bₘ| / n  over bins Bₘ
//    Platt scaling: recalibrate raw scores via logistic fit on held-out data.
//    This is critical — a model with good discrimination but poor calibration
//    will lose money even if it ranks events correctly.
// =============================================================================

// Feature vector for one Kalshi market at a point in time
struct alignas(CACHE_LINE) MarketFeatures {
    double poll_avg;          // weighted average of relevant polls (0..1)
    double poll_std;          // standard deviation across polls
    double market_mid;        // (best_bid + best_ask) / 2 from Kalshi
    double market_spread;     // best_ask - best_bid (liquidity proxy)
    double volume_24h;        // contract volume last 24h (normalized)
    double time_to_expiry_h;  // hours until resolution
    double sentiment_score;   // NLP sentiment from news/social (-1..+1)
    double correlated_market; // implied prob from a related Kalshi market
};

static constexpr int N_FEATURES = sizeof(MarketFeatures) / sizeof(double);

// Beta distribution parameters for Bayesian prior
struct BetaPrior {
    double alpha; // pseudo-count of YES resolutions
    double beta;  // pseudo-count of NO resolutions

    // Mean of Beta(α,β) = α/(α+β) — use as probability estimate
    double mean() const { return alpha / (alpha + beta); }

    // Variance = αβ / ((α+β)²(α+β+1)) — use as uncertainty measure
    double variance() const {
        double s = alpha + beta;
        return (alpha * beta) / (s * s * (s + 1.0));
    }
};

// Kalman filter state for tracking one market's true probability
struct KalmanState {
    double x;   // estimated true probability (state)
    double P;   // estimation error covariance
    double Q;   // process noise (how fast does true prob drift?)
    double R;   // observation noise (how noisy is market price?)

    // TODO: implement predict/update steps
    // predict(): x = x (no drift model), P = P + Q
    // update(z): K = P / (P + R)
    //            x = x + K * (z - x)
    //            P = (1 - K) * P
};

class ProbabilityModel {
public:
    ProbabilityModel();

    // -------------------------------------------------------------------------
    // Primary estimate: combine all sub-models into one probability.
    // Weighted ensemble: p = w₁*p_logistic + w₂*p_bayesian + w₃*p_kalman
    // Weights learned offline via held-out calibration set.
    // -------------------------------------------------------------------------
    double estimate(const MarketFeatures& features);

    // -------------------------------------------------------------------------
    // Logistic regression forward pass.
    // p = σ(wᵀx + b)  where x = feature vector, w = weight vector
    // TODO: implement sigmoid + dot product
    // -------------------------------------------------------------------------
    double logistic(const MarketFeatures& features) const;

    // -------------------------------------------------------------------------
    // Bayesian update: incorporate new poll or resolution data.
    // posterior α += observed_yes, β += observed_no
    // TODO: implement Beta conjugate update
    // -------------------------------------------------------------------------
    void bayesian_update(double observed_yes, double observed_no);
    double bayesian_estimate() const;

    // -------------------------------------------------------------------------
    // Kalman filter update: new market price observation arrives.
    // Fuses market price (noisy) with model estimate (biased but smooth).
    // TODO: implement 1D Kalman predict + update
    // -------------------------------------------------------------------------
    void kalman_observe(double market_price);
    double kalman_estimate() const;

    // -------------------------------------------------------------------------
    // OU mean-reversion signal: how far is market from model fair value?
    // z_score = (market_mid - fair_value) / σ_ou
    // Large |z_score| → strong fade signal (market overreacted)
    // TODO: estimate θ, μ, σ from historical price series via MLE
    //   θ = -log(corr(Xₜ, Xₜ₋₁)) / Δt
    //   μ = mean(X),  σ² = var(X) * 2θ / (1 - e^(-2θΔt))
    // -------------------------------------------------------------------------
    double ou_zscore(double market_mid) const;

    // -------------------------------------------------------------------------
    // Calibration: apply Platt scaling to raw logistic output.
    // p_calibrated = σ(a * p_raw + b)
    // a, b fit on held-out data via MLE.
    // TODO: implement after collecting prediction history
    // -------------------------------------------------------------------------
    double calibrate(double raw_prob) const;

    // -------------------------------------------------------------------------
    // Online weight update (SGD step on new resolved contract).
    // ∂L/∂w = (p̂ - y) * x  (cross-entropy gradient)
    // w ← w - η * ∂L/∂w
    // Call after each contract resolves with actual outcome y ∈ {0,1}.
    // -------------------------------------------------------------------------
    void update_weights(const MarketFeatures& features, double y, double learning_rate);

private:
    std::array<double, N_FEATURES> weights_{};  // logistic regression weights
    double bias_{0.0};

    BetaPrior   prior_{1.0, 1.0};  // uninformative Beta(1,1) = uniform
    KalmanState kalman_{0.5, 1.0, 0.01, 0.05};

    // OU parameters (fit offline from historical data)
    double ou_theta_{1.0};  // mean reversion speed
    double ou_mu_{0.5};     // long-run mean
    double ou_sigma_{0.1};  // volatility

    // Platt scaling parameters
    double platt_a_{1.0};
    double platt_b_{0.0};

    // Ensemble weights (must sum to 1)
    double w_logistic_{0.5};
    double w_bayesian_{0.3};
    double w_kalman_{0.2};
};

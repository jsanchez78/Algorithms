#pragma once
#include "probability_model.hh"
#include "types.hh"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

// =============================================================================
// FDA Features — Domain-Specific Model for Drug Approval Prediction
//
// STRATEGY:
//   Polymarket lists binary contracts: "Will FDA approve [drug] by [PDUFA date]?"
//   Our edge: quantifiable base rates + Bayesian updating on public data.
//   Most retail bettors anchor on headlines; we anchor on historical rates.
//
// BASE RATES (from FDA historical data 2010-2024):
//   Overall NDA/BLA approval rate:     ~85-90%
//   Priority Review:                   ~90%
//   Standard Review:                   ~80%
//   With Adcom Yes vote:               ~89%
//   With Adcom No vote:                ~17%
//   After Complete Response Letter:    ~55% (resubmission)
//   Oncology (accelerated pathway):    ~93%
//   CNS/Psychiatry:                    ~75%
//   Rare disease / Orphan Drug:        ~88%
//
// These base rates become your Beta prior α/(α+β).
// =============================================================================

enum class TherapeuticArea : uint8_t {
    Oncology,
    CNS,
    Cardiovascular,
    RareDisease,
    Immunology,
    Infectious,
    Metabolic,
    Other
};

enum class ReviewType : uint8_t {
    Priority,
    Standard,
    Accelerated,
    Breakthrough
};

enum class AdcomOutcome : uint8_t {
    NotHeld,       // no advisory committee meeting — use base rate only
    VotedYes,      // majority voted favorable
    VotedNo,       // majority voted unfavorable
    Pending        // meeting scheduled but not yet held
};

// Phase 3 trial result summary
struct TrialResult {
    bool   primary_endpoint_met;  // did the trial hit its primary endpoint?
    double p_value;               // statistical significance (lower = stronger)
    double effect_size;           // standardized effect size (Cohen's d or HR)
    int    sample_size;
    bool   safety_signal;         // serious adverse events flagged?
};

// All features for one FDA approval market
struct alignas(CACHE_LINE) FDAFeatures {
    // --- Drug / regulatory info ---
    std::string     drug_name;
    std::string     nct_id;          // ClinicalTrials.gov NCT number
    TherapeuticArea area;
    ReviewType      review_type;
    AdcomOutcome    adcom;
    int             prior_crl_count; // number of prior Complete Response Letters

    // --- Trial data ---
    TrialResult     pivotal_trial;

    // --- Market data (from Polymarket feed) ---
    double          market_yes_bid;
    double          market_yes_ask;
    double          market_volume;
    double          hours_to_pdufa;  // hours until PDUFA action date

    // --- Derived features for model input ---
    double          base_rate;       // looked up from area + review_type
    double          adcom_adjustment; // multiplicative factor from adcom vote
};

// =============================================================================
// Base Rate Lookup Table
//
// Returns Beta(α, β) prior for a given therapeutic area + review type.
// Derived from FDA historical approval data.
//
// Example: Oncology + Priority → Beta(18, 2) → mean = 0.90
//          CNS + Standard      → Beta(15, 5) → mean = 0.75
//
// TODO: populate from FDA Purple Book / Drugs@FDA historical CSV
//   for each (area, review_type) pair:
//     count approvals → α
//     count rejections → β
// =============================================================================
BetaPrior base_rate_prior(TherapeuticArea area, ReviewType review);

// =============================================================================
// Adcom Vote → Likelihood Ratio
//
// Converts advisory committee outcome into a Bayesian likelihood multiplier.
//
// P(approval | adcom yes) ≈ 0.89 → likelihood ratio = 0.89 / base_rate
// P(approval | adcom no)  ≈ 0.17 → likelihood ratio = 0.17 / base_rate
//
// Applied as: posterior_α = prior_α * LR,  posterior_β = prior_β * (1 - LR)
// (simplified; exact conjugate update is more nuanced)
//
// TODO: implement lookup table from historical adcom→approval data
// =============================================================================
double adcom_likelihood_ratio(AdcomOutcome outcome, double base_rate);

// =============================================================================
// ClinicalTrials.gov API Client
//
// REST endpoint: https://clinicaltrials.gov/api/v2/studies/{nct_id}
// Returns JSON with: status, phase, primary outcome, results
//
// Rate limit: ~3 req/sec (generous for our use case)
// =============================================================================
class ClinicalTrialsClient {
public:
    // -------------------------------------------------------------------------
    // Fetch trial result for a given NCT ID.
    // GET https://clinicaltrials.gov/api/v2/studies/{nct_id}
    // Parse JSON: protocolSection.resultsSection.outcomeMeasuresModule
    // Returns nullopt if trial has no posted results yet.
    //
    // TODO: implement with libcurl
    //   - Parse primaryOutcome.measure, statisticalAnalysis.pValue
    //   - Extract adverse events from adverseEventsModule
    // -------------------------------------------------------------------------
    std::optional<TrialResult> fetch_trial(const std::string& nct_id);

    // -------------------------------------------------------------------------
    // Search for Phase 3 trials by drug name / sponsor.
    // GET https://clinicaltrials.gov/api/v2/studies?query.term={drug}&filter.phase=PHASE3
    // Returns list of NCT IDs to evaluate.
    //
    // TODO: implement
    // -------------------------------------------------------------------------
    std::vector<std::string> search_trials(const std::string& drug_name);
};

// =============================================================================
// FDA Feature Builder
//
// Orchestrates: lookup base rate → fetch trial → check adcom → build FDAFeatures
// Then converts FDAFeatures → MarketFeatures for the ProbabilityModel.
// =============================================================================
class FDAFeatureBuilder {
public:
    explicit FDAFeatureBuilder(ClinicalTrialsClient& client);

    // -------------------------------------------------------------------------
    // Build complete feature set for one FDA market.
    // Steps:
    //   1. Look up base rate prior from (area, review_type)
    //   2. Fetch pivotal trial result from ClinicalTrials.gov
    //   3. Apply adcom adjustment if meeting was held
    //   4. Compute hours_to_pdufa from current time
    //   5. Fill market data from Polymarket feed
    //
    // TODO: implement
    // -------------------------------------------------------------------------
    FDAFeatures build(const std::string& drug_name,
                      TherapeuticArea area,
                      ReviewType review,
                      AdcomOutcome adcom,
                      int prior_crl_count,
                      double market_bid,
                      double market_ask,
                      double market_volume,
                      int64_t pdufa_timestamp_utc);

    // -------------------------------------------------------------------------
    // Convert FDAFeatures → MarketFeatures for ProbabilityModel::estimate().
    // Maps domain-specific fields to the generic feature vector:
    //   poll_avg         ← base_rate (our "poll" is the historical rate)
    //   poll_std         ← uncertainty from Beta variance
    //   market_mid       ← (yes_bid + yes_ask) / 2
    //   market_spread    ← yes_ask - yes_bid
    //   volume_24h       ← market_volume (normalized)
    //   time_to_expiry_h ← hours_to_pdufa
    //   sentiment_score  ← adcom_adjustment (mapped to -1..+1)
    //   correlated_market← 0.0 (no correlated market for FDA)
    //
    // TODO: implement
    // -------------------------------------------------------------------------
    MarketFeatures to_market_features(const FDAFeatures& fda) const;

    // -------------------------------------------------------------------------
    // Full pipeline: build features → estimate prob → generate signal.
    // Convenience method that chains FDAFeatureBuilder → ProbabilityModel.
    //
    // TODO: implement after ProbabilityModel::estimate() is working
    // -------------------------------------------------------------------------
    double estimate_approval_prob(const FDAFeatures& fda,
                                  ProbabilityModel& model);

private:
    ClinicalTrialsClient& client_;

    // Cache of fetched trial results to avoid redundant API calls
    std::unordered_map<std::string, TrialResult> trial_cache_;
};

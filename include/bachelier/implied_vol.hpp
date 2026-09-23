#pragma once

#include <optional>

#include "bachelier/option.hpp"

namespace bachelier::implied_vol {

struct Result {
    double volatility{};
    int iterations{};
    double residual{};  // model price - target price, at the returned vol
};

struct Settings {
    double tolerance{1e-10};      // on the price residual
    double vol_tolerance{1e-10};  // on the volatility step
    int max_iterations{200};
    double lower_bound{1e-9};
    double upper_bound{10.0};  // 1000% vol; wide enough for anything liquid
};

/// Solve for the volatility that reproduces `target_price`.
///
/// Uses Newton-Raphson seeded by the Brenner-Subrahmanyam approximation, and
/// falls back to bisection whenever a Newton step leaves the bracket or vega
/// collapses (which it does for deep in- or out-of-the-money options).
///
/// `spec.volatility` is ignored on input. Returns nullopt when the target price
/// violates the no-arbitrage bounds and therefore has no solution.
std::optional<Result> solve(const OptionSpec& spec, double target_price, const Settings& settings = {});

/// No-arbitrage price bounds for the contract: any target outside [lo, hi]
/// is unattainable at any volatility.
struct Bounds {
    double lower;
    double upper;
};
Bounds price_bounds(const OptionSpec& spec);

}  // namespace bachelier::implied_vol

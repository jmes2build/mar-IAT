#include "bachelier/implied_vol.hpp"

#include <algorithm>
#include <cmath>

#include "bachelier/black_scholes.hpp"

namespace bachelier::implied_vol {
namespace {

double price_at(OptionSpec spec, double vol) {
    spec.volatility = vol;
    return black_scholes::price(spec);
}

/// Brenner-Subrahmanyam (1988) ATM approximation: σ ≈ √(2π/T)·(price/spot).
/// Cheap and close enough near the money to save several Newton steps.
double initial_guess(const OptionSpec& spec, double target_price) {
    static constexpr double two_pi = 6.2831853071795864769252867665590;
    if (spec.time_to_expiry <= 0.0) return 0.5;
    const double guess = std::sqrt(two_pi / spec.time_to_expiry) * (target_price / spec.spot);
    if (!std::isfinite(guess) || guess <= 0.0) return 0.5;
    return std::clamp(guess, 0.01, 3.0);
}

}  // namespace

Bounds price_bounds(const OptionSpec& spec) {
    spec.validate();
    const double df_r = std::exp(-spec.rate * spec.time_to_expiry);
    const double df_q = std::exp(-spec.dividend_yield * spec.time_to_expiry);
    const double fwd = spec.spot * df_q;
    const double disc_k = spec.strike * df_r;

    if (spec.type == OptionType::Call) {
        // Intrinsic (forward) value from below, discounted spot from above.
        return {std::max(fwd - disc_k, 0.0), fwd};
    }
    return {std::max(disc_k - fwd, 0.0), disc_k};
}

std::optional<Result> solve(const OptionSpec& spec, double target_price, const Settings& settings) {
    spec.validate();
    if (!std::isfinite(target_price)) return std::nullopt;

    const Bounds bounds = price_bounds(spec);
    // A price at or outside the no-arbitrage bounds has no finite implied vol.
    if (target_price <= bounds.lower || target_price >= bounds.upper) {
        // Exactly on the lower bound is attainable in the limit σ → 0.
        if (std::abs(target_price - bounds.lower) < settings.tolerance) {
            return Result{settings.lower_bound, 0, price_at(spec, settings.lower_bound) - target_price};
        }
        return std::nullopt;
    }

    double lo = settings.lower_bound;
    double hi = settings.upper_bound;
    double vol = initial_guess(spec, target_price);

    for (int i = 1; i <= settings.max_iterations; ++i) {
        const double diff = price_at(spec, vol) - target_price;

        // Maintain the bracket so we can always fall back to bisection.
        if (diff > 0.0) {
            hi = vol;  // price too high -> vol too high
        } else {
            lo = vol;
        }

        // Once the bracket is pinned, the answer is the bracket - regardless
        // of what the price residual says. This is the only criterion that
        // works far from the money, where vega underflows and a wide band of
        // volatilities all reprice within tolerance.
        if (hi - lo <= settings.vol_tolerance) {
            const double mid = 0.5 * (lo + hi);
            return Result{mid, i, price_at(spec, mid) - target_price};
        }

        OptionSpec at_vol = spec;
        at_vol.volatility = vol;
        const double vega = black_scholes::greeks(at_vol).vega;

        // Newton needs a usable derivative. When vega underflows - deep in or
        // out of the money, or very short dated - the step is meaningless (and
        // would be exactly zero, stalling the iteration), so bisect instead.
        double next;
        if (vega > 1e-12) {
            next = vol - diff / vega;
            // Reject steps that leave the bracket. The comparison is
            // deliberately non-strict: landing exactly on a bound means diff
            // was exactly zero, i.e. we are standing on the root, and treating
            // that as an escape would bisect away from the answer.
            if (!std::isfinite(next) || next < lo || next > hi) {
                next = 0.5 * (lo + hi);
            }
        } else {
            next = 0.5 * (lo + hi);
        }

        // Converging on price alone is not enough, so require the step in
        // volatility to have settled as well.
        const double step = next - vol;
        if (std::abs(diff) < settings.tolerance && std::abs(step) < settings.vol_tolerance) {
            return Result{vol, i, diff};
        }
        vol = next;
    }

    const double final_diff = price_at(spec, vol) - target_price;
    return Result{vol, settings.max_iterations, final_diff};
}

}  // namespace bachelier::implied_vol

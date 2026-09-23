#pragma once

#include <cmath>

namespace mariat {

/// Standard normal probability density function.
inline double norm_pdf(double x) {
    static constexpr double inv_sqrt_2pi = 0.3989422804014326779399460599343819;
    return inv_sqrt_2pi * std::exp(-0.5 * x * x);
}

/// Standard normal cumulative distribution function.
///
/// Uses erfc rather than an Abramowitz-Stegun style polynomial: it is accurate
/// to full double precision and avoids catastrophic cancellation in the tails,
/// which matters when solving for implied volatility on deep OTM options.
inline double norm_cdf(double x) {
    static constexpr double inv_sqrt2 = 0.7071067811865475244008443621048490;
    return 0.5 * std::erfc(-x * inv_sqrt2);
}

}  // namespace mariat

#pragma once

#include "bachelier/option.hpp"

namespace bachelier::black_scholes {

/// Black-Scholes-Merton price of a European option with continuous dividends.
double price(const OptionSpec& spec);

/// Analytic sensitivities under the same model.
Greeks greeks(const OptionSpec& spec);

/// Forward price of the underlying at expiry: S·e^((r-q)T).
double forward(const OptionSpec& spec);

/// Left-hand side of put-call parity, C - P, computed from the model.
/// Should equal S·e^(-qT) - K·e^(-rT) to within rounding.
double parity_residual(const OptionSpec& spec);

}  // namespace bachelier::black_scholes

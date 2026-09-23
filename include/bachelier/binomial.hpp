#pragma once

#include "bachelier/option.hpp"

namespace bachelier::binomial {

/// Cox-Ross-Rubinstein lattice price.
///
/// For `Exercise::European` this converges to the Black-Scholes price as
/// `steps` grows; for `Exercise::American` it captures the early-exercise
/// premium, which has no closed form.
///
/// Memory is O(steps); the lattice is collapsed in place during the backward
/// induction rather than materialising the full triangle.
double price(const OptionSpec& spec, Exercise exercise, int steps = 512);

/// Greeks by finite difference on the lattice.
///
/// Delta and gamma are read directly off the first two levels of the tree,
/// which is both cheaper and steadier than re-pricing at bumped spots.
/// Vega, theta and rho are central differences over re-priced trees.
Greeks greeks(const OptionSpec& spec, Exercise exercise, int steps = 512);

/// Early-exercise premium: American price minus European price.
/// Zero for American calls on non-dividend-paying underlyings.
double early_exercise_premium(const OptionSpec& spec, int steps = 512);

}  // namespace bachelier::binomial

// Self-contained test runner: no framework, no dependencies.
//
// The interesting checks here are the ones that hold by construction rather
// than by table lookup - put-call parity, binomial convergence to the closed
// form, and Greeks agreeing with finite differences of the price. Those catch
// real errors; hard-coded expected values mostly catch typos.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "mariat/binomial.hpp"
#include "mariat/black_scholes.hpp"
#include "mariat/implied_vol.hpp"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("  FAIL  %s\n", what.c_str());
    }
}

void near(double got, double want, double tol, const std::string& what) {
    ++g_checks;
    const double diff = std::abs(got - want);
    if (!(diff <= tol)) {
        ++g_failures;
        std::printf("  FAIL  %s\n        got %.12f want %.12f (|diff| %.3e > tol %.3e)\n", what.c_str(), got, want,
                    diff, tol);
    }
}

mariat::OptionSpec base_spec() {
    mariat::OptionSpec s;
    s.spot = 100.0;
    s.strike = 100.0;
    s.time_to_expiry = 1.0;
    s.rate = 0.05;
    s.volatility = 0.20;
    s.dividend_yield = 0.0;
    s.type = mariat::OptionType::Call;
    return s;
}

void test_known_values() {
    std::printf("known Black-Scholes values\n");
    auto s = base_spec();
    // Textbook figures for S=K=100, r=5%, sigma=20%, T=1, q=0.
    near(mariat::black_scholes::price(s), 10.450583572185565, 1e-10, "ATM call price");
    s.type = mariat::OptionType::Put;
    near(mariat::black_scholes::price(s), 5.573526022256971, 1e-10, "ATM put price");
}

void test_put_call_parity() {
    std::printf("put-call parity\n");
    // C - P must equal S*e^(-qT) - K*e^(-rT) for every parameter set.
    const std::vector<double> spots{50, 90, 100, 110, 250};
    const std::vector<double> vols{0.05, 0.2, 0.8, 2.0};
    const std::vector<double> times{0.01, 0.5, 1.0, 5.0};
    const std::vector<double> qs{0.0, 0.03, 0.12};

    for (double spot : spots)
        for (double vol : vols)
            for (double t : times)
                for (double q : qs) {
                    auto s = base_spec();
                    s.spot = spot;
                    s.volatility = vol;
                    s.time_to_expiry = t;
                    s.dividend_yield = q;
                    const double lhs = mariat::black_scholes::parity_residual(s);
                    const double rhs = spot * std::exp(-q * t) - s.strike * std::exp(-s.rate * t);
                    near(lhs, rhs, 1e-9, "parity S=" + std::to_string(spot) + " vol=" + std::to_string(vol));
                }
}

void test_greeks_vs_finite_difference() {
    std::printf("analytic Greeks vs finite differences\n");
    for (auto type : {mariat::OptionType::Call, mariat::OptionType::Put}) {
        auto s = base_spec();
        s.type = type;
        s.spot = 105.0;
        s.dividend_yield = 0.02;
        const auto g = mariat::black_scholes::greeks(s);
        const std::string tag = " (" + mariat::to_string(type) + ")";

        const auto bumped = [&](auto mutate, double h) {
            auto up = s, down = s;
            mutate(up, h);
            mutate(down, -h);
            return (mariat::black_scholes::price(up) - mariat::black_scholes::price(down)) / (2.0 * h);
        };

        near(g.delta, bumped([](mariat::OptionSpec& o, double h) { o.spot += h; }, 1e-4), 1e-6, "delta" + tag);
        near(g.vega, bumped([](mariat::OptionSpec& o, double h) { o.volatility += h; }, 1e-5), 1e-4, "vega" + tag);
        near(g.rho, bumped([](mariat::OptionSpec& o, double h) { o.rate += h; }, 1e-6), 1e-3, "rho" + tag);

        // Gamma is the second derivative of price in spot.
        auto up = s, down = s;
        up.spot += 1e-3;
        down.spot -= 1e-3;
        const double fd_gamma = (mariat::black_scholes::price(up) - 2.0 * mariat::black_scholes::price(s) +
                                 mariat::black_scholes::price(down)) /
                                (1e-3 * 1e-3);
        near(g.gamma, fd_gamma, 1e-4, "gamma" + tag);

        // Theta is dV/dt with time running forward, i.e. -dV/dT.
        auto shorter = s;
        shorter.time_to_expiry -= 1e-6;
        const double fd_theta = (mariat::black_scholes::price(shorter) - mariat::black_scholes::price(s)) / 1e-6;
        near(g.theta, fd_theta, 1e-2, "theta" + tag);
    }
}

void test_gamma_vega_symmetry() {
    std::printf("gamma and vega identical across call/put\n");
    auto c = base_spec();
    c.spot = 93.0;
    c.dividend_yield = 0.04;
    auto p = c;
    p.type = mariat::OptionType::Put;
    const auto gc = mariat::black_scholes::greeks(c);
    const auto gp = mariat::black_scholes::greeks(p);
    near(gc.gamma, gp.gamma, 1e-12, "gamma matches");
    near(gc.vega, gp.vega, 1e-12, "vega matches");
}

void test_binomial_converges() {
    std::printf("binomial converges to Black-Scholes (European)\n");
    for (auto type : {mariat::OptionType::Call, mariat::OptionType::Put}) {
        auto s = base_spec();
        s.type = type;
        s.spot = 96.0;
        s.dividend_yield = 0.01;
        const double closed = mariat::black_scholes::price(s);

        double prev_err = 1e9;
        for (int steps : {50, 200, 800, 3200}) {
            const double lattice = mariat::binomial::price(s, mariat::Exercise::European, steps);
            const double err = std::abs(lattice - closed);
            check(err < prev_err, "error shrinks at " + std::to_string(steps) + " steps (" + mariat::to_string(type) +
                                      ")");
            prev_err = err;
        }
        near(mariat::binomial::price(s, mariat::Exercise::European, 4000), closed, 5e-3,
             "4000-step lattice ~ closed form (" + mariat::to_string(type) + ")");
    }
}

void test_american_premium() {
    std::printf("American early-exercise premium\n");
    auto put = base_spec();
    put.type = mariat::OptionType::Put;
    put.spot = 90.0;  // in the money, so early exercise has value
    const double amer = mariat::binomial::price(put, mariat::Exercise::American, 1000);
    const double euro = mariat::binomial::price(put, mariat::Exercise::European, 1000);
    check(amer >= euro - 1e-12, "American put >= European put");
    check(amer - euro > 1e-4, "American put carries a positive early-exercise premium");

    // An American call on a non-dividend payer should never be exercised early,
    // so its premium collapses to zero. This is Merton's classic result.
    auto call = base_spec();
    call.dividend_yield = 0.0;
    near(mariat::binomial::early_exercise_premium(call, 1000), 0.0, 1e-6, "no premium on non-dividend American call");
}

void test_implied_vol_roundtrip() {
    std::printf("implied volatility round-trip\n");
    for (auto type : {mariat::OptionType::Call, mariat::OptionType::Put})
        for (double vol : {0.05, 0.15, 0.40, 1.20})
            for (double spot : {70.0, 100.0, 130.0}) {
                auto s = base_spec();
                s.type = type;
                s.volatility = vol;
                s.spot = spot;
                const double target = mariat::black_scholes::price(s);

                const auto got = mariat::implied_vol::solve(s, target);
                check(got.has_value(), "solver returned a result");
                if (got) {
                    near(got->volatility, vol, 1e-6,
                         "recovered vol " + std::to_string(vol) + " at S=" + std::to_string(spot));
                    check(got->iterations < 60, "converged in under 60 iterations");
                }
            }
}

void test_implied_vol_rejects_arbitrage() {
    std::printf("implied volatility rejects unattainable prices\n");
    auto s = base_spec();
    const auto bounds = mariat::implied_vol::price_bounds(s);
    check(!mariat::implied_vol::solve(s, bounds.upper * 1.5).has_value(), "price above upper bound has no solution");
    check(!mariat::implied_vol::solve(s, -1.0).has_value(), "negative price has no solution");
}

void test_edge_cases() {
    std::printf("degenerate inputs\n");
    auto s = base_spec();
    s.time_to_expiry = 0.0;
    s.spot = 120.0;
    near(mariat::black_scholes::price(s), 20.0, 1e-12, "expired ITM call is intrinsic");
    near(mariat::black_scholes::greeks(s).delta, 1.0, 1e-12, "expired ITM call delta is 1");

    s.spot = 80.0;
    near(mariat::black_scholes::price(s), 0.0, 1e-12, "expired OTM call is worthless");

    auto zero_vol = base_spec();
    zero_vol.volatility = 0.0;
    const double disc_intrinsic = std::max(100.0 - 100.0 * std::exp(-0.05), 0.0);
    near(mariat::black_scholes::price(zero_vol), disc_intrinsic, 1e-12, "zero vol call is discounted intrinsic");

    bool threw = false;
    try {
        auto bad = base_spec();
        bad.spot = -1.0;
        mariat::black_scholes::price(bad);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "negative spot is rejected");
}

}  // namespace

int main() {
    std::printf("mariat test suite\n\n");
    test_known_values();
    test_put_call_parity();
    test_greeks_vs_finite_difference();
    test_gamma_vega_symmetry();
    test_binomial_converges();
    test_american_premium();
    test_implied_vol_roundtrip();
    test_implied_vol_rejects_arbitrage();
    test_edge_cases();

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}

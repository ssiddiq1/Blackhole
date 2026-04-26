// Geodesic.cpp — CPU geodesic integrators for Schwarzschild spacetime.
//
// All equations are in the equatorial plane (θ = π/2).
// Metric:  ds² = −f dt² + f⁻¹ dr² + r² dφ²    f = 1 − 2M/r
//
// ── Christoffel symbols (equatorial, non-zero) ────────────────────────────
//   Γ^t_tr = M / (r² f)               [MTW eq. 31.13a]
//   Γ^r_tt = M f / r²                 [MTW eq. 31.13b]
//   Γ^r_rr = −M / (r² f)              [MTW eq. 31.13c]
//   Γ^r_φφ = −r f   (at θ=π/2)        [MTW eq. 31.13d]
//   Γ^φ_φr = 1/r                      [MTW eq. 31.13e]
//
// Geodesic ODE:  d²x^μ/dτ² + Γ^μ_νσ ẋ^ν ẋ^σ = 0
//
// Expanding in the equatorial plane (ẋ^θ = 0):
//   ẗ   = −(2M/r²f) ṫ ṙ
//   r̈   = −(Mf/r²) ṫ² + (M/r²f) ṙ² + rf φ̇²
//   φ̈   = −(2/r) φ̇ ṙ

#include "physics/Geodesic.hpp"
#include "physics/Schwarzschild.hpp"
#include <cmath>
#include <stdexcept>

namespace bh {

// ─── Timelike geodesic ────────────────────────────────────────────────────

// Right-hand side of the geodesic ODE.
// State y = (r, phi, rdot, phidot, tdot); returns dy/dτ.
static void rhs(
    double r, double /*phi*/, double rdot, double phidot, double tdot,
    double M,
    double& dr, double& dphi, double& drdot, double& dphidot, double& dtdot)
{
    const double f    = lapse(r, M);         // 1 − 2M/r
    const double finv = 1.0 / f;
    const double Mor2 = M / (r * r);         // M/r²

    // dφ/dτ = φ̇  (trivial)
    dr     = rdot;
    dphi   = phidot;

    // d²t/dτ² = −(2M/r²f) ṫ ṙ
    dtdot  = -2.0 * Mor2 * finv * tdot * rdot;

    // d²φ/dτ² = −(2/r) φ̇ ṙ
    dphidot = -2.0 / r * phidot * rdot;

    // d²r/dτ² = −(Mf/r²) ṫ² + (M/r²f) ṙ² + rf φ̇²
    drdot  = -Mor2 * f   * tdot   * tdot
             +Mor2 * finv * rdot   * rdot
             +r    * f   * phidot * phidot;
}

TimelikeState circular_orbit_ic(double r0, double M)
{
    // For a circular orbit at r0, ṙ = 0, r̈ = 0.
    // From r̈ = 0 with ṙ = 0:
    //   −(Mf/r²) ṫ² + rf φ̇² = 0
    //   →  (φ̇/ṫ)² = M/r³     (Keplerian angular velocity)
    //
    // From metric normalization  −f ṫ² + r² φ̇² = −1:
    //   ṫ² = r / (r − 3M)
    //
    // Conserved quantities:
    //   E²  = f² / (1 − 3M/r) = (r−2M)² / [r(r−3M)]    [MTW eq. 33.9]
    //   L²  = M r² / (r − 3M)                            [MTW eq. 33.8]

    if (r0 <= 3.0 * M)
        throw std::invalid_argument("circular_orbit_ic: r0 must be > 3M");

    const double f0 = lapse(r0, M);
    const double L  = std::sqrt(L_circular_sq(r0, M));
    const double E  = std::sqrt(E_circular_sq(r0, M));

    TimelikeState s{};
    s.tau    = 0.0;
    s.r      = r0;
    s.phi    = 0.0;
    s.rdot   = 0.0;
    s.phidot = L / (r0 * r0);   // φ̇ = L / r²
    s.tdot   = E / f0;           // ṫ = E / f  (from E = f ṫ)
    return s;
}

double metric_norm(const TimelikeState& s, double M) noexcept
{
    const double f = lapse(s.r, M);
    return -f * s.tdot * s.tdot
           + (1.0/f) * s.rdot * s.rdot
           + s.r * s.r * s.phidot * s.phidot;
}

TimelikeState rk4_step(const TimelikeState& s, double M, double dtau) noexcept
{
    // Classic RK4:  y_{n+1} = y_n + (h/6)(k1 + 2k2 + 2k3 + k4)
    double k1r, k1p, k1rd, k1pd, k1td;
    double k2r, k2p, k2rd, k2pd, k2td;
    double k3r, k3p, k3rd, k3pd, k3td;
    double k4r, k4p, k4rd, k4pd, k4td;

    rhs(s.r,                   s.phi,                   s.rdot,                   s.phidot,                   s.tdot,                   M, k1r, k1p, k1rd, k1pd, k1td);
    rhs(s.r + 0.5*dtau*k1r,   s.phi + 0.5*dtau*k1p,   s.rdot + 0.5*dtau*k1rd,   s.phidot + 0.5*dtau*k1pd,   s.tdot + 0.5*dtau*k1td,   M, k2r, k2p, k2rd, k2pd, k2td);
    rhs(s.r + 0.5*dtau*k2r,   s.phi + 0.5*dtau*k2p,   s.rdot + 0.5*dtau*k2rd,   s.phidot + 0.5*dtau*k2pd,   s.tdot + 0.5*dtau*k2td,   M, k3r, k3p, k3rd, k3pd, k3td);
    rhs(s.r +     dtau*k3r,   s.phi +     dtau*k3p,   s.rdot +     dtau*k3rd,   s.phidot +     dtau*k3pd,   s.tdot +     dtau*k3td,   M, k4r, k4p, k4rd, k4pd, k4td);

    const double h6 = dtau / 6.0;
    TimelikeState out{};
    out.tau    = s.tau    + dtau;
    out.r      = s.r      + h6*(k1r  + 2.0*k2r  + 2.0*k3r  + k4r);
    out.phi    = s.phi    + h6*(k1p  + 2.0*k2p  + 2.0*k3p  + k4p);
    out.rdot   = s.rdot   + h6*(k1rd + 2.0*k2rd + 2.0*k3rd + k4rd);
    out.phidot = s.phidot + h6*(k1pd + 2.0*k2pd + 2.0*k3pd + k4pd);
    out.tdot   = s.tdot   + h6*(k1td + 2.0*k2td + 2.0*k3td + k4td);
    return out;
}

std::vector<TimelikeState> integrate_timelike(
    const TimelikeState& ic,
    double M,
    double tau_total,
    double dtau,
    int store_every)
{
    std::vector<TimelikeState> traj;
    traj.reserve(static_cast<size_t>(tau_total / (dtau * store_every)) + 2);
    traj.push_back(ic);

    const double r_horizon = rs(M) * 1.001;
    TimelikeState s = ic;
    long long step  = 0;

    while (s.tau < tau_total) {
        double h = dtau;
        if (s.tau + h > tau_total) h = tau_total - s.tau;  // don't overshoot

        s = rk4_step(s, M, h);
        ++step;

        if (s.r < r_horizon) break;  // swallowed

        if (step % store_every == 0) traj.push_back(s);
    }

    // Always include final state
    if (traj.back().tau < s.tau) traj.push_back(s);
    return traj;
}

// ─── Null geodesic (u-equation) ───────────────────────────────────────────
// d²u/dφ² + u = 3Mu²
//
// Derivation (brief): Along an equatorial null geodesic, conservation of
// E and L gives  (dr/dφ)² = r⁴/b² − r² + 2Mr  (b = L/E impact param).
// Substituting u = 1/r and differentiating w.r.t. φ:
//   d²u/dφ² = −u + 3Mu²    [MTW eq. 25.40]
// The 3Mu² term is the GR correction; without it the orbit is a conic.

static void null_rhs(const NullState& y, double M, double& du, double& ddu) noexcept
{
    du  = y[1];              // du/dφ  = y[1]
    ddu = 3.0*M*y[0]*y[0]   // d²u/dφ² = 3Mu² − u
          - y[0];
}

NullState null_rk4_step(NullState s, double M, double dphi) noexcept
{
    double k1u, k1du, k2u, k2du, k3u, k3du, k4u, k4du;

    null_rhs(s,                                         M, k1u, k1du);
    null_rhs({s[0]+0.5*dphi*k1u, s[1]+0.5*dphi*k1du}, M, k2u, k2du);
    null_rhs({s[0]+0.5*dphi*k2u, s[1]+0.5*dphi*k2du}, M, k3u, k3du);
    null_rhs({s[0]+    dphi*k3u, s[1]+    dphi*k3du}, M, k4u, k4du);

    const double h6 = dphi / 6.0;
    return {
        s[0] + h6*(k1u  + 2.0*k2u  + 2.0*k3u  + k4u),
        s[1] + h6*(k1du + 2.0*k2du + 2.0*k3du + k4du)
    };
}

std::vector<NullState> integrate_null(
    NullState ic,
    double M,
    double dphi,
    double r_escape,
    int max_steps)
{
    std::vector<NullState> traj;
    traj.reserve(std::min(max_steps, 1 << 20));
    traj.push_back(ic);

    const double u_horizon = 1.0 / (rs(M) * 1.001);
    const double u_escape  = (r_escape > 0.0) ? 1.0 / r_escape : 0.0;

    NullState s = ic;
    for (int i = 0; i < max_steps; ++i) {
        s = null_rk4_step(s, M, dphi);
        traj.push_back(s);

        if (s[0] > u_horizon)         break;  // horizon
        if (s[0] < u_escape && s[0] > 0.0) break;  // escaped
        if (s[0] < 0.0)               break;  // unphysical (strong lensing wrap)
    }
    return traj;
}

double deflection_angle(double b, double M, double dphi)
{
    // Initial conditions: start at periapsis (du/dφ = 0).
    //
    // Periapsis u_p satisfies the first integral at du/dφ = 0:
    //   (du/dφ)² = 1/b² − u² + 2Mu³ = 0
    //   →  u_p² − 2M u_p³ = 1/b²
    //
    // Solve via Newton iteration (converges in 2–3 steps for b >> M):
    double u_p = 1.0 / b;
    for (int iter = 0; iter < 30; ++iter) {
        const double f  = u_p*u_p  - 2.0*M*u_p*u_p*u_p - 1.0/(b*b);
        const double fp = 2.0*u_p  - 6.0*M*u_p*u_p;
        const double du = -f / fp;
        u_p += du;
        if (std::abs(du) < 1.0e-15 * std::abs(u_p)) break;
    }

    // Start a tiny step past periapsis so du/dφ < 0 (outgoing).
    // Use the first integral to compute exact magnitude:
    //   (du/dφ)² = 1/b² − u² + 2Mu³
    // At u slightly less than u_p, this is small but non-zero.
    const double u0 = u_p * (1.0 - 1.0e-9);
    const double dudphi2 = 1.0/(b*b) - u0*u0 + 2.0*M*u0*u0*u0;
    // du/dφ < 0 on the outgoing leg (u decreasing from periapsis)
    NullState s{u0, -std::sqrt(std::max(dudphi2, 0.0))};

    // Termination: u < u_escape means r > 10^6 b  (effectively ∞)
    const double u_escape  = u_p * 1.0e-9;
    const double u_horizon = 1.0 / (rs(M) * 1.001);

    // Integrate from periapsis (φ=0) outward; count angle swept.
    double phi = 0.0;
    for (int i = 0; i < 100'000'000; ++i) {
        s = null_rk4_step(s, M, dphi);
        phi += dphi;

        if (s[0] > u_horizon) return std::numeric_limits<double>::quiet_NaN(); // captured
        if (s[0] <= u_escape || s[0] < 0.0) break;
    }

    // In flat space the outgoing leg sweeps exactly π/2.
    // Deflection = 2 * (phi_swept − π/2) = 2φ − π.
    return 2.0 * phi - M_PI;
}

} // namespace bh

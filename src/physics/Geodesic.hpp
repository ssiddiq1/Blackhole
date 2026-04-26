#pragma once
// Geodesic.hpp — CPU-side geodesic integrators for Schwarzschild spacetime.
//
// Two integrators:
//   1. Timelike geodesics (massive particles): full 4D Schwarzschild EOM via RK4.
//   2. Null geodesics (photons):  reduced u-equation  d²u/dφ² + u = 3Mu²  via RK4.
//
// Both restrict motion to the equatorial plane (θ = π/2), which is
// consistent since the equatorial plane is totally geodesic in Schwarzschild.
//
// References:
//   MTW  = Misner, Thorne, Wheeler, "Gravitation" (1973)
//   L79  = Luminet, A&A 75, 228 (1979)
//   J15  = James et al., CQG 32 065001 (2015)  [Interstellar paper]

#include "Schwarzschild.hpp"
#include <array>
#include <limits>
#include <vector>

namespace bh {

// ── Timelike geodesic ─────────────────────────────────────────────────────
// State vector in equatorial Schwarzschild coordinates.
// Proper time τ is the affine parameter.
struct TimelikeState {
    double tau;     // proper time
    double r;
    double phi;
    double rdot;    // dr/dτ
    double phidot;  // dφ/dτ
    double tdot;    // dt/dτ  (kept explicit so we can monitor energy drift)
};

// Initial conditions for a circular orbit at r = r0 (r0 > 3M).
// Sets ṙ = 0, computes tdot and phidot from conserved E and L.
TimelikeState circular_orbit_ic(double r0, double M);

// Specific energy  E = f(r) · ṫ  (conserved along geodesic) [MTW eq. 25.13]
inline double specific_energy(const TimelikeState& s, double M) noexcept {
    return lapse(s.r, M) * s.tdot;
}

// Specific angular momentum  L = r² · φ̇  (conserved) [MTW eq. 25.13]
inline double specific_ang_mom(const TimelikeState& s) noexcept {
    return s.r * s.r * s.phidot;
}

// Metric norm  g_μν u^μ u^ν = −f ṫ² + (1/f) ṙ² + r² φ̇²
// Should equal −1 for a timelike geodesic (massive particle).
double metric_norm(const TimelikeState& s, double M) noexcept;

// Single RK4 step of proper time dtau.
TimelikeState rk4_step(const TimelikeState& s, double M, double dtau) noexcept;

// Integrate for tau_total of proper time; store every store_every steps.
// Terminates early on horizon crossing (r < 1.001 * rs).
// Returns trajectory including initial state and final state.
std::vector<TimelikeState> integrate_timelike(
    const TimelikeState& ic,
    double M,
    double tau_total,
    double dtau,
    int store_every = 1);

// ── Null geodesic ─────────────────────────────────────────────────────────
// Reduced u-equation in equatorial plane:
//   d²u/dφ² + u = 3Mu²    where u ≡ 1/r
//
// Derived by eliminating τ from the photon geodesic equations using
// conservation of energy E and angular momentum L (b = L/E = impact param).
// [MTW eq. 25.40; L79 eq. 3]
//
// State = {u, du/dφ}.  φ is the affine angle along the orbit.
using NullState = std::array<double, 2>;

// Single RK4 step in angle dphi (radians).
NullState null_rk4_step(NullState s, double M, double dphi) noexcept;

// Integrate from initial conditions until:
//   horizon:  r < rs * (1 + eps)  → captured
//   escape:   r > r_escape        → escapes to infinity
// Returns trajectory of (u, du/dφ) pairs.
std::vector<NullState> integrate_null(
    NullState ic,
    double M,
    double dphi,
    double r_escape,
    int max_steps);

// Deflection angle for a photon with impact parameter b passing mass M.
// Integrates from periapsis outward; returns α = 2φ_asymptote − π.
// dphi controls angular resolution (default suitable for weak-field tests).
double deflection_angle(double b, double M, double dphi = 1.0e-6);

} // namespace bh

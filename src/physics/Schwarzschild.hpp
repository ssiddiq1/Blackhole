#pragma once
// Schwarzschild.hpp — metric constants and helper functions.
//
// Geometrized units throughout: G = c = 1.
// All lengths are in units of M (black hole mass parameter).
// Reference: Misner, Thorne, Wheeler (MTW), "Gravitation", Ch. 31.

#include <cmath>

namespace bh {

// ── Fundamental constants (geometrized units) ─────────────────────────────
constexpr double G_NEWTON = 1.0;   // gravitational constant
constexpr double C_LIGHT  = 1.0;   // speed of light

// ── Characteristic radii for Schwarzschild BH of mass M ──────────────────

// Schwarzschild radius: rs = 2GM/c² = 2M  [MTW eq. 31.1]
inline constexpr double rs(double M) noexcept { return 2.0 * M; }

// Innermost stable circular orbit (ISCO): r_ISCO = 6M  [MTW eq. 33.10b]
// Below this radius there are no stable timelike circular orbits.
inline constexpr double r_isco(double M) noexcept { return 6.0 * M; }

// Photon sphere: r_ph = 3M  [MTW eq. 25.40]
// Unstable circular orbit for null geodesics.
inline constexpr double r_photon_sphere(double M) noexcept { return 3.0 * M; }

// Critical impact parameter for photon orbit: b_crit = 3√3 M  [MTW eq. 25.42]
inline double b_critical(double M) noexcept { return 3.0 * std::sqrt(3.0) * M; }

// ── Metric components ─────────────────────────────────────────────────────
// Schwarzschild metric in Schwarzschild coordinates (t, r, θ, φ):
//   ds² = -f dt² + f⁻¹ dr² + r² dΩ²
//   where f(r) = 1 - 2M/r  (lapse function)
//
// Signature convention: (−,+,+,+)

inline double lapse(double r, double M) noexcept { return 1.0 - 2.0*M/r; }
inline double g_tt (double r, double M) noexcept { return -lapse(r, M); }
inline double g_rr (double r, double M) noexcept { return  1.0 / lapse(r, M); }
// g_θθ = r²,  g_φφ = r² sin²θ  (trivial; not needed as functions)

// ── ISCO conserved quantities ─────────────────────────────────────────────
// Derivation: circular orbit requires dr/dτ = 0 and d²r/dτ² = 0.
// From the effective potential V_eff(r) = f(r)(1 + L²/r²):
//   Circular orbit: dV_eff/dr = 0  →  L² = Mr²/(r − 3M)
//   Marginally stable: d²V_eff/dr² = 0  →  r_ISCO = 6M
//
// Specific angular momentum at ISCO: L_ISCO = 2√3 M  [MTW eq. 33.12a]
inline double L_isco(double M) noexcept { return 2.0 * std::sqrt(3.0) * M; }

// Specific energy at ISCO: E_ISCO = √(8/9) ≈ 0.9428  [MTW eq. 33.12b]
// Binding energy = 1 − E_ISCO ≈ 5.72% (maximum efficiency for thin disk)
inline constexpr double E_isco() noexcept { return 0.9428090415820634; } // sqrt(8.0/9.0)

// ── Geodesic circular-orbit quantities at arbitrary r ─────────────────────
// L²(r) = Mr²/(r−3M) for r > 3M  [from dV_eff/dr = 0]
inline double L_circular_sq(double r, double M) noexcept {
    return M * r * r / (r - 3.0*M);
}

// E²(r) = f(r)² / (1−3M/r)  [from normalization + L(r)]
inline double E_circular_sq(double r, double M) noexcept {
    const double f = lapse(r, M);
    return f * f / (1.0 - 3.0*M/r);
}

} // namespace bh

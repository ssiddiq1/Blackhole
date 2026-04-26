#pragma once
// Tidal.hpp — Tidal forces (Riemann tensor) in Schwarzschild spacetime.
//
// Physical picture (orthonormal frame of a freely-falling observer):
//   a_radial     = +2M/r³ · δr    (STRETCHING, positive radial eigenvalue)
//   a_tangential = −M/r³  · δl    (COMPRESSION, tangential eigenvalue)
//
// This is the classic "spaghettification" tidal pattern.  Reference:
//   MTW §31.2, eqs. 31.6–31.8  (Schwarzschild Riemann components)
//   Hartle §9.6                  (tidal forces and the Roche limit)

#include "physics/Schwarzschild.hpp"
#include <array>

namespace bh {

// ── Tidal eigenvalues in the freely-falling orthonormal frame ──────────────
// Returns { λ_radial, λ_θ, λ_φ } — diagonal of the "electric" part of the
// Weyl tensor. Units: 1/length².
inline std::array<double, 3> tidal_eigenvalues(double r, double M) noexcept {
    const double k = M / (r * r * r);
    return { +2.0 * k, -k, -k };
}

// Radial stretching coefficient  (d²δr/dτ² = +coef · δr for infalling pair)
inline double tidal_radial_coef(double r, double M) noexcept {
    return 2.0 * M / (r * r * r);
}

// Tangential compression coefficient
inline double tidal_tangential_coef(double r, double M) noexcept {
    return -M / (r * r * r);
}

// ── Riemann tensor component in coordinate basis ──────────────────────────
// R^r_trt (mixed-index) in Schwarzschild coordinates.
// Sign convention: MTW (−,+,+,+).  Derivation: MTW eq. 31.4–31.5.
//
//   R^r_trt = −(2M/r³) · f(r)         where f = 1 − 2M/r
//
// When applied to a static observer (u^t = 1/√f) via the geodesic-deviation
// equation, this produces the +2M/r³ radial stretch above.
inline double riemann_r_trt(double r, double M) noexcept {
    return -2.0 * M * lapse(r, M) / (r * r * r);
}

// ── Dumbbell break condition ───────────────────────────────────────────────
// A rigid bond of separation L survives until the local tidal stress per
// unit mass (acceleration differential across the body) exceeds the bond
// energy density.  Model: break when
//     (2M/r³) · L > E_bond       [same sign as radial stretching]
inline bool tidal_exceeds_bond(double r, double L, double M, double bond_energy) noexcept {
    return (2.0 * M * L) / (r * r * r) > bond_energy;
}

} // namespace bh

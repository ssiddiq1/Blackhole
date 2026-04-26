// ParticleSystem.cpp
#include "sim/ParticleSystem.hpp"
#include <cmath>

namespace bh {

// ── Particle ───────────────────────────────────────────────────────────────

glm::vec3 Particle::pos3d() const noexcept
{
    float c = std::cos((float)state.phi);
    float s = std::sin((float)state.phi);
    return (float)state.r * (c * e_r + s * e_phi);
}

void Particle::push_trail()
{
    trail[trail_head] = pos3d();
    trail_head  = (trail_head + 1) % TRAIL_CAP;
    if (trail_count < TRAIL_CAP) ++trail_count;
}

// ── ParticleSystem ─────────────────────────────────────────────────────────

// Build initial conditions from a 3-D spawn position and velocity direction.
//
// Orbital plane:  e_z = normalise(e_r × vel_dir),  e_phi = e_z × e_r.
//
// Free-fall IC (speed <= 0): E = 1 (infalling from infinity), radial speed
//   ṙ = −√(2M/r), half-Keplerian φ̇ for visual interest.
// Velocity-directed IC: project vel_dir onto (e_r, e_phi); normalise with
//   the metric to obtain tdot.
Particle ParticleSystem::make_particle(
    glm::vec3 pos, glm::vec3 vel_dir, float speed, float M)
{
    Particle p{};
    p.M = M;

    double r0 = (double)glm::length(pos);
    p.e_r = glm::normalize(pos);

    // ── Orbital plane basis ──────────────────────────────────────────────
    glm::vec3 L_vec = glm::cross(p.e_r, vel_dir);
    float L_mag = glm::length(L_vec);
    glm::vec3 e_z;
    if (L_mag < 1.0e-4f) {
        // Purely radial: choose an orthogonal axis
        glm::vec3 ref = (std::abs(p.e_r.z) < 0.9f)
                      ? glm::vec3(0.0f, 0.0f, 1.0f)
                      : glm::vec3(0.0f, 1.0f, 0.0f);
        e_z = glm::normalize(glm::cross(p.e_r, ref));
    } else {
        e_z = L_vec / L_mag;
    }
    p.e_phi = glm::cross(e_z, p.e_r);   // unit vec (e_z, e_r are orthonormal)

    // ── Initial conditions ───────────────────────────────────────────────
    double f0 = 1.0 - 2.0 * (double)M / r0;
    f0 = std::max(f0, 1.0e-6);

    double rdot0, phidot0, tdot0;

    if (speed <= 0.0f) {
        // Free-fall from infinity: E = 1, ṙ = −√(2M/r) [MTW eq. 25.28]
        rdot0   = -std::sqrt(2.0 * (double)M / r0);
        phidot0 = 0.5 * std::sqrt((double)M / (r0 * r0 * r0));  // half-Keplerian φ̇
        tdot0   = 1.0 / f0;                                       // E = f·ṫ = 1
    } else {
        // Project requested velocity onto orbital plane components
        double vr   = (double)glm::dot(vel_dir, p.e_r)   * (double)speed;
        double vphi = (double)glm::dot(vel_dir, p.e_phi)  * (double)speed;
        rdot0   = vr;
        phidot0 = vphi / r0;
        // Timelike normalisation: −f ṫ² + ṙ²/f + r²φ̇² = −1  →  ṫ = √(...)
        double norm = 1.0 + rdot0 * rdot0 / f0 + r0 * r0 * phidot0 * phidot0;
        tdot0 = std::sqrt(std::max(norm / f0, 1.0));
    }

    p.state.tau    = 0.0;
    p.state.r      = r0;
    p.state.phi    = 0.0;
    p.state.rdot   = rdot0;
    p.state.phidot = phidot0;
    p.state.tdot   = tdot0;
    p.alive        = true;

    p.push_trail();
    return p;
}

void ParticleSystem::step_particle(Particle& p, double dtau)
{
    if (!p.alive) return;

    p.state = rk4_step(p.state, (double)p.M, dtau);

    // Terminate when inside the horizon (with small safety margin)
    if (p.state.r < rs((double)p.M) * 1.01) {
        p.alive = false;
        return;
    }

    p.push_trail();
}

void ParticleSystem::spawn_particle(
    glm::vec3 pos, glm::vec3 vel_dir, float speed, float M)
{
    m_particles.push_back(make_particle(pos, vel_dir, speed, M));
}

void ParticleSystem::spawn_dumbbell(
    glm::vec3 pos, float separation, float bond_energy, float M)
{
    float     r0  = glm::length(pos);
    glm::vec3 e_r = glm::normalize(pos);

    // Offset the two masses along e_r; both inherit a purely radial fall.
    glm::vec3 pos0 = (r0 + 0.5f * separation) * e_r;
    glm::vec3 pos1 = (r0 - 0.5f * separation) * e_r;
    glm::vec3 vdir = -e_r;   // toward BH

    Dumbbell db{};
    db.p0          = make_particle(pos0, vdir, -1.0f, M);
    db.p1          = make_particle(pos1, vdir, -1.0f, M);
    db.bond_energy = bond_energy;
    db.L0          = separation;
    db.broken      = false;

    m_dumbbells.push_back(db);
}

void ParticleSystem::update(double dtau)
{
    for (auto& p : m_particles)
        step_particle(p, dtau);

    for (auto& db : m_dumbbells) {
        step_particle(db.p0, dtau);
        step_particle(db.p1, dtau);

        if (!db.broken && db.p0.alive && db.p1.alive) {
            double r_mid = 0.5 * (db.p0.state.r + db.p1.state.r);
            double L_cur = std::abs(db.p0.state.r - db.p1.state.r);
            if (L_cur < 1.0e-4) L_cur = (double)db.L0;
            if (tidal_exceeds_bond(r_mid, L_cur, db.p0.M, db.bond_energy))
                db.broken = true;
        }
    }
}

} // namespace bh

#pragma once
// ParticleSystem.hpp — 3-D timelike geodesic particles in Schwarzschild spacetime.
//
// Each particle integrates the full 4-D geodesic ODE (Geodesic.hpp::rk4_step)
// in its own orbital plane.  The orbital plane is fixed at spawn time from the
// initial position and velocity, encoded as an orthonormal frame (e_r, e_phi)
// in 3-D world space.
//
// 3-D reconstruction:  pos3D = r · (cos φ · e_r + sin φ · e_phi)
//
// Dumbbells are two independent particles whose separation is monitored against
// the tidal-break threshold from Tidal.hpp::tidal_exceeds_bond.  After breaking,
// both particles continue their individual geodesics; only the 'broken' flag changes.
//
// References:
//   Geodesic.hpp  — RK4 integrator and TimelikeState definition
//   Tidal.hpp     — tidal_exceeds_bond break condition

#include "physics/Geodesic.hpp"
#include "physics/Tidal.hpp"
#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace bh {

static constexpr int TRAIL_CAP = 256;   // positions kept per particle

struct Particle {
    TimelikeState state{};    // 2-D (r, φ) geodesic state
    glm::vec3     e_r{};      // initial radial unit vector in world space
    glm::vec3     e_phi{};    // tangential unit vector  (= e_z × e_r)
    float         M = 1.0f;
    bool          alive = true;

    // Ring buffer of 3-D world positions (newest at trail[trail_head-1])
    std::array<glm::vec3, TRAIL_CAP> trail{};
    int trail_head  = 0;
    int trail_count = 0;

    glm::vec3 pos3d() const noexcept;
    void      push_trail();
};

struct Dumbbell {
    Particle p0, p1;
    float    bond_energy = 0.05f;
    float    L0          = 0.5f;   // initial separation (saved for break test)
    bool     broken      = false;
};

class ParticleSystem {
public:
    // Spawn a test particle at world position pos.
    // vel_dir: desired velocity direction (normalised).
    // speed <= 0 → use free-fall from infinity IC (E=1, ṙ=−√(2M/r), half-Keplerian φ̇).
    void spawn_particle(glm::vec3 pos, glm::vec3 vel_dir, float speed, float M);

    // Spawn a dumbbell at pos; two particles offset ±separation/2 radially.
    void spawn_dumbbell(glm::vec3 pos, float separation, float bond_energy, float M);

    // Advance every particle and dumbbell by one proper-time step dtau.
    void update(double dtau);

    const std::vector<Particle>&  particles() const { return m_particles; }
    const std::vector<Dumbbell>&  dumbbells() const { return m_dumbbells; }
    std::vector<Dumbbell>&        dumbbells()       { return m_dumbbells; }

    void clear()  { m_particles.clear(); m_dumbbells.clear(); }
    int  count()  const { return (int)m_particles.size() + 2*(int)m_dumbbells.size(); }

private:
    static Particle make_particle(glm::vec3 pos, glm::vec3 vel_dir, float speed, float M);
    static void     step_particle(Particle& p, double dtau);

    std::vector<Particle>  m_particles;
    std::vector<Dumbbell>  m_dumbbells;
};

} // namespace bh

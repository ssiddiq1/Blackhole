#pragma once
// Scene.hpp — Top-level simulation state.
//
// Owns the ParticleSystem and runs it at a fixed 60 Hz internal timestep,
// regardless of display frame rate.  Also stores the black-hole and disk
// parameters that are shared by the renderer and the simulation.

#include "sim/ParticleSystem.hpp"

namespace bh {

struct SceneConfig {
    float M          = 1.0f;
    float disk_r_in  = 6.0f;    // default = ISCO
    float disk_r_out = 20.0f;
    bool  disk_on    = true;
};

class Scene {
public:
    explicit Scene(SceneConfig cfg = {}) : m_cfg(cfg) {}

    // Change BH mass; resets disk radii to ISCO/20M and clears particles.
    void set_mass(float M);

    void set_disk_on  (bool on)            { m_cfg.disk_on = on; }
    void set_disk_radii(float r_in, float r_out) {
        m_cfg.disk_r_in  = r_in;
        m_cfg.disk_r_out = r_out;
    }

    // Advance simulation by wall-clock dt; uses fixed 60 Hz substeps internally.
    void update(float dt);

    // Spawn helpers — use Scene's current M if not overridden.
    void spawn_particle(glm::vec3 pos,
                        glm::vec3 vel_dir = glm::vec3(0.0f),
                        float speed = -1.0f);
    void spawn_dumbbell(glm::vec3 pos,
                        float separation  = 1.0f,
                        float bond_energy = 0.04f);

    void clear() { m_psys.clear(); }

    const SceneConfig&    config()    const { return m_cfg; }
    const ParticleSystem& particles() const { return m_psys; }
    ParticleSystem&       particles()       { return m_psys; }

private:
    SceneConfig    m_cfg;
    ParticleSystem m_psys;
    double         m_accum = 0.0;

    static constexpr double FIXED_DT = 1.0 / 60.0;
    static constexpr double DTAU     = 0.005;
};

} // namespace bh

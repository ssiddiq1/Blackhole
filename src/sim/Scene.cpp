// Scene.cpp
#include "sim/Scene.hpp"

namespace bh {

void Scene::set_mass(float M)
{
    m_cfg.M         = M;
    m_cfg.disk_r_in  = 6.0f * M;
    m_cfg.disk_r_out = 20.0f * M;
    m_psys.clear();
}

void Scene::update(float dt)
{
    m_accum += (double)dt;
    while (m_accum >= FIXED_DT) {
        m_psys.update(DTAU);
        m_accum -= FIXED_DT;
    }
}

void Scene::spawn_particle(glm::vec3 pos, glm::vec3 vel_dir, float speed)
{
    glm::vec3 dir = (glm::length(vel_dir) < 1.0e-5f)
                  ? glm::normalize(-pos)
                  : glm::normalize(vel_dir);
    m_psys.spawn_particle(pos, dir, speed, m_cfg.M);
}

void Scene::spawn_dumbbell(glm::vec3 pos, float separation, float bond_energy)
{
    m_psys.spawn_dumbbell(pos, separation, bond_energy, m_cfg.M);
}

} // namespace bh

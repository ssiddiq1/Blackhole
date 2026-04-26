# 🌌 schwarzschild-rt

**A real-time black hole renderer that actually solves Einstein's equations.**

No tricks. No cube maps. No image warps. Every pixel on your screen is the endpoint of a photon trajectory numerically integrated through curved spacetime — at 60 FPS.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![OpenGL 4.1](https://img.shields.io/badge/OpenGL-4.1-red.svg)]()
[![Tests](https://img.shields.io/badge/tests-passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()

> *"Most black hole demos fake the lensing. This one doesn't."*

---

## ⚡ TL;DR

```bash
cmake --preset release && cmake --build --preset release -j
./build/bh_app
```

Then click. Watch a particle fall in. Spawn a dumbbell. Watch it tear apart. Cry softly.

---

## 🎬 What you're looking at

*(insert hero gif here — yes, I know)*

- 🕳️ **Real geodesic raytracing** — fragment shader integrates `d²u/dφ² + u = 3Mu²` per pixel, every frame
- 💫 **Accretion disk with redshift** — gravitational + Doppler shifting, not a flat texture
- 🪐 **Tidal disruption** — spawn a dumbbell, watch spaghettification break the bond
- ⭐ **Procedural starfield** — blackbody-colored, octahedrally mapped, hash-sampled
- 🎨 **HDR pipeline** — RGBA16F → bloom → ACES tonemap → your retinas

---

## 🤔 Wait, why does this matter?

Interstellar's black hole took a render farm. *Gargantua* was offline.

This runs on your laptop. In real time. **And you can fly into it.**

The renderer doesn't fake gravity — it integrates the Schwarzschild null geodesic equation per pixel using RK4. The particles don't fake orbits — they're full timelike geodesics in proper time. The tidal break threshold is derived from the actual `M/r³` scaling.

It's the difference between *animating physics* and *computing it*.

---

## 🛠️ The hard parts

| Problem | Why it hurts |
|---|---|
| ODE per pixel | You have ~16ms. Per frame. For everything. |
| Numerical hell near the horizon | Fixed-step RK4 wants to die at `r → 2M` |
| Two execution models | Double-precision CPU physics ↔ GLSL shaders |
| Honesty vs. framerate | How much physics can you afford to keep? |

The whole project is the answer to that last question.

---

## 🧪 What's real, what's faked

**Real:**
- Schwarzschild metric, ISCO, photon sphere, event horizon
- RK4 integration of null + timelike geodesics
- Tidal stretching scaling with `M/r³`
- Capture/escape determined by integration, not a fudge factor

**Approximated:**
- Disk emission: motivated by thin-disk theory, not full Novikov-Thorne radiative transfer
- Disk thickness: Gaussian-weighted samples around equator
- Starfield: procedural, not a real catalog

The README doesn't lie about which is which. The code separates the physics layer from the renderer so you can see for yourself.

---

## 🎮 Controls

| Key | Action |
|---|---|
| `WASD` `Space` `Shift` | Free-fly camera |
| Mouse | Look around |
| `Esc` | Toggle cursor / ImGui |
| Left click | Spawn test particle along your view |
| Right click | Spawn dumbbell aimed at the singularity |
| `Q` | Quit (and contemplate mortality) |

ImGui panels let you crank `M`, the disk radii, exposure, bloom, tonemap — all live.

---

## 🚀 Build

**Requires:** CMake 3.24+, C++20 compiler, OpenGL 4.1.

```bash
cmake --preset release
cmake --build --preset release -j
ctest --preset release
./build/bh_app
```

Dependencies (GLM, GLFW, Dear ImGui, doctest) are pulled by `FetchContent`. No vcpkg dance.

> ⚠️ **macOS first-class today.** The CMake has Linux/Windows branches but the source still includes `<OpenGL/gl3.h>` directly. PRs welcome — see the roadmap.

---

## 🧬 Architecture

```
src/
├── physics/   ← Schwarzschild helpers, RK4 integrators, tidal tensor
├── render/    ← OpenGL pipeline + the shader that does the magic
├── sim/       ← fixed-step 60Hz scene, particles, dumbbells
├── ui/        ← Dear ImGui controls
└── app/       ← window, input, main loop
```

**Three render passes:**
1. Geodesic HDR pass — fullscreen triangle, `geodesic.frag`, `RGBA16F` target
2. Bloom + ACES tonemap — half-res bright extract, two-tap blur, composite
3. Particle trails — dynamic VBO, age-faded line strips

**The single file you actually want to read:** `src/render/shaders/geodesic.frag`.

---

## ✅ Tests

This repo has *real physics tests*, not snapshot diffs:

- Circular orbit energy conservation over 10 ISCO orbits
- Analytic ISCO energy/angular momentum checks
- Metric normalization on timelike geodesics
- Tidal eigenvalue signs + trace in vacuum
- Stability at `r = 6M`, plunge inside ISCO

```bash
ctest --preset release  # all green
```

What's *not* tested yet: GPU image regression, photon deflection error envelopes, cross-GPU reproducibility. That's roadmap territory.

---

## 🗺️ Roadmap

- [ ] Image regression tests for fixed camera setups
- [ ] Expose `dphi` / `max_steps` in the UI
- [ ] Proper GL loader → Linux/Windows
- [ ] Adaptive stepping near photon sphere
- [ ] **Kerr metric** (spin, frame dragging) ← the big one
- [ ] Better disk emissivity + relativistic transfer
- [ ] Benchmark harness across resolutions

---

## 📚 If you want to understand the math

- Misner, Thorne, Wheeler — *Gravitation* (the brick)
- Hartle — *Gravity: An Introduction to Einstein's General Relativity*
- Luminet 1979 — first black hole image, done by hand
- James et al. 2015 — the *Interstellar* paper
- Novikov & Thorne 1973 — thin-disk accretion

---

## 💭 Why I built this

Because most black hole demos are screen-space distortion fields with a vignette, and the gap between *that* and *actually integrating the geodesic equation* is exactly the kind of gap I find interesting.

Also because spaghettification is funny.

---

**If you star this repo, a photon somewhere completes a full orbit at the photon sphere.** ⭐

*(That's not how physics works. Star it anyway.)*

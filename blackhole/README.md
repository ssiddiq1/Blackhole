# Schwarzschild Black Hole Simulator

A real-time Schwarzschild black hole simulator written in C++20 and OpenGL 4.1, with GPU fragment shaders numerically integrating null geodesics per pixel and CPU-side RK4 solvers advancing timelike geodesics for interactive particles. This is not a screen-space distortion trick: the renderer explicitly evolves photon trajectories in curved spacetime, accumulates accretion-disk emission at disk crossings, and composites the result through an HDR post pipeline.

## Recruiter 30-Second Summary

This repository sits at the intersection of computational physics, graphics, and systems engineering. It combines a tested Schwarzschild geodesics library, a real-time GPU lensing renderer, a fixed-step simulation layer for spawned particles and tidal-break dumbbells, and a clean CMake/OpenGL/ImGui application stack that is structured like a serious technical project rather than a one-off visual demo.

## Visual / Demo

Visual assets are not committed yet. Good additions here would be:

- A hero still of the lensed accretion disk and photon ring region
- A short captured orbit/fly-through clip
- A particle/dumbbell tidal-break GIF

## Status At A Glance

| Status | Scope |
| --- | --- |
| Implemented | Real-time GPU null-geodesic rendering, procedural starfield, physically motivated disk visualization, HDR + bloom + ACES-style post, CPU timelike geodesics, particle trails, tidal-break dumbbells, ImGui controls, doctest-based physics validation |
| Partial | Disk emission is motivated by Schwarzschild thin-disk ideas but is not full radiative transfer; validation is strong on the CPU physics side but does not yet include image regression or GPU invariants; CMake has non-Apple branches, but the source currently includes macOS OpenGL headers directly |
| Planned | Stronger portability, deeper validation, adaptive integration, Kerr/spin support, and more complete astrophysical modeling |

## Core Features

- Per-pixel null-geodesic integration in `src/render/shaders/geodesic.frag` using the reduced Schwarzschild photon equation `d²u/dφ² + u = 3Mu²`
- CPU timelike geodesic integration in `src/physics/Geodesic.cpp` using full Schwarzschild equations of motion and RK4 in proper time
- Interactive test particles with 3D orbit reconstruction from fixed orbital-plane bases
- Tidal-break dumbbells modeled as two infalling particles plus a bond-failure criterion from Schwarzschild tidal scaling
- Procedural starfield with octahedral environment mapping, hashed cell sampling, power-law brightness, and blackbody-colored stars
- Accretion-disk visualization with disk crossing detection, gravitational + Doppler frequency shift, Gaussian thickness sampling, and turbulence modulation
- HDR rendering path with RGBA16F intermediate targets, half-resolution bloom, and ACES-style filmic tonemapping
- Dear ImGui control panels for black-hole mass, disk parameters, camera parameters, post-processing, and scene spawning
- CMake-based build with fetched dependencies (`GLM`, `GLFW`, `Dear ImGui`, `doctest`)
- Unit tests for energy conservation, metric normalization, ISCO quantities, ISCO stability behavior, and tidal tensor sanity checks

## Why It Is Technically Interesting

Most black-hole demos fake lensing with image warps, cube maps, or hand-authored distortion fields. This code takes the harder route: it encodes the Schwarzschild equations directly, uses numerical integration on both CPU and GPU, keeps the physics layer separate from the renderer, and exposes the fidelity/performance tradeoff explicitly through step size, step count, disk shading, and post-processing design.

It is also a genuinely mixed-discipline project. To understand or extend it, you need comfort with general relativity, numerical ODE methods, shader programming, OpenGL resource management, frame-graph thinking, and the practical constraints of running a mathematically nontrivial simulation in real time.

## Why This Is Hard

- Schwarzschild lensing is not just a visual effect; the renderer has to solve an ODE for every pixel while staying inside a real-time frame budget.
- The interesting part of the spacetime is also the numerically awkward part: behavior changes rapidly near the photon sphere and horizon, where fixed-step methods can become expensive or fragile.
- The project has to bridge two very different execution models: scalar/double-precision CPU physics code and massively parallel GLSL fragment code with fixed budgets.
- The engineering challenge is balancing honesty and aesthetics: enough physics to be defensible, enough approximation to stay interactive, and enough systems discipline that the whole thing is maintainable.

## Physics Model

The simulation uses geometrized units throughout: `G = c = 1`, with the black-hole mass parameter `M` setting the scale. The metric is Schwarzschild, so the project models a non-rotating, spherically symmetric black hole with the expected characteristic radii:

- Event horizon: `r_s = 2M`
- Photon sphere: `r = 3M`
- ISCO: `r = 6M`

Timelike motion is integrated from the Schwarzschild geodesic equations in the equatorial plane, with explicit tracking of `t`, `r`, `phi`, `dr/dτ`, and `dφ/dτ`. Null motion for rendering uses the standard reduced photon equation in terms of `u = 1/r`, also in the equatorial plane.

That plane restriction is mathematically sensible here. In Schwarzschild spacetime every geodesic lies in a plane through the origin, so the particle system stores a 3D orbital basis per particle and reconstructs world-space motion from a 2D geodesic state.

## What Is Real vs What Is Approximated

### Real

- Schwarzschild characteristic scales and conserved circular-orbit quantities
- RK4 integration of timelike and null geodesics
- Proper use of geodesic-plane reduction for Schwarzschild trajectories
- Tidal stretching/compression scaling with `M / r^3`
- Explicit capture/escape logic based on radius during integration

### Approximated

- The accretion disk is a visualization model, not a full relativistic radiative-transfer solver
- Disk emissivity is a simplified radial profile with turbulence modulation, not a full Novikov-Thorne flux calculation end to end
- Disk thickness is represented by Gaussian-weighted samples around the equatorial plane
- Orbital disk velocity and frequency shift are simplified for interactive rendering
- The starfield is procedural, not observational data
- Dumbbells are not rigid-body or continuum-material simulations; they are two geodesics plus a tidal break threshold

## Numerical Integration Approach

### Null Geodesics (GPU)

The image-forming pass integrates the reduced Schwarzschild null equation

```text
d²u/dφ² + u = 3Mu²,  where u = 1/r
```

in the fragment shader with classical RK4. For each pixel, the shader:

1. Builds the camera ray in world space
2. Computes the ray's orbital plane from the camera position and ray direction
3. Derives the impact parameter and initial `u, du/dφ`
4. Advances the null geodesic with fixed angular step `dphi`
5. Stops on capture, escape, or budget exhaustion
6. Accumulates disk emission when the ray crosses the equatorial disk
7. Samples the procedural sky in the asymptotic escape direction

Current render-time defaults in the app are `max_steps = 1000` and `dphi = 0.02`. That is the core quality/performance lever in the renderer.

### Timelike Geodesics (CPU)

Massive particles use the full Schwarzschild equations of motion with a six-component state:

- `tau`
- `r`
- `phi`
- `rdot`
- `phidot`
- `tdot`

Those equations are integrated in double precision with RK4 in `src/physics/Geodesic.cpp`. The code exposes helpers for circular-orbit initial conditions, conserved specific energy/angular momentum, metric norm checks, and a standalone photon deflection routine for CPU-side analysis.

### Simulation Stepping

The top-level scene runs on a fixed 60 Hz internal update loop regardless of display frame rate. Within each simulation tick, particle evolution uses a fixed proper-time step `DTAU = 0.005`, which keeps the simulation deterministic relative to the scene update cadence and decouples it from rendering jitter.

## Renderer Architecture

The renderer is a compact multi-pass OpenGL 4.1 pipeline:

### Pass A: Geodesic HDR Pass

- Fullscreen triangle
- `geodesic.frag`
- Writes HDR radiance into a full-resolution `RGBA16F` framebuffer
- Consumes a `std140` uniform block mirrored exactly by `SceneUBO` in C++

### Pass B: Post / Bloom / Tonemap

- Bright extract + horizontal blur into half-resolution bloom target
- Vertical blur into second half-resolution bloom target
- Composite HDR + bloom back to the default framebuffer
- Apply ACES-style filmic tonemap (or Reinhard fallback) and gamma correction

### Pass C: Particle Trails

- Dynamic VBO update each frame
- Line-strip rendering with age-based alpha fade and trail-type coloring
- Separate pass over the default framebuffer after the main image composite

The implementation also pays attention to practical graphics details that matter in real projects:

- Explicit framebuffer recreation on resize
- Uniform-block layout assertions on the C++ side
- Fullscreen-triangle rendering instead of quad boilerplate
- Debug GL error polling in non-release builds
- Half-resolution bloom to keep the post cost under control

## Controls / Interaction

Runtime controls currently implemented:

- `W`, `A`, `S`, `D`, `Space`, `Left Shift`: free-fly camera translation
- Mouse drag while captured: look / aim
- `Escape`: toggle cursor capture and switch between camera control and ImGui interaction
- Left click while captured: spawn a test particle at the camera position, launched along the camera forward vector
- Right click while captured: spawn a dumbbell aimed toward the black hole
- `Q`: quit

ImGui panels expose:

- Black-hole mass
- Disk enable/disable and disk radii
- Camera elevation / orbit radius / FOV
- Disk visual parameters (`base_gain`, `T_peak`, turbulence amplitude, scale height)
- Post-processing parameters (exposure, bloom, tonemap)
- Spawn / clear actions
- FPS stats

## Build Instructions

### Requirements

- CMake 3.24+
- A C++20 compiler
- An OpenGL 4.1-capable environment

Dependencies are fetched automatically at configure time via CMake `FetchContent`.

### Build

```bash
cmake --preset release
cmake --build --preset release -j
ctest --preset release
./build/bh_app
```

Debug preset:

```bash
cmake --preset debug
cmake --build --preset debug -j
ctest --preset debug
./build/debug/bh_app
```

### Portability Note

The build system already contains Apple and non-Apple link branches, but the current source includes `<OpenGL/gl3.h>` directly in multiple translation units. In practice, that makes the checked-in codepath macOS-oriented today; Linux/Windows portability is a reasonable next step, not a solved problem yet.

## File Structure

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── cmake/
│   └── deps.cmake
├── src/
│   ├── app/       # window creation, input, main loop
│   ├── physics/   # Schwarzschild helpers, geodesic integrators, tidal model
│   ├── render/    # renderer + GLSL shaders
│   ├── sim/       # fixed-step scene, particles, dumbbells
│   └── ui/        # Dear ImGui layer
└── tests/
    └── test_geodesic.cpp
```

Key files:

- `src/render/shaders/geodesic.frag`: the core real-time lensing and disk shader
- `src/physics/Geodesic.cpp`: CPU geodesic integrators and conserved-quantity helpers
- `src/sim/ParticleSystem.cpp`: 3D particle reconstruction and dumbbell breakup logic
- `src/render/Renderer.cpp`: OpenGL resource setup and pass orchestration
- `tests/test_geodesic.cpp`: analytic and numerical sanity checks

## Tests And Validation

This repository has real physics tests, not just visual eyeballing.

Current doctest coverage includes:

- Circular-orbit energy conservation over ten ISCO orbits
- Analytic ISCO energy and angular momentum checks
- Metric normalization for timelike geodesics
- Tidal eigenvalue sign and trace checks in vacuum Schwarzschild spacetime
- Stability behavior at `r = 6M` and inward spiral behavior inside the ISCO

The current CTest wiring passes under the release preset:

```bash
ctest --preset release
```

What is validated well today:

- CPU-side Schwarzschild helpers
- CPU RK4 timelike integration behavior
- Tidal scaling and sign conventions

What is not yet validated deeply enough:

- GPU image regression
- Quantitative photon-deflection error envelopes for render settings
- Cross-GPU reproducibility / precision characterization
- Performance benchmarks tracked over time

## Performance Notes

This is a real-time numerical renderer, so performance is dominated by per-pixel geodesic stepping, not by traditional scene complexity.

Important current tradeoffs:

- Resolution scales the null-geodesic workload directly
- `max_steps` and `dphi` set the main quality/performance budget
- Disk emission adds extra work at equatorial crossings
- Bloom is deliberately half resolution
- VSync is disabled in the window layer for uncapped FPS measurement
- Particle trails are lightweight relative to the main geodesic pass

The app prints live FPS and camera radius to stdout once per second. There is no formal benchmark harness yet, so performance claims should be treated as hardware-dependent and qualitative at this stage.

## Limitations

- Schwarzschild only: no spin, no frame dragging, no Kerr lensing
- Fixed-step RK4 only: no adaptive stepping or error control
- Null geodesics are integrated in a reduced form optimized for this renderer, not with a general 4D ray state
- Disk shading is visually and physically motivated, but not a full astrophysical emission/transfer model
- The plunge region is handled with a simple emission cutoff rather than a richer accretion model
- The test suite is CPU-physics focused; renderer correctness is still mostly established by construction and inspection
- The current source layout is cleaner than the portability story; macOS is the obvious first-class platform right now

## Roadmap

Good next steps, in roughly descending order of leverage:

1. Add quantitative validation for photon deflection and image formation, including regression images for fixed camera setups.
2. Expose geodesic render controls such as `dphi` and `max_steps` in the UI instead of leaving them as app-level constants.
3. Replace platform-specific OpenGL includes with a proper loader strategy and verify Linux/Windows builds.
4. Add adaptive stepping near the photon sphere / horizon to improve accuracy-per-millisecond.
5. Extend from Schwarzschild to Kerr to capture spin and frame dragging.
6. Deepen the disk model with better emissivity, redshift handling, and possibly a more principled relativistic transfer treatment.
7. Add automated performance benchmarking across resolutions and parameter regimes.

## References

- Misner, Thorne, Wheeler, *Gravitation* (1973)
- Novikov and Thorne, thin-disk accretion model (1973)
- Luminet, *A&A* 75, 228 (1979)
- James et al., *Classical and Quantum Gravity* 32, 065001 (2015)
- Hartle, *Gravity: An Introduction to Einstein's General Relativity*
- Tanner Helland blackbody color approximation

---

If you are reading this as an engineer or researcher: the interesting part of this repository is not just that it renders something cool, but that it treats black-hole visualization as a numerical methods problem, a graphics architecture problem, and a software design problem at the same time.

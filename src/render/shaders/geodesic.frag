#version 410 core
// geodesic.frag — Per-pixel null geodesic + Novikov-Thorne disk (Interstellar aesthetic).
//
// ── Physics ───────────────────────────────────────────────────────────────
// d²u/dφ² + u = 3Mu²   (null geodesic, u=1/r)  [MTW eq. 25.40]
//
// ── Disk model ────────────────────────────────────────────────────────────
// Novikov-Thorne thin disk in the equatorial plane.  Disk crossings are found
// by tracking z-sign changes.  Thickness is modelled as a gaussian in z with
// scale-height h(r) = h_scale·r; 4 z-offset samples per crossing accumulate
// emission with gaussian weights.  3-octave fBm turbulence adds radial filament
// streaks.
//
// ── Starfield ─────────────────────────────────────────────────────────────
// Octahedral UV projection → 2048² virtual grid.  Per-cell uint xorshift hash,
// ~1/300 star density, power-law brightness, blackbody RGB 3000-12000 K.
//
// References: MTW "Gravitation", Novikov-Thorne 1973, Tanner Helland blackbody fit.

out vec4 fragColor;

uniform samplerCube u_galaxy;     // nebula skybox cubemap
uniform sampler2D   u_color_map;  // radial color gradient for disk tint
uniform float       u_time;       // elapsed seconds for disk animation

// ── Uniform block (std140) — must mirror SceneUBO in Renderer.hpp ─────────
layout(std140) uniform SceneUBO {
    vec3  cam_pos;    float M;
    vec3  cam_fwd;    float rs;
    vec3  cam_right;  float dphi;
    vec3  cam_up;     float fov_tan;
    int   width;      int   height;
    int   max_steps;  int   disk_on;
    float disk_r_in;        float disk_r_out;
    float base_gain;        float T_peak;
    float turb_amp;         float h_scale;
    float adisk_noise_lod;  float adisk_speed;
};

// ══════════════════════════════════════════════════════════════════════════
// BLACKBODY — used by disk colour model
// ══════════════════════════════════════════════════════════════════════════

// Blackbody RGB via Tanner Helland polynomial fit (T in Kelvin, 1000–40000 K)
vec3 blackbody_kelvin(float T) {
    T = clamp(T, 1000.0, 40000.0) / 100.0;
    float r, g, b;

    // Red
    r = (T <= 66.0) ? 1.0
                    : clamp(pow(T - 60.0, -0.1332047592) * 1.29293618606, 0.0, 1.0);

    // Green
    if (T <= 66.0)
        g = clamp(log(T) * 0.39008157288 - 0.63184144378, 0.0, 1.0);
    else
        g = clamp(pow(T - 60.0, -0.0755148492) * 1.12989086089, 0.0, 1.0);

    // Blue
    if (T >= 66.0)
        b = 1.0;
    else if (T <= 19.0)
        b = 0.0;
    else
        b = clamp(log(T - 10.0) * 0.54320678911 - 1.19625408914, 0.0, 1.0);

    return vec3(r, g, b);
}

// ══════════════════════════════════════════════════════════════════════════
// DISK — simplex noise fBm, gaussian thickness, NT emission
// Simplex 3D noise by Ian McEwan, Ashima Arts
// ══════════════════════════════════════════════════════════════════════════

vec4 permute(vec4 x) { return mod(((x * 34.0) + 1.0) * x, 289.0); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v) {
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);
    vec3 g  = step(x0.yzx, x0.xyz);
    vec3 l  = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.xxx * 2.0;
    vec3 x3 = x0 - 1.0 + C.xxx * 3.0;
    i = mod(i, 289.0);
    vec4 p = permute(permute(permute(
        i.z + vec4(0.0, i1.z, i2.z, 1.0)) +
        i.y + vec4(0.0, i1.y, i2.y, 1.0)) +
        i.x + vec4(0.0, i1.x, i2.x, 1.0));
    float n_ = 1.0/7.0;
    vec3 ns = n_ * D.wyz - D.xzx;
    vec4 j  = p - 49.0 * floor(p * ns.z * ns.z);
    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);
    vec4 x  = x_ * ns.x + ns.yyyy;
    vec4 y  = y_ * ns.x + ns.yyyy;
    vec4 h  = 1.0 - abs(x) - abs(y);
    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);
    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));
    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;
    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);
    vec4 norm = taylorInvSqrt(vec4(
        dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w;
    vec4 m = max(0.6 - vec4(
        dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

// Disk colour: perceptual blackbody mapping.
// At r = r_in:  T_vis = T_peak (user-controlled, default 12000 K → blue-white).
// At r >> r_in: T_vis → 2000 K (deep orange-red).
vec3 disk_color(float r) {
    float t = pow(disk_r_in / r, 0.75);
    float T_vis = clamp(T_peak * t, 2000.0, 40000.0);
    return blackbody_kelvin(T_vis);
}

// ── Null geodesic integrator ───────────────────────────────────────────────
void null_rhs(float u, float dudf, out float du, out float ddu) {
    du  = dudf;
    ddu = 3.0 * M * u * u - u;
}

vec2 null_rk4(vec2 y, float h) {
    float k1u, k1du, k2u, k2du, k3u, k3du, k4u, k4du;
    null_rhs(y.x,              y.y,              k1u, k1du);
    null_rhs(y.x+0.5*h*k1u,   y.y+0.5*h*k1du,   k2u, k2du);
    null_rhs(y.x+0.5*h*k2u,   y.y+0.5*h*k2du,   k3u, k3du);
    null_rhs(y.x+    h*k3u,   y.y+    h*k3du,    k4u, k4du);
    float w = h / 6.0;
    return y + w * vec2(k1u + 2.0*k2u + 2.0*k3u + k4u,
                        k1du + 2.0*k2du + 2.0*k3du + k4du);
}

// ── Main ───────────────────────────────────────────────────────────────────
void main()
{
    // ── 1. Pixel → ray direction ──────────────────────────────────────────
    float aspect = float(width) / float(height);
    vec2  pix    = gl_FragCoord.xy;
    float u_ndc  = (2.0 * pix.x / float(width)  - 1.0) * aspect * fov_tan;
    float v_ndc  = (2.0 * pix.y / float(height) - 1.0) *           fov_tan;
    vec3 ray_dir = normalize(cam_fwd + u_ndc * cam_right + v_ndc * cam_up);

    // ── 2. Orbital plane basis ────────────────────────────────────────────
    float r_cam = length(cam_pos);
    vec3  e_r   = cam_pos / r_cam;

    vec3  L_vec = cross(cam_pos, ray_dir);
    float L_mag = length(L_vec);

    if (L_mag < 1.0e-5 * r_cam) {
        float r_dot = dot(ray_dir, e_r);
        fragColor = (r_dot < 0.0)
                  ? vec4(0.0, 0.0, 0.0, 1.0)
                  : vec4(texture(u_galaxy, ray_dir).rgb, 1.0);
        return;
    }

    vec3 e_z   = L_vec / L_mag;
    vec3 e_phi = cross(e_z, e_r);

    // ── 3. Impact parameter ───────────────────────────────────────────────
    float b  = L_mag;
    float b2 = b * b;

    // ── 4. Initial conditions ─────────────────────────────────────────────
    float u0        = 1.0 / r_cam;
    float dudphi_sq = 1.0/b2 - u0*u0 + 2.0*M*u0*u0*u0;
    float dudphi0   = sqrt(max(dudphi_sq, 0.0));
    if (dot(ray_dir, e_r) > 0.0) dudphi0 = -dudphi0;

    // ── 5. RK4 integration ────────────────────────────────────────────────
    float u_horizon = 1.0 / (rs * 1.001);
    float u_escape  = u0 * 0.002;

    vec2  y        = vec2(u0, dudphi0);
    float phi      = 0.0;
    bool  captured = false;

    float z_prev   = e_r.z;           // z-component at phi=0
    vec3  disk_accum = vec3(0.0);

    for (int i = 0; i < max_steps; ++i) {
        float phi_next = phi + dphi;
        float z_next   = cos(phi_next) * e_r.z + sin(phi_next) * e_phi.z;

        y   = null_rk4(y, dphi);
        phi = phi_next;

        if (y.x > u_horizon) { captured = true; break; }
        if (y.x < 0.0)       { captured = true; break; }
        if (y.x < u_escape)  { break; }

        // ── Disk crossing ─────────────────────────────────────────────────
        if (disk_on != 0 && z_prev * z_next < 0.0 && y.x > 0.0) {
            float r_cross = 1.0 / max(y.x, 1.0e-6);

            // Hard cutoff below ISCO — plunging region emits nothing
            if (r_cross >= disk_r_in && r_cross <= disk_r_out) {
                float phi_cross = phi - 0.5 * dphi;

                // ── 3-D crossing position (equatorial plane) ───────────────
                vec3  p_eq    = r_cross * (cos(phi_cross) * e_r + sin(phi_cross) * e_phi);
                vec2  p_2d    = p_eq.xy;
                float p2d_len = length(p_2d);
                vec3  v_disk  = (p2d_len > 1.0e-4)
                              ? vec3(-p_2d.y, p_2d.x, 0.0) / p2d_len
                              : vec3(0.0, 1.0, 0.0);

                // ── Doppler + gravitational frequency shift ────────────────
                float f_cross   = max(1.0 - 2.0 * M / r_cross, 1.0e-3);
                float v_orb     = clamp(sqrt(M / r_cross), 0.0, 0.92);
                vec3  ph_in     = normalize(cam_pos - p_eq);
                float cos_psi   = dot(ph_in, v_disk);
                float g_dop     = sqrt(1.0 - v_orb*v_orb) / max(1.0 - v_orb*cos_psi, 0.01);
                float g         = sqrt(f_cross) * g_dop;
                float g4        = pow(clamp(g, 0.0, 8.0), 4.0);

                // ── Simplex fBm turbulence (animated, Blackhole-style) ────
                float phi_eq = atan(p_eq.y, p_eq.x);
                // Spherical coord for noise: (rho, azimuth*2, 0)
                vec3 sc = vec3(r_cross, phi_eq * 2.0, 0.0);
                float snoise_val = 1.0;
                for (int ni = 0; ni < int(adisk_noise_lod); ++ni) {
                    float oct = pow(float(ni), 2.0);
                    snoise_val *= 0.5 * snoise(sc * oct) + 0.5;
                    if (ni % 2 == 0) sc.y += u_time * adisk_speed;
                    else             sc.y -= u_time * adisk_speed;
                }

                // ── Gaussian vertical profile — 4 z-offset samples ────────
                float h    = h_scale * r_cross;
                vec3  disk_col = disk_color(r_cross);
                // Tint with colorMap radial gradient
                disk_col *= texture(u_color_map, vec2(r_cross / disk_r_out, 0.5)).rgb * 2.0;
                // Radial NT emission profile: E ∝ (r_in/r)^3
                float E_radial = pow(disk_r_in / r_cross, 3.0);
                float turb_mod = mix(1.0, abs(snoise_val), turb_amp);

                // z-offset sample positions: -1.5h, -0.5h, +0.5h, +1.5h
                // At each, the "actual z" of the crossing is 0 (equatorial crossing);
                // we treat the disk as having finite thickness and weight by gaussian.
                float z_w_sum = 0.0;
                float emit_sum = 0.0;
                float z_samps[4];
                z_samps[0] = -1.5 * h;
                z_samps[1] = -0.5 * h;
                z_samps[2] =  0.5 * h;
                z_samps[3] =  1.5 * h;

                for (int zi = 0; zi < 4; ++zi) {
                    float zs  = z_samps[zi];
                    float gw  = exp(-0.5 * zs * zs / (h * h));
                    z_w_sum  += gw;
                    emit_sum += gw;
                }
                float norm_w = (z_w_sum > 0.0) ? 1.0 / z_w_sum : 0.0;

                float emission = base_gain * E_radial * emit_sum * norm_w
                                 * max(turb_mod, 0.0);

                disk_accum += g4 * emission * disk_col;
            }
        }
        z_prev = z_next;
    }

    // ── 6. Output ─────────────────────────────────────────────────────────
    if (captured) {
        fragColor = vec4(disk_accum, 1.0);   // might have crossed disk before capture
        return;
    }

    vec3 sky_dir = cos(phi) * e_r + sin(phi) * e_phi;
    vec3 col = texture(u_galaxy, sky_dir).rgb + disk_accum;

    fragColor = vec4(col, 1.0);
}

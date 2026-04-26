#version 410 core
// post.frag — final composite: HDR + pyramid bloom, ACES tonemap, sRGB gamma.

out vec4 fragColor;

uniform sampler2D u_hdr_tex;    // full-res RGBA16F from geodesic pass
uniform sampler2D u_bloom_tex;  // pyramid bloom result (up[0], half-res)
uniform vec2      u_vp_size;    // output viewport size
uniform float     u_exposure;   // exposure bias in stops
uniform float     u_bloom_int;  // bloom blend weight
uniform int       u_tonemap_on; // 1 = ACES filmic, 0 = Reinhard

vec3 aces(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec2 uv   = gl_FragCoord.xy / u_vp_size;
    float ev  = pow(2.0, u_exposure);
    vec3  hdr  = texture(u_hdr_tex,   uv).rgb * ev;
    vec3  bloom = texture(u_bloom_tex, uv).rgb * ev;

    vec3 col = hdr + bloom * u_bloom_int;

    if (u_tonemap_on != 0)
        col = aces(col);
    else
        col = col / (col + 1.0);

    col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));
    fragColor = vec4(col, 1.0);
}

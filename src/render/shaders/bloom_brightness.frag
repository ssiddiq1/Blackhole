#version 410 core
// Bright-pass filter: passes only pixels above luminance threshold.
out vec4 fragColor;

uniform sampler2D u_src_tex;
uniform vec2      u_vp_size;
uniform float     u_bloom_thr;

const vec3 kLum = vec3(0.2126, 0.7152, 0.0722);

void main() {
    vec2 uv = gl_FragCoord.xy / u_vp_size;
    vec4 c  = texture(u_src_tex, uv);
    float lum  = dot(kLum, c.rgb);
    float knee = max(lum - u_bloom_thr, 0.0) / max(lum, 1.0e-4);
    fragColor = vec4(c.rgb * knee, 1.0);
}

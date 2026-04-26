#version 410 core
// 2×2 box filter downsample — halves resolution each pass.
out vec4 fragColor;

uniform sampler2D u_src_tex;
uniform vec2      u_vp_size;  // destination (output) size

void main() {
    vec2 uv = gl_FragCoord.xy / u_vp_size;
    vec2 ts = 0.5 / u_vp_size;
    fragColor = 0.25 * (
        texture(u_src_tex, uv + vec2(-ts.x, -ts.y)) +
        texture(u_src_tex, uv + vec2( ts.x, -ts.y)) +
        texture(u_src_tex, uv + vec2(-ts.x,  ts.y)) +
        texture(u_src_tex, uv + vec2( ts.x,  ts.y)));
    fragColor.a = 1.0;
}

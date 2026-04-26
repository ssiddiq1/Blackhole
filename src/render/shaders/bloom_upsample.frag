#version 410 core
// Box-filter upsample of the lower-level blur, then add the detail from the
// matching downsample level to reconstruct the bloom pyramid.
out vec4 fragColor;

uniform sampler2D u_blur_tex;    // lower-res accumulated blur
uniform sampler2D u_detail_tex;  // same-level downsample (adds sharpness)
uniform vec2      u_vp_size;     // output size

void main() {
    vec2 uv = gl_FragCoord.xy / u_vp_size;
    vec2 ts = 0.5 / u_vp_size;
    vec4 up = 0.25 * (
        texture(u_blur_tex, uv + vec2(-ts.x, -ts.y)) +
        texture(u_blur_tex, uv + vec2( ts.x, -ts.y)) +
        texture(u_blur_tex, uv + vec2(-ts.x,  ts.y)) +
        texture(u_blur_tex, uv + vec2( ts.x,  ts.y)));
    fragColor = up + texture(u_detail_tex, uv);
    fragColor.a = 1.0;
}

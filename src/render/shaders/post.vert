#version 410 core
// post.vert — fullscreen triangle for all post-processing passes.
void main()
{
    // Generates a triangle that covers the entire clip-space quad.
    vec2 pos[3];
    pos[0] = vec2(-1.0, -1.0);
    pos[1] = vec2( 3.0, -1.0);
    pos[2] = vec2(-1.0,  3.0);
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}

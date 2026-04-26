#version 410 core
// screen.vert — Fullscreen triangle with no vertex buffer.
//
// Three vertices at NDC positions (-1,-1), (3,-1), (-1, 3) form one triangle
// that covers the entire clip-space quad. No VBO or attribute data needed;
// position is derived purely from gl_VertexID.  Call with glDrawArrays(GL_TRIANGLES, 0, 3).

void main()
{
    // Vertex positions for a single oversized triangle covering [-1,1]²
    const vec2 pos[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}

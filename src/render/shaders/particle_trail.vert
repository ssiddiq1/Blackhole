#version 410 core
// particle_trail.vert — Trail line strips for simulation particles.

layout(location = 0) in vec3  a_pos;
layout(location = 1) in float a_age;   // 0 = newest sample, 1 = oldest
layout(location = 2) in float a_type;  // 0=particle  1=dumbbell-p0  2=dumbbell-p1  3=broken

uniform mat4 u_mvp;

out float v_age;
out float v_type;

void main()
{
    gl_Position = u_mvp * vec4(a_pos, 1.0);
    v_age  = a_age;
    v_type = a_type;
}

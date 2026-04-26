#version 410 core
// particle_trail.frag — Colour and alpha for particle trail segments.

in float v_age;
in float v_type;
out vec4 fragColor;

void main()
{
    // Quadratic fade: head is opaque, tail is transparent
    float alpha = pow(1.0 - clamp(v_age, 0.0, 1.0), 2.0);

    vec3 col;
    int itype = int(v_type + 0.5);
    if      (itype == 0) col = vec3(1.00, 0.85, 0.40);  // test particle — warm gold
    else if (itype == 1) col = vec3(0.30, 0.80, 1.00);  // dumbbell p0  — cyan
    else if (itype == 2) col = vec3(1.00, 0.40, 0.85);  // dumbbell p1  — magenta
    else                 col = vec3(1.00, 0.30, 0.10);  // broken bond  — orange-red

    fragColor = vec4(col, alpha);
}

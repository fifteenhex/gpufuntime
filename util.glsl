mat4 rot_x(in float angle) {
    return mat4(1.0, 0, 0, 0,
                0, cos(angle), sin(angle), 0,
                0, sin(angle), cos(angle), 0,
                0, 0, 0, 1);
}

mat4 rot_y( in float angle ) {
    return mat4(cos(angle),0,sin(angle),0,
                0,1.0,0,0,
                -sin(angle),0,cos(angle),0,
                0,0,0,1);
}

mat4 rot_z( in float angle ) {
    return mat4(cos(angle),-sin(angle),0,0,
                sin(angle),cos(angle),0,0,
                0,0,1,0,
                0,0,0,1);
}

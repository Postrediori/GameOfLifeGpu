#version 330 core

in vec2 coord;
in vec2 texCoord;

out vec2 fragTexCoord;

uniform vec2 res;
uniform mat4 mvp;

vec2 adjust_proportions(vec2 coord, vec2 res) {
    vec2 p=coord;
    if (res.x > res.y) {
        p.x *= res.y / res.x;
    }
    else {
        p.y *= res.x / res.y;
    }
    return p;
}

void main(void) {
    fragTexCoord=texCoord;
    vec2 p=adjust_proportions(coord,res);
    gl_Position=mvp*vec4(p,0.,1.);
}

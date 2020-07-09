#version 330 core

in vec2 fragTexCoord;

out vec4 outFragCol;

uniform sampler2D tex;

const vec4 c1=vec4(0.97,0.98,1.,1.);
const vec4 c2=vec4(0.03,0.19,0.48,1.);

void main(void) {
    float c=texture(tex,fragTexCoord).r;
    outFragCol=vec4((c2+c*(c1-c2)).rgb,1.);
}

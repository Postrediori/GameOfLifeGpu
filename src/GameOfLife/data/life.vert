#version 330 core

in vec2 coord;
in vec2 texCoord;

out vec2 fragTexCoord;
out vec2 fragRes;

uniform vec2 res;
uniform mat4 mvp;

void main(void) {
    fragTexCoord=texCoord;
    fragRes=res;
    gl_Position=vec4(coord,0.,1.);
}

#version 330 core

in vec2 fragTexCoord;

out vec4 outFragCol;

uniform sampler2D tex;
uniform float time;

const float ActiveCell=1.;
const float InactiveCell=0.;

float rand(vec2 crd) {
   return fract(sin(dot(crd.xy,vec2(12.9898,78.233))) * 43758.5453);
}

void main(void) {
    vec2 uv=fragTexCoord;
    vec2 timeSeed=vec2(time,time)*0.002;
	
    float c=rand(uv+timeSeed) > .5 ? ActiveCell : InactiveCell;
    outFragCol=vec4(c,0.,0.,1.);
}

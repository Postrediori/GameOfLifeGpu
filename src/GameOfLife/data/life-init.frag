#version 330 core

in vec2 fragTexCoord;

out vec4 outFragCol;

uniform sampler2D tex;
uniform float time;
uniform int initType;

const int InitEmpty=0;
const int InitUniformRandom=1;
const int InitRadialRandom=2;

const float ActiveCell=1.;
const float InactiveCell=0.;

const float TimeScale=0.002;
const float RadialScale=100.;

float rand(vec2 crd) {
   return fract(sin(dot(crd.xy,vec2(12.9898,78.233))) * 43758.5453);
}

void main(void) {
    float c = 0.;
    
    if (initType==InitEmpty) {
        c=InactiveCell;
    }
    else {
        vec2 uv=fragTexCoord;
        vec2 timeSeed=vec2(time,time)*TimeScale;
        
        if (initType==InitUniformRandom) {
            c=rand(uv+timeSeed) > .5 ? ActiveCell : InactiveCell;
        }
        else if (initType==InitRadialRandom) {
            vec2 c0=uv-vec2(0.5);
            vec2 rt=vec2(trunc((length(c0)*RadialScale)));
        
            c=rand(rt+timeSeed) > .5 ? ActiveCell : InactiveCell;
        }
    }
    
    outFragCol=vec4(c,0.,0.,1.);
}

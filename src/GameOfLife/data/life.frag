#version 330 core

in vec2 fragTexCoord;
in vec2 fragRes;

out vec4 outFragCol;

uniform sampler2D tex;

const float ActiveCell=1.;
const float InactiveCell=0.;

int getNeighbours(vec2 uv) {
    const vec2 dnb[8]=vec2[](
        vec2(-1.,1.), vec2(0.,1.), vec2(1.,1.),
        vec2(-1.,0.), vec2(1.,0.),
        vec2(-1.,-1.), vec2(0.,-1.), vec2(1.,-1.)
    );
    vec2 dxy=vec2(1./fragRes.x,1./fragRes.y);

    int k=0;
    for (int i=0; i<8; i++) {
        float nb=texture(tex,uv+dxy*dnb[i]).r;
        if(nb==ActiveCell){
            k++;
        }
    }
    return k;
}

float calcActivity(float c, int nb) {
    float nc=c;
    if (nc==ActiveCell) {
        if (nb<2 || nb>3) {
            nc=InactiveCell;
        }
    }
    else if (nc==InactiveCell) {
        if (nb==3) {
            nc=ActiveCell;
        }
    }
    return nc;
}

void main(void) {
    int k=getNeighbours(fragTexCoord);

    float c=texture(tex,fragTexCoord).r;
    c=calcActivity(c,k);

    outFragCol=vec4(c,0.,0.,1.);
}

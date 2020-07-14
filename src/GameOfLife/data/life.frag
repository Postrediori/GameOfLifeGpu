#version 330 core

in vec2 fragTexCoord;

out vec4 outFragCol;

uniform sampler2D tex;

struct GameRules {
    int became;
    int stay;
};

uniform GameRules rules;

const float ActiveCell=1.;
const float InactiveCell=0.;

// Check that n-th bit of b is set
bool isBitSet(int b, int n) {
    return ((b / (1 << n)) % 2==1);
}

int getNeighbours(vec2 uv) {
    const int NBCount = 8;
    const vec2 dnb[NBCount]=vec2[](
        vec2(-1.,1.), vec2(0.,1.), vec2(1.,1.),
        vec2(-1.,0.), vec2(1.,0.),
        vec2(-1.,-1.), vec2(0.,-1.), vec2(1.,-1.)
    );
    ivec2 sz=textureSize(tex,0);
    vec2 dxy=vec2(1./float(sz.x),1./float(sz.y));

    int k=0;
    for (int i=0; i<NBCount; i++) {
        float nb=texture(tex,uv+dxy*dnb[i]).r;
        if(nb==ActiveCell){
            k++;
        }
    }
    return k;
}

bool ruleBegin(int nb) {
    return isBitSet(rules.became, nb);
}

bool ruleStay(int nb) {
    return isBitSet(rules.stay, nb);
}

float calcActivity(float c, int nb) {
    if (c==ActiveCell) {
        if (!ruleStay(nb)) {
            c=InactiveCell;
        }
    }
    else if (c==InactiveCell) {
        if (ruleBegin(nb)) {
            c=ActiveCell;
        }
    }
    return c;
}

void main(void) {
    int k=getNeighbours(fragTexCoord);

    float c=texture(tex,fragTexCoord).r;
    c=calcActivity(c,k);

    outFragCol=vec4(c,0.,0.,1.);
}

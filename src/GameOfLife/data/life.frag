#version 330 core

in vec2 fragTexCoord;

out vec4 outFragCol;

uniform sampler2D tex;

struct GameRules {
    int birth;
    int survive;
};

uniform GameRules rules;

uniform bool needSetActivity;
uniform vec2 activityPos;
const float ActivityRadius = 0.05;

const float PopulatedCell=1.;
const float UnpopulatedCell=0.;

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
        if(nb==PopulatedCell){
            k++;
        }
    }
    return k;
}

bool ruleBirth(int nb) {
    return isBitSet(rules.birth, nb);
}

bool ruleSurvive(int nb) {
    return isBitSet(rules.survive, nb);
}

float calcActivity(float c, int nb) {
    if (c==PopulatedCell) {
        if (!ruleSurvive(nb)) {
            c=UnpopulatedCell;
        }
    }
    else if (c==UnpopulatedCell) {
        if (ruleBirth(nb)) {
            c=PopulatedCell;
        }
    }
    return c;
}

void main(void) {
    vec2 uv = fragTexCoord;

    int k=getNeighbours(uv);

    float c=texture(tex,uv).r;
    c=calcActivity(c,k);

    if (needSetActivity && length(uv-activityPos)<ActivityRadius) {
        c=PopulatedCell;
    }

    outFragCol=vec4(c,0.,0.,1.);
}

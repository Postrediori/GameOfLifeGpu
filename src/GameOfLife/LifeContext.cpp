#include "stdafx.h"
#include "GraphicsUtils.h"
#include "PlanarTextureRenderer.h"
#include "FrameBufferWrapper.h"
#include "LifeContext.h"


static const std::string BufferRendererVertSrc =
    "#version 330 core\n"
    "in vec2 coord;"
    "in vec2 texCoord;"
    "out vec2 fragTexCoord;"
    "out vec2 fragRes;"
    "uniform vec2 res;"
    "uniform mat4 mvp;"
    "void main(void) {"
    "    fragTexCoord=texCoord;"
    "    fragRes=res;"
    "    gl_Position=vec4(coord,0.,1.);"
    "}";

static const std::string BufferRendererFragSrc =
    "#version 330 core\n"

    "in vec2 fragTexCoord;"
    "in vec2 fragRes;"

    "out vec4 outFragCol;"

    "uniform sampler2D tex;"

    "const float ActiveCell=1.;"
    "const float InactiveCell=0.;"

    "int getNeighbours(vec2 uv) {"
    "    const vec2 dnb[8]=vec2[]("
    "        vec2(-1.,1.), vec2(0.,1.), vec2(1.,1.),"
    "        vec2(-1.,0.), vec2(1.,0.),"
    "        vec2(-1.,-1.), vec2(0.,-1.), vec2(1.,-1.)"
    "    );"
    "    vec2 dxy=vec2(1./fragRes.x,1./fragRes.y);"

    "    int k=0;"
    "    for (int i=0; i<8; i++) {"
    "        float nb=texture(tex,uv+dxy*dnb[i]).r;"
    "        if(nb==ActiveCell){"
    "            k++;"
    "        }"
    "    }"
    "    return k;"
    "}"

    "float calcActivity(float c, int nb) {"
    "    float nc=c;"
    "    if (nc==ActiveCell) {"
    "        if (nb<2 || nb>3) {"
    "            nc=InactiveCell;"
    "        }"
    "    }"
    "    else if (nc==InactiveCell) {"
    "        if (nb==3) {"
    "            nc=ActiveCell;"
    "        }"
    "    }"
    "    return nc;"
    "}"

    "void main(void) {"
    "    int k=getNeighbours(fragTexCoord);"

    "    float c=texture(tex,fragTexCoord).r;"
    "    c=calcActivity(c,k);"

    "    outFragCol=vec4(c,0.,0.,1.);"
    "}";

static const std::string InitialDataVertSrc = BufferRendererVertSrc;
static const std::string InitialDataFragSrc =
    "#version 330 core\n"
    "in vec2 fragTexCoord;"
    "out vec4 outFragCol;"
    "uniform sampler2D tex;"
    "uniform float time;"
    "const float ActiveCell=1.;"
    "const float InactiveCell=0.;"
    "float rand(vec2 crd) {"
    "   return fract(sin(dot(crd.xy,vec2(12.9898,78.233))) * 43758.5453);"
    "}"
    "void main(void) {"
    "    float c=rand(fragTexCoord+vec2(time,time)*0.002) > .5 ? ActiveCell : InactiveCell;"
    "    outFragCol=vec4(c,0.,0.,1.);"
    "}";


static const std::string ScreenRendererVertSrc =
    "#version 330 core\n"
    "in vec2 coord;"
    "in vec2 texCoord;"
    "out vec2 fragTexCoord;"
    "uniform vec2 res;"
    "uniform mat4 mvp;"
    "vec2 adjust_proportions(vec2 coord, vec2 res) {"
    "    vec2 p=coord;"
    "    if (res.x > res.y) {"
    "        p.x *= res.y / res.x;"
    "    } else {"
    "        p.y *= res.x / res.y;"
    "    }"
    "    return p;"
    "}"
    "void main(void) {"
    "    fragTexCoord=texCoord;"
    "    vec2 p=adjust_proportions(coord,res);"
    "    gl_Position=mvp*vec4(p,0.,1.);"
    "}";

static const std::string ScreenRendererFragSrc =
    "#version 330 core\n"
    "in vec2 fragTexCoord;"
    "out vec4 outFragCol;"
    "uniform sampler2D tex;"
    "const vec4 c1=vec4(0.97, 0.98, 1., 1.);"
    "const vec4 c2=vec4(0.03, 0.19, 0.48, 1.);"
    "void main(void) {"
    "    float c=texture(tex,fragTexCoord).r;"
    "    outFragCol=vec4((c2+c*(c1-c2)).rgb,1.);"
    "}";


static GLuint InitTexture(GLenum format, GLsizei size) {
    GLuint tex = 0;

    glGenTextures(1, &tex); LOGOPENGLERROR();
    if (!tex) {
        LOGE << "Failed to init texture";
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, tex); LOGOPENGLERROR();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        size, size,
        0, format, GL_UNSIGNED_BYTE, nullptr); LOGOPENGLERROR();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); LOGOPENGLERROR();

    return tex;
}

LifeContext::~LifeContext() {
    Release();
}

bool LifeContext::InitTextures() {
    srcTexture = InitTexture(GL_RED, static_cast<GLsizei>(textureSize));
    if (!srcTexture) {
        return false;
    }

    dstTexture = InitTexture(GL_RED, static_cast<GLsizei>(textureSize));
    if (!dstTexture) {
        return false;
    }

    return true;
}

bool LifeContext::Init(int width, int height, int texSize) {
    LOGI << "OpenGL Renderer : " << glGetString(GL_RENDERER);
    LOGI << "OpenGL Vendor : " << glGetString(GL_VENDOR);
    LOGI << "OpenGL Version : " << glGetString(GL_VERSION);
    LOGI << "GLSL Version : " << glGetString(GL_SHADING_LANGUAGE_VERSION);

    textureSize = texSize;

    // Init textures
    if (!InitTextures()) {
        LOGE << "Failed to init textures";
        return false;
    }

    // Init initial texture renderer
    if (!bufferRenderer.Init(BufferRendererVertSrc, BufferRendererFragSrc)) {
        LOGE << "Failed to init texture renderer for frame buffer";
        return false;
    }

    bufferRenderer.SetTexture(srcTexture);
    bufferRenderer.Resize(textureSize, textureSize);

    // Init framebuffer
    if (!frameBuffer.Init()) {
        LOGE << "Failed to init framebuffer";
        return false;
    }

    frameBuffer.SetTexColorBuffer(dstTexture);

    // Init processed texture renderer
    if (!screenRenderer.Init(ScreenRendererVertSrc, ScreenRendererFragSrc)) {
        LOGE << "Failed to init processed texture renderer";
        return false;
    }
    
    screenRenderer.SetTexture(dstTexture);
    screenRenderer.Resize(width, height);

    // Initial texture renderer
    if (!InitWithRandomData(glfwGetTime())) {
        LOGE << "Unable to create initial buffer texture renderer";
        return false;
    }

    // Setup OpenGL flags
    glClearColor(0.0, 0.0, 0.0, 1.0); LOGOPENGLERROR();
    glClearDepth(1.0); LOGOPENGLERROR();

    return true;
}

bool LifeContext::InitWithRandomData(double seed) {
    PlanarTextureRenderer initialBufferWriter;
    if (!initialBufferWriter.Init(InitialDataVertSrc, InitialDataFragSrc)) {
        return false;
    }
    initialBufferWriter.SetTexture(dstTexture);
    initialBufferWriter.Resize(textureSize, textureSize);
    initialBufferWriter.SetTime(seed);

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.GetFrameBuffer()); LOGOPENGLERROR();
    initialBufferWriter.Render();
    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();

    SwapTextures();
}

void LifeContext::Reshape(int width, int height) {
    screenRenderer.Resize(width, height);
}

void LifeContext::Release() {
    glDeleteTextures(1, &srcTexture);
    glDeleteTextures(1, &dstTexture);
}

void LifeContext::Update() {
    SwapTextures();
}

void LifeContext::SwapTextures() {
    GLuint temp = 0;

    // Swap IDs
    temp = dstTexture;
    dstTexture = srcTexture;
    srcTexture = temp;

    // Swap IDs in the renderer objects
    bufferRenderer.SetTexture(srcTexture);
    frameBuffer.SetTexColorBuffer(dstTexture);

    screenRenderer.SetTexture(dstTexture);
}

void LifeContext::Display() {
    glClear(GL_COLOR_BUFFER_BIT); LOGOPENGLERROR();

    // Render to fixed-size framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.GetFrameBuffer()); LOGOPENGLERROR();

    bufferRenderer.Render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();

    // Render to screen
    screenRenderer.Render();
}

void LifeContext::Reshape(GLFWwindow* window, int width, int height) {
    LifeContext* context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    context->Reshape(width, height);
}

void LifeContext::Keyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    LifeContext* context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    static bool gFullscreen = false;

    static int gSavedXPos = 0, gSavedYPos = 0;
    static int gSavedWidth = 0, gSavedHeight = 0;

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;

        case GLFW_KEY_F1:
            gFullscreen = !gFullscreen;
            if (gFullscreen) {
                glfwGetWindowPos(window, &gSavedXPos, &gSavedYPos);
                glfwGetWindowSize(window, &gSavedWidth, &gSavedHeight);

                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0,
                    mode->width, mode->height, mode->refreshRate);
            }
            else {
                glfwSetWindowMonitor(window, nullptr, gSavedXPos, gSavedYPos,
                    gSavedWidth, gSavedHeight, GLFW_DONT_CARE);
            }
            break;

        case GLFW_KEY_SPACE:
            context->InitWithRandomData(glfwGetTime());
            break;
        }
    }
}

void LifeContext::Mouse(GLFWwindow* window, int button, int action, int /*mods*/) {
    LifeContext* context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_2) {
            context->InitWithRandomData(glfwGetTime());
        }
    }
}


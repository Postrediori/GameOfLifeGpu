#include "stdafx.h"
#include "GraphicsUtils.h"
#include "PlanarTextureRenderer.h"
#include "FrameBufferWrapper.h"
#include "LifeContext.h"


static const double UiWidth = 250.0;

static const std::string BufferRendererVert = "data/life.vert";
static const std::string BufferRendererFrag = "data/life.frag";

static const std::string InitialDataVert = BufferRendererVert;
static const std::string InitialDataFrag = "data/life-init.frag";

static const std::string ScreenRendererVert = "data/screen-plane.vert";
static const std::string ScreenRendererFrag = "data/screen-plane.frag";


static const struct {
    double xmin, xmax, ymin, ymax;
} ScreenArea = { -1.0, 1.0, -1.0, 1.0 };

static const std::vector<std::tuple<std::string, int>> ModelSizes = {
    {"128", 128},
    {"256", 256},
    {"512", 512},
    {"1024", 1024}
};

static const std::vector<std::tuple<std::string, std::string, AutomatonRules>> AutomatonRules = {
    {"Game of Life", "B3/S23", {8, 12}},
    {"High Life", "B36/S23", {72, 12}},
    {"Assimilation", "B345/S4567", {56, 240}},
    {"Day and Night", "B3678/S34678", {456, 472}},
    //{"Amoeba", "B357/S1358", {168, 298}}, // <-- White noise
    {"Move", "B368/S245", {328, 52}},
    {"Pseudo Life", "B357/S238", {168, 268}},
    //{"Diamoeba", "B35678/S5678", {488, 480}}, // <-- Captures all texture
    //{"34", "B34/S34", {24, 24}}, // ---
    //{"Long Life", "B345/S5", {56, 32}}, // <-- White noise
    {"Stains", "B3678/S235678", {456, 492}},
    //{"Seeds", "B2/S", {4, 0}}, // <-- White noise
    {"Maze", "B3/S12345", {8, 62}},
    {"Coagulations", "B378/S235678", {392, 492}}, // <-- Captures all texture
    //{"Walled cities", "B45678/S2345", {496, 60}}, // <-- White noise
    //{"Gnarl", "B1/S1", {1, 1}}, // <-- White noise
    //{"Replicator", "B1357/S1357", {170, 170}}, // <-- White noise
    {"Mystery", "B3458/S05678", {312, 481}},
};

LifeContext::~LifeContext() {
    Release();
}

bool LifeContext::InitTextures(int newSize) {
    Release();

    textureSize = newSize;

    srcTexture = GraphicsUtils::InitTexture(GL_RED, (GLsizei)textureSize, GL_NEAREST, GL_REPEAT);
    if (!srcTexture) {
        return false;
    }

    dstTexture = GraphicsUtils::InitTexture(GL_RED, (GLsizei)textureSize, GL_NEAREST, GL_REPEAT);
    if (!dstTexture) {
        return false;
    }

    return true;
}

bool LifeContext::Init(int newWidth, int newHeight, int texSize) {
    LOGI << "OpenGL Renderer : " << glGetString(GL_RENDERER);
    LOGI << "OpenGL Vendor : " << glGetString(GL_VENDOR);
    LOGI << "OpenGL Version : " << glGetString(GL_VERSION);
    LOGI << "GLSL Version : " << glGetString(GL_SHADING_LANGUAGE_VERSION);

    // Setup of ImGui visual style
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 0.0f;

    // Init default rules
    currentRules = std::get<2>(AutomatonRules[0]);

    // Init textures
    if (!InitTextures(texSize)) {
        LOGE << "Failed to init textures";
        return false;
    }

    // Init initial texture renderer
    if (!bufferRenderer.Init(BufferRendererVert, BufferRendererFrag)) {
        LOGE << "Failed to init texture renderer for frame buffer";
        return false;
    }

    bufferRendererProgram = bufferRenderer.GetProgram();
    bufferRendererRulesBecame = glGetUniformLocation(bufferRendererProgram, "rules.became");
    bufferRendererRulesStay = glGetUniformLocation(bufferRendererProgram, "rules.stay");

    bufferRenderer.SetTexture(srcTexture);
    bufferRenderer.Resize(textureSize, textureSize);

    // Init framebuffer
    if (!frameBuffer.Init()) {
        LOGE << "Failed to init framebuffer";
        return false;
    }

    frameBuffer.SetTexColorBuffer(dstTexture);

    // Init processed texture renderer
    if (!screenRenderer.Init(ScreenRendererVert, ScreenRendererFrag)) {
        LOGE << "Failed to init processed texture renderer";
        return false;
    }
    
    screenRenderer.SetTexture(dstTexture);
    screenRenderer.Resize(width, height);

    // Initial texture renderer
    if (!initialBufferWriter.Init(InitialDataVert, InitialDataFrag)) {
        LOGE << "Unable to create initial buffer texture renderer";
        return false;
    }

    NeedDataInit();

    // Setup OpenGL flags
    glClearColor(0.0, 0.0, 0.0, 1.0); LOGOPENGLERROR();
    glClearDepth(1.0); LOGOPENGLERROR();

    Reshape(newWidth, newHeight);

    return true;
}

void LifeContext::InitWithRandomData() {
    generationCounter = 0;

    initialBufferWriter.SetTexture(dstTexture);
    initialBufferWriter.Resize(textureSize, textureSize);
    initialBufferWriter.SetTime(glfwGetTime());

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.GetFrameBuffer()); LOGOPENGLERROR();
    initialBufferWriter.Render(true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();
}

void LifeContext::SetModelSize(int newSize) {
    InitTextures(newSize);

    bufferRenderer.SetTexture(srcTexture);
    frameBuffer.SetTexColorBuffer(dstTexture);

    bufferRenderer.Resize(textureSize, textureSize);

    NeedDataInit();
}

void LifeContext::Reshape(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;

    double widthWithoutUi = width - UiWidth;
    double newScale = 2.0 / (double)(widthWithoutUi);
    double newXMin = ScreenArea.xmin - UiWidth * newScale;
    glm::mat4 screenMvp = glm::ortho(newXMin, ScreenArea.xmax, ScreenArea.ymin, ScreenArea.ymax);

    screenRenderer.Resize(widthWithoutUi, height);
    screenRenderer.SetMvp(screenMvp);
}

void LifeContext::Release() {
    if (srcTexture) {
        glDeleteTextures(1, &srcTexture);
        srcTexture = 0;
    }
    if (dstTexture) {
        glDeleteTextures(1, &dstTexture);
        srcTexture = 0;
    }
}

void LifeContext::Update() {
    static int gensCounter = 0;

    static double lastTime = 0.0;
    double currentTime = glfwGetTime();

    // Update FPS counter every second
    if (currentTime - lastTime > 1.0) {
        gensPerSec = gensCounter;
        fps = ImGui::GetIO().Framerate;
        lastTime = currentTime;
        gensCounter = 0;
    }

    if (needDataInit) {
        InitWithRandomData();
        needDataInit = false;
    }

    // Move to the next generation
    SwapTextures();
    gensCounter++;
    generationCounter++;
}

void LifeContext::SwapTextures() {
    // Swap IDs
    GLuint temp = dstTexture;
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

    glUseProgram(bufferRendererProgram);
    glUniform1i(bufferRendererRulesBecame, currentRules.became);
    glUniform1i(bufferRendererRulesStay, currentRules.stay);

    bufferRenderer.Render(true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();

    // Render to screen
    glViewport(0, 0, width, height);
    screenRenderer.Render(false);

    DisplayUi();
}

void LifeContext::DisplayUi() {
    ImVec2 uiSize = ImVec2(UiWidth, height);

    ImGui::SetNextWindowPos(ImVec2(0.0, 0.0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(uiSize, ImGuiCond_Always);

    ImGui::Begin("Game of Life", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::Text("Game of Life");

    ImGui::Separator();

    int gModelSize = textureSize;
    ImGui::Text("Model size:");
    for (auto it = ModelSizes.begin(); it != ModelSizes.end(); it++) {
        const auto& s = *it;
        if (ImGui::RadioButton(std::get<0>(s).c_str(), &gModelSize, std::get<1>(s))) {
            SetModelSize(gModelSize);
        }
        if (it != std::prev(ModelSizes.end())) {
            ImGui::SameLine();
        }
    }

    ImGui::Separator();

    ImGui::Text("Cellular Automaton Rules:");

    static int selectedRule = 0;
    for (int i = 0; i < AutomatonRules.size(); i++) {
        const auto& r = AutomatonRules[i];
        if (ImGui::Selectable(std::get<0>(r).c_str(), selectedRule == i)) {
            selectedRule = i;

            currentRules = std::get<2>(r);
            NeedDataInit();
        }
        ImGui::SameLine(150); ImGui::Text(std::get<1>(r).c_str());
    }

    ImGui::Separator();

    ImGui::Text("User Guide:");
    ImGui::BulletText("F1 to on/off fullscreen mode.");
    ImGui::BulletText("RMB/Space to Clear model.");

    ImGui::Separator();

    ImGui::Text("Generation no.: %d", generationCounter);
    ImGui::Text("Gens/sec: %.1f", gensPerSec);

    ImGui::Separator();

    ImGui::Text("FPS Counter : %.1f", fps);

    ImGui::End();
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
            context->NeedDataInit();
            break;

        case GLFW_KEY_1:
            context->SetModelSize(std::get<1>(ModelSizes[0]));
            break;

        case GLFW_KEY_2:
            context->SetModelSize(std::get<1>(ModelSizes[1]));
            break;

        case GLFW_KEY_3:
            context->SetModelSize(std::get<1>(ModelSizes[2]));
            break;

        case GLFW_KEY_4:
            context->SetModelSize(std::get<1>(ModelSizes[3]));
            break;
        }
    }
}

void LifeContext::Mouse(GLFWwindow* window, int button, int action, int /*mods*/) {
    LifeContext* context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_2) {
            context->NeedDataInit();
        }
    }
}


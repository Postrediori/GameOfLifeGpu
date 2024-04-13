#include "stdafx.h"
#include "GraphicsLogger.h"
#include "GraphicsResource.h"
#include "Shader.h"
#include "PlanarTextureRenderer.h"
#include "CellularAutomata.h"
#include "ResourceFinder.h"
#include "LifeContext.h"

constexpr double UiWidth = 250.0;

const std::filesystem::path BufferRendererVert = "life.vert";
const std::filesystem::path BufferRendererFrag = "life.frag";

const std::filesystem::path InitialDataVert = BufferRendererVert;
const std::filesystem::path InitialDataFrag = "life-init.frag";

const std::filesystem::path ScreenRendererVert = "screen-plane.vert";
const std::filesystem::path ScreenRendererFrag = "screen-plane.frag";

const HMM_Vec4 ScreenArea = { -1.0, 1.0, -1.0, 1.0 };

const std::vector<std::tuple<std::string, int>> ModelSizes = {
    {"128", 128},
    {"256", 256},
    {"512", 512},
    {"1024", 1024}
};

using CellularAutomataRules = std::tuple<std::string, std::string, CellularAutomata::AutomatonRules>;
static const std::vector<CellularAutomataRules> AutomatonRules = {
    {"Game of Life", "B3/S23", {1, 8, 12}},
    {"High Life", "B36/S23", {2, 72, 12}},
    {"Assimilation", "B345/S4567", {3, 56, 240}},
    {"Day and Night", "B3678/S34678", {4, 456, 472}},
    //{"Amoeba", "B357/S1358", {5, 168, 298}}, // <-- White noise
    {"Move", "B368/S245", {6, 328, 52}},
    {"Pseudo Life", "B357/S238", {7, 168, 268}},
    //{"Diamoeba", "B35678/S5678", {8, 488, 480}}, // <-- Captures all texture
    //{"34", "B34/S34", {9, 24, 24}}, // ---
    //{"Long Life", "B345/S5", {10, 56, 32}}, // <-- White noise
    {"Stains", "B3678/S235678", {11, 456, 492}},
    //{"Seeds", "B2/S", {12, 4, 0}}, // <-- White noise
    {"Maze", "B3/S12345", {13, 8, 62}},
    {"Coagulations", "B378/S235678", {14, 392, 492}}, // <-- Captures all texture
    //{"Walled cities", "B45678/S2345", {15, 496, 60}}, // <-- White noise
    //{"Gnarl", "B1/S1", {16, 1, 1}}, // <-- White noise
    //{"Replicator", "B1357/S1357", {17, 170, 170}}, // <-- White noise
    {"Mystery", "B3458/S05678", {18, 312, 481}},
    {"Anneal", "B4678/S35678", {19, 464, 488}},
};

static const std::vector<std::tuple<std::string, CellularAutomata::FirstGenerationType>> InitialRandomTypes = {
    {"Empty / Manual draw", CellularAutomata::FirstGenerationType::Empty},
    {"Radial Random", CellularAutomata::FirstGenerationType::RadialRandom},
    {"Uniform Random", CellularAutomata::FirstGenerationType::UniformRandom},
};

auto InitTexture(GLuint tex, GLenum format, GLsizei size, GLenum filter, GLenum wrap) -> void {
    glBindTexture(GL_TEXTURE_2D, tex); LOGOPENGLERROR();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        size, size,
        0, format, GL_UNSIGNED_BYTE, nullptr); LOGOPENGLERROR();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap); LOGOPENGLERROR();
}

LifeContext::LifeContext(GLFWwindow* w)
    : window(w) {
}

bool LifeContext::InitTextures(int newSize) {
    ReleaseTextures();

    textureSize = newSize;

    glGenTextures(1, currentGenerationTex.put()); LOGOPENGLERROR();
    if (!currentGenerationTex) {
        LOGE << "Failed to init texture";
        return false;
    }

    glGenTextures(1, nextGenerationTex.put()); LOGOPENGLERROR();
    if (!nextGenerationTex) {
        LOGE << "Failed to init texture";
        return false;
    }

    InitTexture(static_cast<GLuint>(currentGenerationTex), GL_RED, (GLsizei)textureSize, GL_NEAREST, GL_REPEAT);
    InitTexture(static_cast<GLuint>(nextGenerationTex), GL_RED, (GLsizei)textureSize, GL_NEAREST, GL_REPEAT);

    return true;
}

bool LifeContext::Init(int /*argc*/, const char* argv[], int newWidth, int newHeight, int texSize) {
    std::filesystem::path moduleDataDir;
    if (!Utils::ResourceFinder::GetDataDirectory(argv[0], moduleDataDir)) {
        LOGE << "Unable to find data directory";
        return false;
    }

    LOGI << "OpenGL Renderer : " << glGetString(GL_RENDERER);
    LOGI << "OpenGL Vendor : " << glGetString(GL_VENDOR);
    LOGI << "OpenGL Version : " << glGetString(GL_VERSION);
    LOGI << "GLSL Version : " << glGetString(GL_SHADING_LANGUAGE_VERSION);

    LOGI << "GLFW Version : " << GLFW_VERSION_MAJOR << "." << GLFW_VERSION_MINOR << "." << GLFW_VERSION_REVISION;
    LOGI << "ImGui Version : " << IMGUI_VERSION << " (" << IMGUI_VERSION_NUM << ")";
    
    glfwSetWindowUserPointer(this->window, static_cast<void *>(this));

    // Init default rules
    currentRules = std::get<2>(AutomatonRules[0]);
    firstGenerationType = CellularAutomata::FirstGenerationType::RadialRandom;

    // CA simulation
    auto bufferRendererVert = (moduleDataDir / BufferRendererVert).string();
    auto bufferRendererFrag = (moduleDataDir / BufferRendererFrag).string();
    automataProgram.reset(Shader::CreateProgram(bufferRendererVert, bufferRendererFrag));
    if (!automataProgram) {
        LOGE << "Failed to init shader program for cellular automata";
        return false;
    }

    uRulesBirth = glGetUniformLocation(static_cast<GLuint>(automataProgram), "rules.birth"); LOGOPENGLERROR();
    uRulesSurvive = glGetUniformLocation(static_cast<GLuint>(automataProgram), "rules.survive"); LOGOPENGLERROR();

    uNeedSetActivity = glGetUniformLocation(static_cast<GLuint>(automataProgram), "needSetActivity"); LOGOPENGLERROR();
    uActivityPos = glGetUniformLocation(static_cast<GLuint>(automataProgram), "activityPos"); LOGOPENGLERROR();

    if (!automataRenderer.Init(static_cast<GLuint>(automataProgram))) {
        LOGE << "Failed to init texture renderer for frame buffer";
        return false;
    }

    // CA init data
    auto initialDataVert = (moduleDataDir / InitialDataVert).string();
    auto initialDataFrag = (moduleDataDir / InitialDataFrag).string();
    automataInitProgram.reset(Shader::CreateProgram(initialDataVert, initialDataFrag));
    if (!automataInitProgram) {
        LOGE << "Failed to init shader program for initial state of cellular automata";
        return false;
    }

    uInitType = glGetUniformLocation(static_cast<GLuint>(automataInitProgram), "initType"); LOGOPENGLERROR();

    if (!automataInitialRenderer.Init(static_cast<GLuint>(automataInitProgram))) {
        LOGE << "Failed to setup initial cellular automata data creator";
        return false;
    }

    // Screen renderer
    auto screenRendererVert = (moduleDataDir / ScreenRendererVert).string();
    auto screenRendererFrag = (moduleDataDir / ScreenRendererFrag).string();
    screenProgram.reset(Shader::CreateProgram(screenRendererVert, screenRendererFrag));
    if (!screenProgram) {
        LOGE << "Failed to init shader program for screen rendering";
        return false;
    }

    if (!screenRenderer.Init(static_cast<GLuint>(screenProgram))) {
        LOGE << "Failed to init processed texture renderer";
        return false;
    }

    screenRenderer.Resize(width, height);

    // Init framebuffer
    glGenFramebuffers(1, frameBuffer.put()); LOGOPENGLERROR();
    if (!frameBuffer) {
        LOGE << "Failed to init framebuffer";
        return false;
    }

    // Setup OpenGL flags
    glClearColor(0.0, 0.0, 0.0, 1.0); LOGOPENGLERROR();
    glClearDepth(1.0); LOGOPENGLERROR();

    Reshape(newWidth, newHeight);

    // Init textures and create first generation
    SetModelSize(texSize);

    RegisterCallbacks();

    return true;
}

void LifeContext::InitFirstGeneration() {
    generationCounter = 0;

    automataInitialRenderer.SetTime(glfwGetTime());

    glUseProgram(static_cast<GLuint>(automataInitProgram)); LOGOPENGLERROR();
    glUniform1i(uInitType, static_cast<int>(firstGenerationType)); LOGOPENGLERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(frameBuffer)); LOGOPENGLERROR();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<GLuint>(nextGenerationTex), 0); LOGOPENGLERROR();

    automataInitialRenderer.AdjustViewport();
    automataInitialRenderer.Render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();
}

void LifeContext::SetModelSize(int newSize) {
    InitTextures(newSize);

    automataRenderer.Resize(textureSize, textureSize);
    automataInitialRenderer.Resize(textureSize, textureSize);

    NeedDataInit();
}

void LifeContext::SetAutomatonRules(CellularAutomata::AutomatonRules newRules) {
    this->currentRules = newRules;
    this->NeedDataInit();
}

void LifeContext::SetFirstGenerationType(CellularAutomata::FirstGenerationType newType) {
    this->firstGenerationType = newType;
    this->NeedDataInit();
}

void LifeContext::RegisterCallbacks() {
    glfwSetWindowUserPointer(window, static_cast<void*>(this));

    glfwSetKeyCallback(window, LifeContext::KeyboardCallback);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

    glfwSetWindowSizeCallback(window, LifeContext::ReshapeCallback);

    glfwSetMouseButtonCallback(window, LifeContext::MouseCallback);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
}

void LifeContext::Reshape(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;

    double widthWithoutUi = width - UiWidth;
    double newScale = 2.0 / (double)(widthWithoutUi);
    double newXMin = ScreenArea.X - UiWidth * newScale;
    HMM_Mat4 screenMvp = HMM_Orthographic_RH_NO(newXMin, ScreenArea.Y, ScreenArea.Z, ScreenArea.W, 1.f, -1.f);

    screenRenderer.Resize(widthWithoutUi, height);
    screenRenderer.SetMvp(screenMvp);
}

void LifeContext::ReleaseTextures() {
    currentGenerationTex.reset();
    nextGenerationTex.reset();
}

void LifeContext::Update() {
    double currentTime = glfwGetTime();

    // Update FPS counter every second
    if (currentTime - lastFpsTime > 1.0) {
        gensPerSec = gensCounter;
        fps = ImGui::GetIO().Framerate;
        lastFpsTime = currentTime;
        gensCounter = 0;
    }

    int status = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    if (status == GLFW_PRESS) {
        double x = 0.0, y = 0.0;
        glfwGetCursorPos(window, &x, &y);

        this->MouseDown(x, y);
    }

    if (needDataInit) {
        InitFirstGeneration();
        needDataInit = false;
    }
    else {
        CalcNextGeneration();
    }

    // Move to the next generation
    SwapGenerations();
    gensCounter++;
}

void LifeContext::CalcNextGeneration() {
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(frameBuffer)); LOGOPENGLERROR();

    glUseProgram(static_cast<GLuint>(automataProgram)); LOGOPENGLERROR();
    glUniform1i(uRulesBirth, currentRules.birth); LOGOPENGLERROR();
    glUniform1i(uRulesSurvive, currentRules.survive); LOGOPENGLERROR();

    if (needSetActivity) {
        glUniform1i(uNeedSetActivity, 1); LOGOPENGLERROR();
        glUniform2fv(uActivityPos, 1, (const GLfloat*)(&activityPos)); LOGOPENGLERROR();
        needSetActivity = false;
    }
    else {
        glUniform1i(uNeedSetActivity, 0); LOGOPENGLERROR();
    }

    automataRenderer.AdjustViewport();
    automataRenderer.Render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();

    generationCounter++;
}

void LifeContext::SwapGenerations() {
    // Swap IDs
    nextGenerationTex.swap(currentGenerationTex);

    // Swap IDs in the renderer objects
    automataRenderer.SetTexture(static_cast<GLuint>(currentGenerationTex));

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(frameBuffer)); LOGOPENGLERROR();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<GLuint>(nextGenerationTex), 0); LOGOPENGLERROR();
    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();

    screenRenderer.SetTexture(static_cast<GLuint>(nextGenerationTex));
}

void LifeContext::Display() {
    glClear(GL_COLOR_BUFFER_BIT); LOGOPENGLERROR();

    // Render to screen
    glViewport(0, 0, width, height); LOGOPENGLERROR();
    screenRenderer.Render();

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

    ImGui::Text("Model size:");

    int iModelSize = textureSize;
    bool first = true;
    for (const auto& s : ModelSizes) {
        std::string name;
        int size;
        std::tie(name, size) = s;

        if (first) {
            first = false;
        }
        else {
            ImGui::SameLine();
        }
        if (ImGui::RadioButton(name.c_str(), &iModelSize, size)) {
            SetModelSize(iModelSize);
        }
    }

    ImGui::Separator();

    ImGui::Text("Cellular Automaton Rules:");

    for (const auto& r : AutomatonRules) {
        std::string name, rulesDesc;
        CellularAutomata::AutomatonRules rules;
        std::tie(name, rulesDesc, rules) = r;

        if (ImGui::Selectable(name.c_str(), currentRules.id == rules.id)) {
            SetAutomatonRules(rules);
        }
        ImGui::SameLine(150); ImGui::Text("%s", rulesDesc.c_str());
    }

    ImGui::Separator();

    ImGui::Text("Initial state:");

    int iRandomType = static_cast<int>(this->firstGenerationType);
    for (const auto& s : InitialRandomTypes) {
        std::string name;
        CellularAutomata::FirstGenerationType id;
        std::tie(name, id) = s;

        if (ImGui::RadioButton(name.c_str(), &iRandomType, static_cast<int>(id))) {
            this->SetFirstGenerationType(id);
        }
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

void LifeContext::MouseDown(int x, int y) {
    if (firstGenerationType != CellularAutomata::FirstGenerationType::Empty) {
        return;
    }

    int newX = x - UiWidth;

    int w = this->width - UiWidth;
    int h = this->height;

    int cx = 0, cy = 0;
    if (w > h) {
        cx = newX - (w - h) / 2;
        cy = y;
    }
    else {
        cx = newX;
        cy = y - (h - w) / 2;
    }

    int size = (w > h) ? h : w;

    if (!(cx<0 || cy<0 || cx>size || cy>size)) {
        float s = (float)cx / (float)size;
        float t = 1.f - (float)cy / (float)size;

        this->SetActivity(HMM_Vec2{ s, t });

        LOGI << "Set Activity at [" << s << "," << t << "]";
    }
}

void LifeContext::SetActivity(HMM_Vec2 pos) {
    needSetActivity = true;
    activityPos = pos;
}

void LifeContext::Keyboard(int key, int /*scancode*/, int action, int /*mods*/) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;

        case GLFW_KEY_F1:
            isFullscreen = !isFullscreen;

            if (isFullscreen) {
                glfwGetWindowPos(window, &savedWindowPos.X, &savedWindowPos.Y);
                glfwGetWindowSize(window, &savedWindowPos.Width, &savedWindowPos.Height);

                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0,
                    mode->width, mode->height, mode->refreshRate);
            }
            else {
                glfwSetWindowMonitor(window, nullptr, savedWindowPos.X, savedWindowPos.Y,
                    savedWindowPos.Width, savedWindowPos.Height, GLFW_DONT_CARE);
            }
            break;

        case GLFW_KEY_SPACE:
            NeedDataInit();
            break;

        case GLFW_KEY_1:
            SetModelSize(std::get<1>(ModelSizes[0]));
            break;

        case GLFW_KEY_2:
            SetModelSize(std::get<1>(ModelSizes[1]));
            break;

        case GLFW_KEY_3:
            SetModelSize(std::get<1>(ModelSizes[2]));
            break;

        case GLFW_KEY_4:
            SetModelSize(std::get<1>(ModelSizes[3]));
            break;
        }
    }
}

void LifeContext::Mouse(int button, int action, int /*mods*/) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_2) {
            NeedDataInit();
        }
    }
}

void LifeContext::ReshapeCallback(GLFWwindow* window, int width, int height) {
    auto context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    context->Reshape(width, height);
}

void LifeContext::KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    context->Keyboard(key, scancode, action, mods);
}

void LifeContext::MouseCallback(GLFWwindow* window, int button, int action, int mods) {
    auto context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    context->Mouse(button, action, mods);
}

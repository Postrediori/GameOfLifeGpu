#include "stdafx.h"
#include "GraphicsLogger.h"
#include "GraphicsResource.h"
#include "Shader.h"
#include "PlanarTextureRenderer.h"
#include "LifeContext.h"

static const double UiWidth = 250.0;

static const std::string BufferRendererVert = "data/life.vert";
static const std::string BufferRendererFrag = "data/life.frag";

static const std::string InitialDataVert = BufferRendererVert;
static const std::string InitialDataFrag = "data/life-init.frag";

static const std::string ScreenRendererVert = "data/screen-plane.vert";
static const std::string ScreenRendererFrag = "data/screen-plane.frag";

static const hmm_vec4 ScreenArea = { -1.0, 1.0, -1.0, 1.0 };

static const std::vector<std::tuple<std::string, int>> ModelSizes = {
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

LifeContext::~LifeContext() {
    Release();
}

bool LifeContext::InitTextures(int newSize) {
    ReleaseTextures();

    textureSize = newSize;

    GLuint tex{ 0 };

    glGenTextures(1, &tex); LOGOPENGLERROR();
    if (tex == 0) {
        LOGE << "Failed to init texture";
        return false;
    }
    currentGenerationTex.reset(tex);

    glGenTextures(1, &tex); LOGOPENGLERROR();
    if (tex == 0) {
        LOGE << "Failed to init texture";
        return false;
    }
    nextGenerationTex.reset(tex);

    InitTexture(currentGenerationTex.get(), GL_RED, (GLsizei)textureSize, GL_NEAREST, GL_REPEAT);
    InitTexture(nextGenerationTex.get(), GL_RED, (GLsizei)textureSize, GL_NEAREST, GL_REPEAT);

    return true;
}

bool LifeContext::Init(int newWidth, int newHeight, int texSize) {
    LOGI << "OpenGL Renderer : " << glGetString(GL_RENDERER);
    LOGI << "OpenGL Vendor : " << glGetString(GL_VENDOR);
    LOGI << "OpenGL Version : " << glGetString(GL_VERSION);
    LOGI << "GLSL Version : " << glGetString(GL_SHADING_LANGUAGE_VERSION);

    glfwSetWindowUserPointer(this->window, static_cast<void *>(this));

    // Setup of ImGui visual style
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 0.0f;

    // Init default rules
    currentRules = std::get<2>(AutomatonRules[0]);
    firstGenerationType = CellularAutomata::FirstGenerationType::RadialRandom;

    // CA simulation
    if ((automataProgram = Shader::CreateProgram(BufferRendererVert, BufferRendererFrag)) == 0) {
        LOGE << "Failed to init shader program for cellular automata";
        return false;
    }

    uRulesBirth = glGetUniformLocation(automataProgram, "rules.birth"); LOGOPENGLERROR();
    uRulesSurvive = glGetUniformLocation(automataProgram, "rules.survive"); LOGOPENGLERROR();

    uNeedSetActivity = glGetUniformLocation(automataProgram, "needSetActivity"); LOGOPENGLERROR();
    uActivityPos = glGetUniformLocation(automataProgram, "activityPos"); LOGOPENGLERROR();

    if (!automataRenderer.Init(automataProgram)) {
        LOGE << "Failed to init texture renderer for frame buffer";
        return false;
    }

    // CA init data
    if ((automataInitProgram = Shader::CreateProgram(InitialDataVert, InitialDataFrag)) == 0) {
        LOGE << "Failed to init shader program for initial state of cellular automata";
        return false;
    }

    uInitType = glGetUniformLocation(automataInitProgram, "initType"); LOGOPENGLERROR();

    if (!automataInitialRenderer.Init(automataInitProgram)) {
        LOGE << "Failed to setup initial cellular automata data creator";
        return false;
    }

    // Screen renderer
    if ((screenProgram = Shader::CreateProgram(ScreenRendererVert, ScreenRendererFrag)) == 0) {
        LOGE << "Failed to init shader program for screen rendering";
        return false;
    }

    if (!screenRenderer.Init(screenProgram)) {
        LOGE << "Failed to init processed texture renderer";
        return false;
    }

    screenRenderer.Resize(width, height);

    // Init framebuffer
    GLuint fb{ 0 };
    glGenFramebuffers(1, &fb); LOGOPENGLERROR();
    if (fb == 0) {
        LOGE << "Failed to init framebuffer";
        return false;
    }
    frameBuffer.reset(fb);

    // Setup OpenGL flags
    glClearColor(0.0, 0.0, 0.0, 1.0); LOGOPENGLERROR();
    glClearDepth(1.0); LOGOPENGLERROR();

    Reshape(newWidth, newHeight);

    // Init textures and create first generation
    SetModelSize(texSize);

    return true;
}

void LifeContext::InitFirstGeneration() {
    generationCounter = 0;

    automataInitialRenderer.SetTime(glfwGetTime());

    glUseProgram(automataInitProgram); LOGOPENGLERROR();
    glUniform1i(uInitType, static_cast<int>(firstGenerationType)); LOGOPENGLERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.get()); LOGOPENGLERROR();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nextGenerationTex.get(), 0); LOGOPENGLERROR();

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

void LifeContext::Reshape(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;

    double widthWithoutUi = width - UiWidth;
    double newScale = 2.0 / (double)(widthWithoutUi);
    double newXMin = ScreenArea.X - UiWidth * newScale;
    hmm_mat4 screenMvp = HMM_Orthographic(newXMin, ScreenArea.Y, ScreenArea.Z, ScreenArea.W, 1.f, -1.f);

    screenRenderer.Resize(widthWithoutUi, height);
    screenRenderer.SetMvp(screenMvp);
}

void LifeContext::Release() {
    ReleaseTextures();

    if (automataProgram) {
        glDeleteProgram(automataProgram); LOGOPENGLERROR();
        automataProgram = 0;
    }
    if (automataInitProgram) {
        glDeleteProgram(automataInitProgram); LOGOPENGLERROR();
        automataInitProgram = 0;
    }
}

void LifeContext::ReleaseTextures() {
    currentGenerationTex.reset();
    nextGenerationTex.reset();
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
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.get()); LOGOPENGLERROR();

    glUseProgram(automataProgram); LOGOPENGLERROR();
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
    automataRenderer.SetTexture(currentGenerationTex.get());

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.get()); LOGOPENGLERROR();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nextGenerationTex.get(), 0); LOGOPENGLERROR();
    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();

    screenRenderer.SetTexture(nextGenerationTex.get());
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
    size_t k = 0;
    for (const auto& s : ModelSizes) {
        std::string name;
        int size;
        std::tie(name, size) = s;

        if (ImGui::RadioButton(name.c_str(), &iModelSize, size)) {
            SetModelSize(iModelSize);
        }
        if (k++ < (ModelSizes.size() - 1)) {
            ImGui::SameLine();
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

        this->SetActivity(hmm_vec2{ s, t });

        LOGI << "Set Activity at [" << s << "," << t << "]";
    }
}

void LifeContext::SetActivity(hmm_vec2 pos) {
    needSetActivity = true;
    activityPos = pos;
}

void LifeContext::Reshape(GLFWwindow* window, int width, int height) {
    LifeContext* context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    context->Reshape(width, height);
}

void LifeContext::Keyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    LifeContext* context = static_cast<LifeContext *>(glfwGetWindowUserPointer(window));
    assert(context);

    static struct {
        bool fullscreen{ false };
        int savedXPos{ 0 }, savedYPos{ 0 };
        int savedWidth{ 0 }, savedHeight{ 0 };

        void ToggleFullscreen(GLFWwindow* window) {
            fullscreen = !fullscreen;

            if (fullscreen) {
                glfwGetWindowPos(window, &savedXPos, &savedYPos);
                glfwGetWindowSize(window, &savedWidth, &savedHeight);

                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0,
                    mode->width, mode->height, mode->refreshRate);
            }
            else {
                glfwSetWindowMonitor(window, nullptr, savedXPos, savedYPos,
                    savedWidth, savedHeight, GLFW_DONT_CARE);
            }

        }
    } gScreenState;

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;

        case GLFW_KEY_F1:
            gScreenState.ToggleFullscreen(window);
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

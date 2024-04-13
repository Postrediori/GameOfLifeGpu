#pragma once

struct WindowDimensions {
    int X, Y;
    int Width, Height;
};

class LifeContext {
public:
    LifeContext(GLFWwindow* w);

    bool Init(int argc, const char* argv[], int width, int height, int textureSize);

    void Display();
    void Update();

    void NeedDataInit() { needDataInit = true; }

    void MouseDown(int x, int y);
    void SetActivity(HMM_Vec2 pos);

    void Reshape(int width, int height);
    void Keyboard(int key, int /*scancode*/, int action, int /*mods*/);
    void Mouse(int button, int action, int /*mods*/);

    static void ReshapeCallback(GLFWwindow* window, int width, int height);
    static void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseCallback(GLFWwindow* window, int button, int action, int mods);

private:
    bool InitTextures(int newSize);
    void ReleaseTextures();

    void InitFirstGeneration();
    void CalcNextGeneration();
    void SwapGenerations();

    void DisplayUi();

    void SetModelSize(int newSize);
    void SetAutomatonRules(CellularAutomata::AutomatonRules newRules);
    void SetFirstGenerationType(CellularAutomata::FirstGenerationType newType);

    void RegisterCallbacks();

private:
    GLFWwindow* window = nullptr;

    bool isFullscreen = false;
    int width = 0, height = 0;
    WindowDimensions savedWindowPos = { 0, 0, 0, 0 };

    int generationCounter = 0;
    float fps = 0.0;
    float gensPerSec = 0.0;

    int textureSize = 0;

    GraphicsUtils::unique_texture currentGenerationTex;
    GraphicsUtils::unique_texture nextGenerationTex;

    GraphicsUtils::unique_program automataProgram;
    GLint uRulesBirth = -1, uRulesSurvive = -1;
    GLint uNeedSetActivity = -1, uActivityPos = -1;
    PlanarTextureRenderer automataRenderer;

    GraphicsUtils::unique_program automataInitProgram;
    GLint uInitType = -1;
    PlanarTextureRenderer automataInitialRenderer;

    GraphicsUtils::unique_program screenProgram;
    PlanarTextureRenderer screenRenderer;

    GraphicsUtils::unique_framebuffer frameBuffer;

    bool needDataInit = false;

    CellularAutomata::AutomatonRules currentRules{ 0 };
    CellularAutomata::FirstGenerationType firstGenerationType{
        CellularAutomata::FirstGenerationType::Empty };

    bool needSetActivity = false;
    HMM_Vec2 activityPos = { 0 };

    int gensCounter = 0;
    double lastFpsTime = 0.0;
};

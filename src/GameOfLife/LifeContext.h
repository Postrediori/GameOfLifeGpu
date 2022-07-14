#pragma once

class LifeContext {
public:
    LifeContext(GLFWwindow* w);

    bool Init(int width, int height, int textureSize);

    void Reshape(int width, int height);
    void Display();
    void Update();

    void NeedDataInit() { needDataInit = true; }

    void MouseDown(int x, int y);
    void SetActivity(hmm_vec2 pos);

    static void Reshape(GLFWwindow* window, int width, int height);
    static void Keyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/);
    static void Mouse(GLFWwindow* window, int button, int action, int /*mods*/);

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

private:
    GLFWwindow* window = nullptr;

    int width = 0, height = 0;

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
    hmm_vec2 activityPos = { 0 };
};

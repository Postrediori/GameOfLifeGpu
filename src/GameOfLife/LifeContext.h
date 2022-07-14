#pragma once

namespace CellularAutomata {
    struct AutomatonRules {
        int id;
        int birth;
        int survive;
    };

    enum class FirstGenerationType {
        Empty = 0,
        UniformRandom = 1,
        RadialRandom = 2,
    };
}

class LifeContext {
public:
    LifeContext(GLFWwindow* w);
    ~LifeContext();

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
    void Release();

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

    GLuint currentGenerationTex = 0;
    GLuint nextGenerationTex = 0;

    GLuint automataProgram = 0;
    GLint uRulesBirth = -1, uRulesSurvive = -1;
    GLint uNeedSetActivity = -1, uActivityPos = -1;
    PlanarTextureRenderer automataRenderer;

    GLuint automataInitProgram = 0;
    GLint uInitType = -1;
    PlanarTextureRenderer automataInitialRenderer;

    GLuint screenProgram = 0;
    PlanarTextureRenderer screenRenderer;

    GraphicsUtils::unique_framebuffer frameBuffer;

    bool needDataInit = false;

    CellularAutomata::AutomatonRules currentRules;
    CellularAutomata::FirstGenerationType firstGenerationType;

    bool needSetActivity = false;
    hmm_vec2 activityPos;
};

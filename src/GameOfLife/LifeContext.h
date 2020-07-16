#pragma once

namespace CellularAutomata {
    struct AutomatonRules {
        int id;
        int became;
        int stay;
    };

    enum class InitialRandomType {
        EmptyInit = 0,
        UniformRandom = 1,
        RadialRandom = 2,
    };
}

class LifeContext {
public:
    ~LifeContext();

    bool Init(int width, int height, int textureSize);

    void Reshape(int width, int height);
    void Display();
    void Update();

    void NeedDataInit() { needDataInit = true; }

    void MouseDown(int x, int y);
    void SetActivity(const glm::vec2& pos);

    static void Reshape(GLFWwindow* window, int width, int height);
    static void Keyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/);
    static void Mouse(GLFWwindow* window, int button, int action, int /*mods*/);

private:
    bool InitTextures(int newSize);
    void ReleaseTextures();
    void Release();

    void SwapTextures();

    void DisplayUi();

    void InitWithRandomData();
    void SetModelSize(int newSize);
    void SetAutomatonRules(CellularAutomata::AutomatonRules newRules);
    void SetInitialRandomType(CellularAutomata::InitialRandomType newType);

private:
    int width = 0, height = 0;

    int generationCounter = 0;
    float fps = 0.0;
    float gensPerSec = 0.0;

    int textureSize = 0;

    GLuint srcTexture = 0;
    GLuint dstTexture = 0;

    GLuint automataProgram = 0;
    GLuint automataInitProgram = 0;

    GLint autotamaRulesBecameUniform = -1;
    GLint autotamaRulesStayUniform = -1;
    GLint autotamaInitTypeUniform = -1;

    PlanarTextureRenderer automataRenderer;
    PlanarTextureRenderer automataInitialRenderer;
    FrameBufferWrapper frameBuffer;

    GLuint screenProgram = 0;
    PlanarTextureRenderer screenRenderer;

    bool needDataInit = false;

    CellularAutomata::AutomatonRules currentRules;
    CellularAutomata::InitialRandomType initialRandomType;

    bool needSetActivity = false;
    glm::vec2 activityPos;
};

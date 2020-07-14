#pragma once

struct AutomatonRules {
    int became;
    int stay;
};

class LifeContext {
public:
    ~LifeContext();

    bool Init(int width, int height, int textureSize);

    void Reshape(int width, int height);
    void Display();
    void Update();

    void NeedDataInit() { needDataInit = true; }

    static void Reshape(GLFWwindow* window, int width, int height);
    static void Keyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/);
    static void Mouse(GLFWwindow* window, int button, int action, int /*mods*/);

private:
    bool InitTextures(int newSize);
    void Release();

    void SwapTextures();

    void DisplayUi();

    void InitWithRandomData();
    void SetModelSize(int newSize);

private:
    int width = 0, height = 0;

    int generationCounter = 0;
    float fps = 0.0;
    float gensPerSec = 0.0;

    int textureSize = 0;

    GLuint srcTexture = 0;
    GLuint dstTexture = 0;

    GLuint bufferRendererProgram = 0;
    GLint bufferRendererRulesBecame = -1, bufferRendererRulesStay = -1;
    PlanarTextureRenderer bufferRenderer;
    PlanarTextureRenderer initialBufferWriter;
    FrameBufferWrapper frameBuffer;

    PlanarTextureRenderer screenRenderer;

    bool needDataInit = false;

    AutomatonRules currentRules;
};

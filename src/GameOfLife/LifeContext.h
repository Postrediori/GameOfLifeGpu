#pragma once

class LifeContext {
public:
    ~LifeContext();

    bool Init(int width, int height, int textureSize);
    bool InitWithRandomData(double seed);

    void Reshape(int width, int height);
    void Display();
    void Update();

    static void Reshape(GLFWwindow* window, int width, int height);
    static void Keyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/);
    static void Mouse(GLFWwindow* window, int button, int action, int /*mods*/);

private:
    bool InitTextures();
    void Release();

    void SwapTextures();

private:
    int textureSize = 0;

    GLuint srcTexture = 0;
    GLuint dstTexture = 0;

    PlanarTextureRenderer bufferRenderer;
    FrameBufferWrapper frameBuffer;

    PlanarTextureRenderer screenRenderer;
};

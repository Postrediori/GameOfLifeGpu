#pragma once

class FrameBufferWrapper {
public:
    ~FrameBufferWrapper();

    bool Init();
    void Release();

    void SetTexColorBuffer(GLuint tex);

    void Bind();
    void Unbind();

private:
    GLuint frameBuffer = 0;
    GLuint texColorBuffer = 0;
    bool isBound = false;
};

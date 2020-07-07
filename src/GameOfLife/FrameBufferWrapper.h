#pragma once

class FrameBufferWrapper {
public:
    ~FrameBufferWrapper();

    bool Init();
    void Release();

    void SetTexColorBuffer(GLuint tex);

    GLuint GetFrameBuffer() const { return frameBuffer; }

private:
    GLuint frameBuffer = 0;
    GLuint texColorBuffer = 0;
};

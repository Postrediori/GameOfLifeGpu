#pragma once

class PlanarTextureRenderer {
public:
    ~PlanarTextureRenderer();

    bool Init(const std::string& vertexFileName, const std::string& fragmentFileName);
    void Release();
    void Resize(int newWidth, int newHeight);
    void Render(bool adjustViewport);

    void SetTexture(GLuint t);
    void SetTime(double t);
    void SetMvp(glm::mat4 mvp);
    GLuint GetProgram() const { return program; }

private:
    int width = 0, height = 0;
    double time = 0.0;

    GLuint texture = 0;

    GLuint program = 0;
    GLint uRes = -1, uMvp = -1, uTex = -1, uTime = -1;

    GLuint vao = 0;
    GLuint vbo = 0, indVbo = 0;

    glm::mat4 mvp;
};

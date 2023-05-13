#pragma once

class PlanarTextureRenderer {
public:
    PlanarTextureRenderer() = default;

    bool Init(GLuint program);
    void Resize(int newWidth, int newHeight);
    void Render();

    void AdjustViewport();

    void SetTexture(GLuint t);
    void SetTime(double t);
    void SetMvp(HMM_Mat4 mvp);

private:
    int width = 0, height = 0;
    double time = 0.0;

    GLuint texture = 0;

    GLuint program = 0;
    GLint uRes = -1, uMvp = -1, uTex = -1, uTime = -1;

    GraphicsUtils::unique_vertex_array vao;
    GraphicsUtils::unique_buffer vbo, indVbo;

    HMM_Mat4 mvp;
};

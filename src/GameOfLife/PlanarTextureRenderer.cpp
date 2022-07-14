#include "stdafx.h"
#include "GraphicsLogger.h"
#include "Shader.h"
#include "PlanarTextureRenderer.h"

static const hmm_vec4 PlaneBounds = { -1.0f, 1.0f, -1.0f, 1.0f };

static const std::vector<float> PlaneVertices = {
    -1.0f, -1.0f, 0.f, 0.f,
    -1.0f, 1.0f, 0.f, 1.f,
    1.0f, -1.0f, 1.f, 0.f,
    1.0f, 1.0f, 1.f, 1.f,
};

static const std::vector<GLuint> PlaneIndices = {
    0, 1, 2,
    2, 1, 3,
};

PlanarTextureRenderer::~PlanarTextureRenderer() {
    Release();
}

bool PlanarTextureRenderer::Init(GLuint p) {
    program = p;

    uRes = glGetUniformLocation(program, "res"); LOGOPENGLERROR();
    uMvp = glGetUniformLocation(program, "mvp"); LOGOPENGLERROR();
    uTex = glGetUniformLocation(program, "tex"); LOGOPENGLERROR();
    uTime = glGetUniformLocation(program, "time"); LOGOPENGLERROR();

    mvp = HMM_Orthographic(PlaneBounds.X, PlaneBounds.Y, PlaneBounds.Z, PlaneBounds.W, 1.f, -1.f);

    // Init VAO
    glGenVertexArrays(1, &vao); LOGOPENGLERROR();
    if (!vao) {
        LOGE << "Failed to create VAO for planar texture";
        return false;
    }
    glBindVertexArray(vao); LOGOPENGLERROR();

    // Init VBO
    glGenBuffers(1, &vbo); LOGOPENGLERROR();
    if (!vbo) {
        LOGE << "Failed to create VBO for planar texture";
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo); LOGOPENGLERROR();
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * PlaneVertices.size(),
        PlaneVertices.data(), GL_STATIC_DRAW); LOGOPENGLERROR();

    // Init indices VBO
    glGenBuffers(1, &indVbo); LOGOPENGLERROR();
    if (!indVbo) {
        LOGE << "Failed to create indices VBO";
        return false;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indVbo); LOGOPENGLERROR();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * PlaneIndices.size(), PlaneIndices.data(),
        GL_STATIC_DRAW); LOGOPENGLERROR();

    // Setup VAO
    GLint aCoord = glGetAttribLocation(program, "coord"); LOGOPENGLERROR();
    GLint aTexCoord = glGetAttribLocation(program, "texCoord"); LOGOPENGLERROR();

    glEnableVertexAttribArray(aCoord); LOGOPENGLERROR();
    glVertexAttribPointer(aCoord, 2, GL_FLOAT, GL_FALSE,
        sizeof(GLfloat) * 4, (void *)(0)); LOGOPENGLERROR();

    glEnableVertexAttribArray(aTexCoord); LOGOPENGLERROR();
    glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE,
        sizeof(GLfloat) * 4, (void *)(sizeof(GLfloat) * 2)); LOGOPENGLERROR();

    glBindVertexArray(0); LOGOPENGLERROR();

    return true;
}

void PlanarTextureRenderer::SetTexture(GLuint t) {
    texture = t;
}

void PlanarTextureRenderer::SetTime(double t) {
    time = t;
}

void PlanarTextureRenderer::SetMvp(hmm_mat4 newMvp) {
    mvp = newMvp;
}

void PlanarTextureRenderer::Release() {
    glDeleteVertexArrays(1, &vao); LOGOPENGLERROR();
    vao = 0;

    glDeleteBuffers(1, &vbo); LOGOPENGLERROR();
    vbo = 0;
}

void PlanarTextureRenderer::Resize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
}

void PlanarTextureRenderer::AdjustViewport() {
    glViewport(0, 0, width, height); LOGOPENGLERROR();
}

void PlanarTextureRenderer::Render() {
    glUseProgram(program); LOGOPENGLERROR();
    glBindVertexArray(vao); LOGOPENGLERROR();

    glActiveTexture(GL_TEXTURE0); LOGOPENGLERROR();
    glBindTexture(GL_TEXTURE_2D, texture); LOGOPENGLERROR();

    glUniformMatrix4fv(uMvp, 1, GL_FALSE, (const GLfloat*)(&mvp)); LOGOPENGLERROR();
    glUniform2f(uRes, (GLfloat)width, (GLfloat)height); LOGOPENGLERROR();
    glUniform1i(uTex, 0); LOGOPENGLERROR();
    glUniform1f(uTime, time); LOGOPENGLERROR();

    glDrawElements(GL_TRIANGLES, (GLsizei)PlaneIndices.size(), GL_UNSIGNED_INT, nullptr); LOGOPENGLERROR();

    glBindVertexArray(0); LOGOPENGLERROR();
    glUseProgram(0); LOGOPENGLERROR();
}

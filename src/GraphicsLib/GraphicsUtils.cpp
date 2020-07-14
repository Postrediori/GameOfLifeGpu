#include "stdafx.h"
#include "GraphicsUtils.h"

static const std::unordered_map<GLenum, std::string> OpenGlErrors = {
    {GL_INVALID_ENUM, "GL_INVALID_ENUM"},
    {GL_INVALID_VALUE, "GL_INVALID_VALUE"},
    {GL_INVALID_OPERATION, "GL_INVALID_OPERATION"},
    {GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION"},
    {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"},
};

void GraphicsUtils::LogOpenGLError(const char* file, int line) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOGE << " OpenGL Error in file " << file << " line " << line << " : " << OpenGlErrors.at(err);
    }
}

GLuint GraphicsUtils::InitTexture(GLenum format, GLsizei size, GLenum filter, GLenum wrap) {
    GLuint tex = 0;

    glGenTextures(1, &tex); LOGOPENGLERROR();
    if (!tex) {
        LOGE << "Failed to init texture";
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, tex); LOGOPENGLERROR();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        size, size,
        0, format, GL_UNSIGNED_BYTE, nullptr); LOGOPENGLERROR();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap); LOGOPENGLERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap); LOGOPENGLERROR();

    return tex;
}

#include "stdafx.h"
#include "GraphicsLogger.h"

void GraphicsUtils::LogOpenGLError(const char* file, int line) {
    GLenum err = glGetError();
    auto const errStr = [err]() {
        switch (err) {
        case GL_INVALID_ENUM: return "INVALID_ENUM";
        case GL_INVALID_VALUE: return "INVALID_VALUE";
        case GL_INVALID_OPERATION: return "INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case GL_STACK_UNDERFLOW: return "STACK_UNDERFLOW";
        case GL_STACK_OVERFLOW: return "STACK_OVERFLOW";
        default: return "Unknown error";
        }
    }();

    if (err != GL_NO_ERROR) {
        LOGE << " OpenGL Error in file " << file << " line " << line << " : " << errStr;
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

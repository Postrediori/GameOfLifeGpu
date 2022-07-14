#include "stdafx.h"
#include "GraphicsLogger.h"
#include "FrameBufferWrapper.h"

FrameBufferWrapper::~FrameBufferWrapper() {
    Release();
}

bool FrameBufferWrapper::Init() {
    glGenFramebuffers(1, &frameBuffer); LOGOPENGLERROR();
    if (!frameBuffer) {
        LOGE << "Failed to create framebuffer";
        return false;
    }

    return true;
}

void FrameBufferWrapper::Release() {
    if (frameBuffer) {
        glDeleteFramebuffers(1, &frameBuffer); LOGOPENGLERROR();
        frameBuffer = 0;
    }
}

void FrameBufferWrapper::SetTexColorBuffer(GLuint tex) {
    texColorBuffer = tex;

    if (!isBound) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer); LOGOPENGLERROR();
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0); LOGOPENGLERROR();
    if (!isBound) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();
    }
}

void FrameBufferWrapper::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer); LOGOPENGLERROR();
    isBound = true;
}

void FrameBufferWrapper::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();
    isBound = false;
}

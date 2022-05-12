#include "stdafx.h"
#include "GraphicsUtils.h"
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

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer); LOGOPENGLERROR();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0); LOGOPENGLERROR();
    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();
}

void FrameBufferWrapper::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer); LOGOPENGLERROR();
}

void FrameBufferWrapper::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0); LOGOPENGLERROR();
}

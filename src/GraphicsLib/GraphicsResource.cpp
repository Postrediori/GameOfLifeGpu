#include "stdafx.h"
#include "GraphicsLogger.h"
#include "GraphicsResource.h"


namespace GraphicsUtils {

auto unique_texture::close() -> void {
    glDeleteTextures(1, &resourceId_); LOGOPENGLERROR();
}

auto unique_framebuffer::close() -> void {
    glDeleteFramebuffers(1, &resourceId_); LOGOPENGLERROR();
}

auto unique_program::close() -> void {
    glDeleteProgram(resourceId_); LOGOPENGLERROR();
}

auto unique_vertex_array::close() -> void {
    glDeleteVertexArrays(1, &resourceId_); LOGOPENGLERROR();
}

auto unique_buffer::close() -> void {
    glDeleteBuffers(1, &resourceId_); LOGOPENGLERROR();
}

} // namespace GraphicsUtils

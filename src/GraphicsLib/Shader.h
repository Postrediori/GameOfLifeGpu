#pragma once

namespace Shader {
    GLuint CreateProgram(const std::string& vertex_shader, const std::string& fragment_shader);
    GLuint CreateProgramFromSource(const std::string& vertex_shader, const std::string& fragment_shader);
}

#include "shader_support/glsl_shader.hpp"

#include "filesystem_support/file_locator.hpp"

#include <iostream>

namespace cg
{

GLSLShader::GLSLShader(const std::string &shader_str, GLenum shader_type)
{
    shader_type_str_ = shader_str;
    gl_shader_type_ = shader_type;
}

bool GLSLShader::create_from_source(const char *source)
{
    bool success = true;
    gl_shader_ = glCreateShader(gl_shader_type_);
    glShaderSource(gl_shader_, 1, &source, NULL);
    glCompileShader(gl_shader_);
    if(!check_compile_status(gl_shader_))
    {
        std::cout << shader_type_str_ << " shader compile failed.\n";
        log_compile_error(gl_shader_);
        std::cout << shader_type_str_ << " Shader Source = \n";
        std::cout << source << '\n';
        success = false;
    }
    return success;
}

std::string trim(const std::string &str)
{
    // [space]  =  32, 0x20
    // ~        = 126, 0x7E
    size_t         first_idx = 0;
    size_t         last_idx = str.length() - 1;
    constexpr char SPACE = ' ';
    constexpr char TILDA = '~';

    for(size_t i = 0; i < str.length(); ++i)
    {
        if(str[i] > SPACE && str[i] <= TILDA)
        {
            first_idx = i;
            break;
        }
    }

    for(size_t i = str.length() - 1; i > 0; --i)
    {
        if(str[i] == '}')
        {
            last_idx = i;
            break;
        }
    }

    if(first_idx >= last_idx) return str;

    return str.substr(first_idx, last_idx - first_idx + 1);
}

bool GLSLShader::create(const char *filename)
{
    FileContents file_contents;
    if(!read_shader_source(filename, file_contents)) exit(-1);

    std::string t_content_str(file_contents.data, file_contents.size);
    auto        trimmed_str = trim(t_content_str);

    bool success = create_from_source(trimmed_str.c_str());
    file_contents.destroy();
    return success;
}

GLuint GLSLShader::get() const { return gl_shader_; }

bool GLSLShader::check_compile_status(GLuint shader)
{
    int param = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
    return (param == GL_TRUE);
}

bool GLSLShader::read_shader_source(const char *filename, FileContents &file_contents)
{
    if(filename == 0)
    {
        std::cout << "NULL filename for shader...exiting\n";
        return false;
    }

    // Get path to file and file size
    auto file_info = locate_path_for_filename(filename);
    if(!file_info.found)
    {
        std::cout << "Could not find shader file " << filename
                  << ". Also not in parent directories\n";
        return false;
    }

    return load_file_contents(file_info.file_path, file_contents);
}

void GLSLShader::log_compile_error(GLuint shader)
{
    GLint   len = 0;
    GLsizei slen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if(len > 1)
    {
        GLchar *compilerlog = (GLchar *)new GLchar *[len];
        glGetShaderInfoLog(shader, len, &slen, compilerlog);
        std::cout << "compiler log:\n" << compilerlog << '\n';
        delete[] compilerlog;
    }
}

GLSLVertexShader::GLSLVertexShader() : GLSLShader("Vertex", GL_VERTEX_SHADER) {}

GLSLFragmentShader::GLSLFragmentShader() : GLSLShader("Fragment", GL_FRAGMENT_SHADER) {}

} // namespace cg

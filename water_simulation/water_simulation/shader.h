#ifndef SHADER_HEADER
#define SHADER_HEADER

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace graphics {

    class Shader {
    private:
        void checkCompileErrors(GLuint shader, std::string type);

    public:
        GLuint ID,
            model_mat_location,
            view_mat_location,
            proj_mat_location,
            color_location
            ;

        Shader(const char* vertex_shader_file, const char* fragment_shader_file);
    };

};

#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <LearnOpenGL/stb_image.h>
#include <algorithm>
#include <LearnOpenGL/glad.c>
#include "maths.h"
#include "rectangle.h"
#include "water_surface.h"
#include "plane.h"
#include "shader.h"
#include <iostream>
#include <vector>
#include <LearnOpenGL/camera.h>

#define STB_IMAGE_IMPLEMENTATION
#define M_PI 3.1415926

void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

const int SCR_WIDTH = 1000;
const int SCR_HEIGHT = 1000;

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    vec3 cam_pos(5, 5, 5);
    vec3 cam_targ(0, 0, -2);
    vec3 cam_up(0, 0, 1);

    mat4 view_mat = mat4::look_at(cam_pos, cam_targ, cam_up);
    mat4 inv_view_mat = view_mat.inverse();

    mat4 proj_mat = mat4::perspective_projection(60, 1, 0.1, 100);
    mat4 inv_proj_mat = proj_mat.inverse();

    graphics::Shader surface_shader("shaders/surface.vert", "shaders/surface.frag");
    graphics::Shader floor_shader("shaders/floor.vert", "shaders/floor.frag");
    graphics::Shader rectangle_shader("shaders/rectangle.vert", "shaders/rectangle.frag");

    object3d::rectangle rectangle(vec3(0, 0, 0), vec3(0, 0, 1));
    object3d::water_surface water_surface;
    object3d::plane plane;

    GLuint floor_texture = loadTexture("floor.jpg");
    GLuint sky_texture = loadTexture("sky2.jpg");

    glUseProgram(surface_shader.ID);
    GLuint floor_texture_location = glGetUniformLocation(surface_shader.ID, "floor_texture");
    glUniform1i(floor_texture_location, 0);

    GLuint sky_texture_location = glGetUniformLocation(surface_shader.ID, "sky_texture");
    glUniform1i(sky_texture_location, 1);
    glUseProgram(0);

    float previouse_time = glfwGetTime();
    bool is_drawing_continous = true;
    bool is_c_key_down = false;

    while (!glfwWindowShouldClose(window)) {

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        processInput(window);

        int c_key_state = glfwGetKey(window, GLFW_KEY_C);
        if (c_key_state == GLFW_PRESS && !is_c_key_down) {
            is_c_key_down = true;
            is_drawing_continous = !is_drawing_continous;
        }
        if(c_key_state == GLFW_RELEASE) {
            is_c_key_down = false;
        }

        float current_time = glfwGetTime();
        float elapsed_time = current_time - previouse_time;
        previouse_time = current_time;

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        proj_mat = mat4::perspective_projection(60, width / (float)height, 0.1, 100);
        inv_proj_mat = proj_mat.inverse();

        {
            static bool is_mouse_down = false;

            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            vec3 mouse_world = inv_view_mat * inv_proj_mat * vec3((2.0 * xpos) / width - 1.0, 1.0 - (2.0 * ypos) / height, 1.0);
            mouse_world.make_unit_length();
            vec3 mouse_intersection = cam_pos + (-cam_pos.z / mouse_world.z) * mouse_world;

            int mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (mouse_state == GLFW_PRESS && !is_mouse_down) {
                is_mouse_down = true;

                if (mouse_intersection.x > -3.0 &&
                    mouse_intersection.x < 3.0 &&
                    mouse_intersection.y > -3.0 &&
                    mouse_intersection.y < 3.0) {
                    int i = (mouse_intersection.x + 3.0) / 6.0 * water_surface.width;
                    int j = (mouse_intersection.y + 3.0) / 6.0 * water_surface.height;

                    if (i > 0 && j > 0 && i < water_surface.width - 1 && j < water_surface.height - 1) {
                        water_surface.u[i][j] =1.2;
                        water_surface.u[i - 1][j - 1] = 0.7;
                        water_surface.u[i - 1][j] =0.7;
                        water_surface.u[i - 1][j + 1] = 0.7;
                        water_surface.u[i + 1][j - 1] = 0.7;
                        water_surface.u[i + 1][j] = 0.7;
                        water_surface.u[i + 1][j + 1] = 0.7;
                        water_surface.u[i][j + 1] = 0.7;
                        water_surface.u[i][j - 1] = 0.5;
                    }
                }
            } else if (mouse_state == GLFW_RELEASE && is_mouse_down) {
                is_mouse_down = false;
            }
        }

        {
            glUseProgram(floor_shader.ID);

            // Set up uniforms for floor of water
            mat4 model_mat = mat4::translation(vec3(0, 0, -3)) * mat4::scale(vec3(3, 3, 1)) * mat4::rotation_z(M_PI / 2.0);
            glUniformMatrix4fv(floor_shader.view_mat_location, 1, GL_TRUE, view_mat.m);
            glUniformMatrix4fv(floor_shader.proj_mat_location, 1, GL_TRUE, proj_mat.m);
            glUniformMatrix4fv(floor_shader.model_mat_location, 1, GL_TRUE, model_mat.m);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floor_texture);

            // Draw floor of water
            glBindVertexArray(plane.vao);
            glDrawArrays(GL_TRIANGLES, 0, 18);
            glBindVertexArray(0);

            // Set up uniforms for side 1 of water
            model_mat = mat4::translation(vec3(0, -3, -1)) * mat4::scale(vec3(3, 1, 2)) * mat4::rotation_x(M_PI / 2.0);
            glUniformMatrix4fv(floor_shader.model_mat_location, 1, GL_TRUE, model_mat.m);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floor_texture);

            // Draw side 1 of water
            glBindVertexArray(plane.vao);
            glDrawArrays(GL_TRIANGLES, 0, 18);
            glBindVertexArray(0);

            // Set up uniforms for side 2 of water
            model_mat = mat4::translation(vec3(-3, 0, -1)) * mat4::scale(vec3(1, 3, 2)) * mat4::rotation_y(M_PI / 2.0);
            glUniformMatrix4fv(floor_shader.model_mat_location, 1, GL_TRUE, model_mat.m);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floor_texture);

            // Draw side 2 of water
            glBindVertexArray(plane.vao);
            glDrawArrays(GL_TRIANGLES, 0, 18);
            glBindVertexArray(0);

            glUseProgram(0);
        }

        water_surface.update(elapsed_time);

        if (!is_drawing_continous) {
            glUseProgram(rectangle_shader.ID);

            for (int i = 0; i < water_surface.width; i++) {
                for (int j = 0; j < water_surface.height; j++) {
                    //网格的间距有关
                    //1-是至关重要的，没有1-的话整个水平面就像是反过来了一样
                    float x = 3 - 6 * (1-(i / (float)water_surface.width));
                    float y = 3 - 6 * (1-(j / (float)water_surface.height));

                    vec3 position(x, y, 0);
                    vec3 scale(1.0, 1.0, water_surface.u[i][j]);

                    mat4 model_mat = mat4::translation(position) * mat4::scale(scale);

                    glUniform3f(rectangle_shader.color_location, 0.527, 0.843, 0.898);
                    glUniformMatrix4fv(rectangle_shader.model_mat_location, 1, GL_TRUE, model_mat.m);
                    glUniformMatrix4fv(rectangle_shader.view_mat_location, 1, GL_TRUE, view_mat.m);
                    glUniformMatrix4fv(rectangle_shader.proj_mat_location, 1, GL_TRUE, proj_mat.m);

                    glBindVertexArray(rectangle.vao);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    glBindVertexArray(0);
                }
            }

            glUseProgram(0);
        }

        if (is_drawing_continous) {
            glUseProgram(surface_shader.ID);

            mat4 model_mat = mat4::identity();
            glUniformMatrix4fv(surface_shader.model_mat_location, 1, GL_TRUE, model_mat.m);
            glUniformMatrix4fv(surface_shader.view_mat_location, 1, GL_TRUE, view_mat.m);
            glUniformMatrix4fv(surface_shader.proj_mat_location, 1, GL_TRUE, proj_mat.m);
            glUniform3f(surface_shader.color_location, 0.527, 0.843, 0.898);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floor_texture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, sky_texture);

            // Draw the water surface
            glBindVertexArray(water_surface.vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, water_surface.elements_vbo);
            glDrawElements(GL_TRIANGLES, (water_surface.N - 1) * (water_surface.N - 1) * 2 * 3, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glUseProgram(0);
        }

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

unsigned int loadTexture(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
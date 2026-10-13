//std=c23
#include <glad/glad.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string.h>
#include "TOOL_Error.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct {
    SDL_Window *w;
    SDL_GLContext gl_context;
    GLuint program;
    GLuint vao, vbo, ebo;
    GLuint texture;
} AppResources;

void AppResourcesDestroy(AppResources *r) {
    if (r->gl_context != nullptr) {
        if (r->ebo)     { glDeleteBuffers(1, &r->ebo);      r->ebo = 0; }
        if (r->vbo)     { glDeleteBuffers(1, &r->vbo);      r->vbo = 0; }
        if (r->vao)     { glDeleteVertexArrays(1, &r->vao);  r->vao = 0; }
        if (r->program) { glDeleteProgram(r->program);         r->program = 0; }
        SDL_GL_DestroyContext(r->gl_context);
        r->gl_context = nullptr;
    }
    if (r->w != nullptr) {
        SDL_DestroyWindow(r->w);
        r->w = nullptr;
    }
    SDL_Quit();
}

SDL_Window *H_CreateWindow() {
    const char *WINDOW_TITLE = "HelloTriangle";
    constexpr int WINDOW_HEIGHT = 600;
    constexpr int WINDOW_WIDTH = 800;
    SDL_Window *w = scp(SDL_CreateWindow(
                WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 
                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));
    return w;
}

SDL_GLContext H_LoadGL(SDL_Window *w) {
    SDL_GLContext gl = scp(SDL_GL_CreateContext(w));
    scc(SDL_GL_SetSwapInterval(1));
    if (gladLoadGL() == 0) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return nullptr;
    }
    return gl;
}

void H_Uniform(GLuint program) {
    GLint location = glGetUniformLocation(program, "time");
    float curTime = SDL_GetTicks() / 1200.0f;
    glUniform1f(location, (GLfloat)curTime);
    GLint locat = glGetUniformLocation(program, "sampler");
    glUniform1i(locat, 0);
}

GLuint prepareTexture() {
    stbi_set_flip_vertically_on_load(true);
    int w = 0, h = 0, channels = 0;
    unsigned char *data = scp(stbi_load(
            "smile.jpeg",
            &w, &h, &channels,
            STBI_rgb_alpha));
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
            GL_TEXTURE_2D, 
            0, GL_RGBA,
            w, h, 0,
            GL_RGBA,
            GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    stbi_image_free(data);
    return texture;
}

GLuint prepareEBO() {
    unsigned int indices[] = {0, 1, 2};
    GLuint ebo = 0;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    return ebo;
}

GLuint prepareVBO() {
    float vertices[] = {
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 2.0f,
    };
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    return vbo;
}

GLuint prepareVAO() {
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray((GLuint)0);
    glVertexAttribPointer(
            (GLuint)0,
            (GLint)3,
            GL_FLOAT,
            GL_FALSE,
            (GLsizei)(8 * sizeof(float)),
            (void *)0);
    glEnableVertexAttribArray((GLuint)1);
    glVertexAttribPointer(
            (GLuint)1,
            (GLint)3,
            GL_FLOAT,
            GL_FALSE,
            (GLsizei)(8 * sizeof(float)),
            (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray((GLuint)2);
    glVertexAttribPointer(
            (GLuint)2,
            (GLint)2,
            GL_FLOAT,
            GL_FALSE,
            (GLsizei)(8 * sizeof(float)),
            (void *)(6 * sizeof(float)));
    return vao;
}

const char *vertexShaderSource =
    "#version 410 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "layout (location = 2) in vec2 aUV;\n"
    "uniform float time;\n"
    "out vec3 ourColor;\n"
    "out vec2 ourUV;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 1.0);\n"
    "    ourColor = aColor * (sin(time) * 0.5 + 0.5);\n"
    "    ourUV = aUV;\n"
    "}";

const char *fragShaderSource =
    "#version 410 core\n"
    "in vec3 ourColor;\n"
    "in vec2 ourUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D sampler;\n"
    "void main() {\n"
    "   FragColor = texture(sampler, ourUV);\n"
    "}";

void shaderCompileStatus(GLuint shader) {
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        constexpr int MAX_BUFFER_SIZE = 1024;
        char infoLog[MAX_BUFFER_SIZE];
        glGetShaderInfoLog(
                shader,
                (GLsizei)MAX_BUFFER_SIZE,
                (GLsizei *)nullptr,
                (GLchar *)infoLog);
        fprintf(stderr, "Error: %s", infoLog);
    }
}

GLuint prepareShader() {
    GLuint vertexShaderID = 0;
    vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(
            vertexShaderID,
            (GLsizei)1,
            &vertexShaderSource,
            (const GLint *)nullptr);
    glCompileShader(vertexShaderID);
    shaderCompileStatus(vertexShaderID);
    GLuint fragShaderID = 0;
    fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(
            fragShaderID,
            (GLsizei)1,
            &fragShaderSource,
            (const GLint *)nullptr);
    glCompileShader(fragShaderID);
    shaderCompileStatus(fragShaderID);
    GLuint shaderProgram = 0;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShaderID);
    glAttachShader(shaderProgram, fragShaderID);
    glLinkProgram(shaderProgram);
    GLint programStatus = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &programStatus);
    if (!programStatus) {
        constexpr int MAX_BUFFER_SIZE = 1024;
        char infoLog[MAX_BUFFER_SIZE];
        glGetProgramInfoLog(
                shaderProgram,
                (GLsizei)MAX_BUFFER_SIZE,
                (GLsizei *)nullptr,
                (GLchar *)infoLog);
        fprintf(stderr, "Error: %s", infoLog);
        glDeleteProgram(shaderProgram);
        return 0;
    }
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragShaderID);
    return shaderProgram;
}

void H_Render(AppResources *r) {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(r->w, &w, &h);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(r->program);
    H_Uniform(r->program);
    glBindVertexArray(r->vao);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,r->ebo);
    // glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawElements(GL_TRIANGLES,3,GL_UNSIGNED_INT, 0);
}

void AppResourcesInit(AppResources *r) {
    memset(r, 0, sizeof(*r));
    scc(SDL_Init(SDL_INIT_VIDEO));
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
            SDL_GL_CONTEXT_PROFILE_CORE);
    if (!(r->w = scp(H_CreateWindow()))) goto fail;
    if (!(r->gl_context = scp(H_LoadGL(r->w)))) goto fail;
    r->program = prepareShader();
    r->vbo = prepareVBO();
    r->vao = prepareVAO();
    r->ebo = prepareEBO();
    r->texture = prepareTexture();
    if (
            r->program == 0 || r->vao == 0 ||
            r->vbo == 0 || r->ebo == 0 ||
            r->texture == 0) goto fail;
    return;
fail:
    AppResourcesDestroy(r);
}

int main(int, char **) {
    AppResources app = {0};
    AppResourcesInit(&app);
    glClearColor(0.2f,0.3f,0.3f,1.0f);
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        H_Render(&app);
        SDL_GL_SwapWindow(app.w);
    }
    AppResourcesDestroy(&app);
    return 0;
}

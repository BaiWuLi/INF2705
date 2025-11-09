#pragma once

#include <glbinding/gl/gl.h>

using namespace gl;

class Model
{
public:
    void load(const char* path);
    
    ~Model();
    
    void draw();

private:
    GLuint vao_, vbo_, ebo_;
    GLsizei count_;
};


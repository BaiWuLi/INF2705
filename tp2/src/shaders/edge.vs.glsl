#version 330 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec3 normal;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(position + 0.05 * normal, 1.0);
}

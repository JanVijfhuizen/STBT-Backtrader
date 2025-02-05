#version 330 core
layout (location = 0) in float aPos;

uniform vec2 start;
uniform vec2 end;
uniform vec4 color;

out vec4 vertexColor;

void main()
{
    vec4 offset = vec4(start * (1 - aPos) + end * aPos, 0, 1);
    gl_Position = offset;
    vertexColor = color;
}
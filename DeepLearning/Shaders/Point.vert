#version 330 core
layout (location = 0) in float aPos;

uniform vec2 pos;
uniform float size;
uniform vec4 color;


out vec4 vertexColor;

void main()
{
    gl_Position = vec4(pos.xy, 0, 1);
    gl_PointSize = size;
    vertexColor = color;
}
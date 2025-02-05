#version 330 core
layout (location = 0) in vec3 aPos;

uniform vec2 position;
uniform vec2 scale;
uniform vec4 color;

out vec4 vertexColor;

void main()
{
    gl_Position = vec4(aPos.x * scale.x, aPos.y * scale.y, aPos.z, 1.0) + vec4(position, 0, 0);
    vertexColor = color;
}
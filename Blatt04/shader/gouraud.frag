#version 330 core

in vec3 vertexColor;

out vec3 fragColor;

void main()
{
    fragColor = vertexColor;
}
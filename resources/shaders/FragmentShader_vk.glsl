#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec3 FragPos;
layout (location = 2) in vec2 TexCoord;

void main()
{
    FragColor = vec4(0.8, 0.4, 0.2, 1.0);
}

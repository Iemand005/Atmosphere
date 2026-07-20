#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec3 FragPos;
layout (location = 2) in vec2 TexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 objectColor;
} pc;

layout(binding = 0) uniform UBO {
    mat4 view;
    mat4 projection;
};

layout(binding = 1) uniform sampler2D ourTexture;

void main()
{
    vec3 norm = normalize(Normal);

    vec3 sunDir = normalize(vec3(0.4, 0.8, 0.3));
    float sunDiff = max(dot(norm, sunDir), 0.0);
    vec3 sunColor = vec3(1.0, 0.95, 0.85) * sunDiff;

    vec3 ambient = vec3(0.12);

    vec3 lighting = ambient + sunColor;
    FragColor = vec4(pc.objectColor.rgb * lighting, pc.objectColor.a);
}

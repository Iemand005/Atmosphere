#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec3 FragPos;
layout (location = 2) out vec2 TexCoord;

#ifdef VULKAN
layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
};
#else
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
#endif

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;

    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalMat * aNormal;
    FragPos = worldPos.xyz;
    TexCoord = aTexCoord;
}

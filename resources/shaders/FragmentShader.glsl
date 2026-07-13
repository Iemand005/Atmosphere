#ifdef VULKAN
#version 450
#else
#version 330 core
#endif

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 Normal;
layout (location = 1) in vec3 FragPos;
layout (location = 2) in vec2 TexCoord;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

#ifdef VULKAN
layout (binding = 0) uniform sampler2D ourTexture;

layout(binding = 1) uniform LightingUBO {
    int lightCount;
    PointLight pointLights[8];
};
#else
uniform sampler2D ourTexture;

uniform int lightCount;
uniform PointLight pointLights[8];
#endif

void main()
{
    vec3 n = normalize(Normal);

    vec3 lighting = vec3(0.1);
    for (int i = 0; i < lightCount; ++i) {
        vec3 L = pointLights[i].position - FragPos;
        float dist = length(L);
        if (dist < 0.0001) continue;
        vec3 ldir = L / dist;

        float diff = max(dot(n, ldir), 0.0);
        float radius = max(pointLights[i].radius, 0.001);
        float atten = 1.0 / (1.0 + (dist * dist) / (radius * radius));

        vec3 contrib = pointLights[i].color * pointLights[i].intensity * diff * atten;
        lighting += contrib;
    }

    vec4 texSample = texture(ourTexture, TexCoord);
    FragColor = vec4(texSample.rgb * lighting, texSample.a);
}

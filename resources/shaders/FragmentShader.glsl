#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform vec3 objectColor;

void main()
{
    vec3 norm = normalize(Normal);

    vec3 sunDir = normalize(vec3(0.4, 0.8, 0.3));
    float sunDiff = max(dot(norm, sunDir), 0.0);
    vec3 sunColor = vec3(1.0, 0.95, 0.85) * sunDiff;

    vec3 ambient = vec3(0.12);

    vec3 lighting = ambient + sunColor;
    FragColor = vec4(objectColor * lighting, 1.0);
}

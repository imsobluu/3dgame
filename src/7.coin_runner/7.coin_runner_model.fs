#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform vec3 baseColor;
uniform vec3 lightDir;
uniform bool useTexture;

void main()
{
    vec3 norm = normalize(Normal);
    float diffuse = max(dot(norm, normalize(-lightDir)), 0.0);
    float ambient = 0.35;
    vec3 base = baseColor;
    if (useTexture)
    {
        base *= texture(texture_diffuse1, TexCoords).rgb;
    }
    vec3 color = base * (ambient + diffuse * 0.85);
    FragColor = vec4(color, 1.0);
}

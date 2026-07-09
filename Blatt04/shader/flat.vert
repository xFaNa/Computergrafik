#version 330 core

in vec3 position;
in vec3 normal;

uniform mat4 mvp;
uniform mat4 model;

uniform vec3 objectColor;

uniform int lightType;
uniform vec3 lightDirection;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

flat out vec3 vertexColor;

void main()
{
    vec4 worldPosition = model * vec4(position, 1.0);

    vec3 N = normalize(mat3(model) * normal);

    vec3 L;

    if (lightType == 0)
    {
        L = normalize(lightDirection);
    }
    else
    {
        L = normalize(lightPosition - worldPosition.xyz);
    }

    vec3 V = normalize(cameraPosition - worldPosition.xyz);
    vec3 R = reflect(-L, N);

    float ambientStrength = 0.15;
    float diffuseStrength = max(dot(N, L), 0.0);

    float specularStrength = 0.5;
    float shininess = 32.0;
    float specular = pow(max(dot(V, R), 0.0), shininess);

    vec3 ambient = ambientStrength * objectColor;
    vec3 diffuse = diffuseStrength * objectColor;
    vec3 specularColor = specularStrength * specular * vec3(1.0);

    vertexColor = ambient + diffuse + specularColor;

    gl_Position = mvp * vec4(position, 1.0);
}
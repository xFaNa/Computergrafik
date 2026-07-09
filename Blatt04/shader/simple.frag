#version 330 core

in vec3 fragmentNormal;
in vec3 fragmentPosition;

uniform vec3 objectColor;

uniform int lightType;
uniform vec3 lightDirection;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform float specularStrength;
uniform float shininess;

out vec3 fragColor;

void main()
{
	vec3 N = normalize(fragmentNormal);

	vec3 L;

	if (lightType == 0) {
	L = normalize(lightDirection);
	} else {
	L = normalize(lightPosition - fragmentPosition);
	}

	vec3 V = normalize(cameraPosition - fragmentPosition);
	vec3 R = reflect(-L, N);

	float ambientStrength = 0.15;
	float diffuseStrength = max (dot(N, L), 0.0);

	float specular = pow(max(dot(V, R), 0.0), shininess);

	vec3 ambient = ambientStrength * objectColor;
	vec3 diffuse = diffuseStrength * objectColor;
	vec3 specularColor = specularStrength * specular * vec3(1.0);

	fragColor = ambient + diffuse + specularColor;
}
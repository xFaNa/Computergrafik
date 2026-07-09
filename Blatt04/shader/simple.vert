#version 330 core

in vec3 position;
in vec3 normal;

uniform mat4 mvp;
uniform mat4 model;

out vec3 fragmentNormal;
out vec3 fragmentPosition;

void main()
{
	vec4 worldPosition = model * vec4(position, 1.0);

	fragmentPosition = worldPosition.xyz;
	fragmentNormal = normalize(mat3(model) * normal);

	gl_Position   = mvp * vec4(position,  1.0);
}
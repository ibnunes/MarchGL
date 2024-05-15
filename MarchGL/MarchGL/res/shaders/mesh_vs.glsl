#version 460 core
#extension GL_ARB_shading_language_include : require

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 vsPos;
out vec3 vsNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	vsPos = vec3(model * vec4(aPos, 1.f));
	vsNormal = mat3(transpose(inverse(model))) * aNormal;

	gl_Position = projection * view * vec4(vec3(model * vec4(aPos, 1.f)), 1.0);
}
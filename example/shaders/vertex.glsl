#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;

uniform mat4 model_transform;
uniform mat4 normal_transform;
uniform mat4 view_transform;
uniform mat4 projection_transform;

//out vec3 Normal;
out vec2 UV;
out vec3 WorldPosition;
out mat3 TBN;

void main() {
    gl_Position = projection_transform * view_transform * model_transform * vec4(position, 1.0);
    WorldPosition = vec3(model_transform * vec4(position, 1.0));
    //Normal = normalize(mat3(normal_transform) * normal);
    UV = uv;

    // TBN: Tangent space to world space.
    vec3 T = normalize(mat3(model_transform) * tangent);
    vec3 N = normalize(mat3(model_transform) * normal);
    vec3 B = normalize(cross(N, T));
    TBN = mat3(T, B, N);

    // TBN: World space to tangent space.
    //vec3 T = normalize((normal_transform * vec4(tangent, 0.0)).xyz);
    //vec3 N = normalize((normal_transform * vec4(normal, 0.0)).xyz);
    //vec3 B = cross(N, T);
    //mat3 TBN = transpose(mat3(T, B, N));
}
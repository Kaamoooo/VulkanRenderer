#version 450
#include "UBO.glsl"

layout (location = 0) out vec4 outColor;

void main() {
    vec3 edgeColor = vec3(0.9, 0.9, 0.0);
    outColor = vec4(edgeColor, 1.0);
}
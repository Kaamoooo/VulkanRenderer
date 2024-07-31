#version 450
#include "UBO.glsl"

layout (triangles) in;
layout (triangle_strip, max_vertices = 12) out;

layout (location = 0) in vec3 fragColor[];
layout (location = 1) in vec4 worldPos[];
layout (location = 2) in vec4 worldNormal[];
layout (location = 3) in vec2 outUV[];


void main() {
    
}

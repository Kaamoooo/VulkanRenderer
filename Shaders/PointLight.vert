#version 450
#include "UBO.glsl"

layout(location = 0) out vec2 fragOffset;

const vec2 OFFSETS[6] = vec2[](
vec2(-1, -1),
vec2(-1, 1),
vec2(1, -1),
vec2(1, -1),
vec2(-1, 1),
vec2(1, 1)
);

const float LIGHT_RADIUS=0.1;
 


layout(push_constant) uniform Push{
    vec4 position;
    vec4 color;
    float radius;
}push;

void main() {
    fragOffset = OFFSETS[gl_VertexIndex];
    vec3 worldCameraRight = vec3(ubo.viewMatrix[0][0], ubo.viewMatrix[1][0], ubo.viewMatrix[2][0]);
    vec3 worldCameraUp = vec3(ubo.viewMatrix[0][1], ubo.viewMatrix[1][1], ubo.viewMatrix[2][1]);

    vec3 worldPos = push.position.xyz +
    push.radius*fragOffset.x*worldCameraRight.xyz+
    push.radius*fragOffset.y*worldCameraUp.xyz;

    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(worldPos, 1);
}

#version 450

#include "../UBO.glsl"
layout (points) in;
layout (triangle_strip, max_vertices = 50) out;

layout (location = 0) in vec3 fragColor[];
layout (location = 1) in vec4 worldPos[];
layout (location = 2) in vec4 worldNormal[];
layout (location = 3) in vec2 uv[];

layout (location = 0) out vec3 outFragColor;
layout (location = 1) out vec4 outWorldPos;
layout (location = 2) out vec4 outWorldNormal;
layout (location = 3) out vec2 outUv;

layout (push_constant) uniform PushConstantData {
    mat4 modelMatrix;
    mat4 vaseModelMatrix;
} push;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float _Width = 0.014, _Height = 0.3, _BendDegree = 12, _BendSpeed = 3;
const float PI = 3.1415926535;
float _CollisionRadius = 0.45;

mat3 rotateMatrix(vec3 axis, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float t = 1.0 - c;

    vec3 axisNormalized = normalize(axis);

    float x = axisNormalized.x;
    float y = axisNormalized.y;
    float z = axisNormalized.z;

    mat3 rotationMatrix = mat3(
        t * x * x + c, t * x * y - s * z, t * x * z + s * y,
        t * x * y + s * z, t * y * y + c, t * y * z - s * x,
        t * x * z - s * y, t * y * z + s * x, t * z * z + c
    );

    return rotationMatrix;
}

void main() {
    vec4 rootWorldPos = worldPos[0];

    const float random = rand(uv[0]);
    rootWorldPos.x += random;
    rootWorldPos.z += random;
    _Width += random * 0.02;
    _Height += random * 0.2;
    float curHeight = 0;
    float layerHeight = _Height * 0.2;
    float bendRadian = (_BendDegree + sin(ubo.curTime * _BendSpeed + random * 180) * 6 + random) * PI / 180;
    vec3 obstacleWorldPos = push.vaseModelMatrix[3].xyz;
    float obstacleRootDistance = distance(vec4(push.modelMatrix * rootWorldPos).xyz, obstacleWorldPos);
    bool isBend = false;
    if (obstacleRootDistance < _CollisionRadius)
    {
        isBend = true;
    }

    for (int i = 0; i < 12; i++)
    {
        if (isBend)
        {
            float maxBendRadian = radians(90);
            bendRadian = mix(maxBendRadian, 0, clamp(0, 1, obstacleRootDistance / _CollisionRadius));
            vec3 forward = (push.modelMatrix * vec4(0, 0, 1, 1)).xyz;
            vec3 dir = normalize(rootWorldPos.xyz - obstacleWorldPos);
            if (dot(forward, dir) < 0)
            {
                bendRadian *= -1;
            }
            isBend = true;
        }
        int level = i / 2;
        float widthWeight = 0.8 - 0.03 * level * level;
        vec3 axis;
        vec3 up;
        float randomX = rand(rootWorldPos.xx);
        float randomZ = rand(rootWorldPos.zz);
        if (i % 2 == 0) {
            outWorldPos = vec4(rootWorldPos.x - _Width * widthWeight * randomX, rootWorldPos.y - curHeight, rootWorldPos.z - _Width * widthWeight * randomZ, 1);
            outUv = vec2(0, curHeight / _Height);
            axis = vec3(rootWorldPos.x - outWorldPos.x, 0, rootWorldPos.z - outWorldPos.z);
        } else {
            outWorldPos = vec4(rootWorldPos.x + _Width * widthWeight * randomX, rootWorldPos.y - curHeight, rootWorldPos.z + _Width * widthWeight * randomZ, 1);
            outUv = vec2(1, curHeight / _Height);
            curHeight += layerHeight;
            axis = vec3(outWorldPos.x - rootWorldPos.x, 0, outWorldPos.z - rootWorldPos.z);
        }
        up = normalize(cross(axis, vec3(0, 1, 0)));
        if (level == 2)
        {
            outWorldPos.x += layerHeight * sin(bendRadian) * dot(up, vec3(1, 0, 0));
            outWorldPos.z += layerHeight * sin(bendRadian) * dot(up, vec3(0, 0, 1));
            outWorldPos.y = rootWorldPos.y - layerHeight - layerHeight * cos(bendRadian);
        } else if (level == 3)
        {
            outWorldPos.x += layerHeight * (sin(bendRadian) + sin(2 * bendRadian)) * dot(up, vec3(1, 0, 0));
            outWorldPos.z += layerHeight * (sin(bendRadian) + sin(2 * bendRadian)) * dot(up, vec3(0, 0, 1));
            outWorldPos.y = rootWorldPos.y - layerHeight - layerHeight * (cos(bendRadian) + cos(2 * bendRadian));
        } else if (level == 4)
        {
            outWorldPos.x += layerHeight * (sin(bendRadian) + sin(2 * bendRadian) + sin(3 * bendRadian)) * dot(up, vec3(1, 0, 0));
            outWorldPos.z += layerHeight * (sin(bendRadian) + sin(2 * bendRadian) + sin(3 * bendRadian)) * dot(up, vec3(0, 0, 1));
            outWorldPos.y = rootWorldPos.y - layerHeight - layerHeight * (cos(bendRadian) + cos(2 * bendRadian) + cos(3 * bendRadian));
        } else if (level == 5)
        {
            outWorldPos.x += layerHeight * (sin(bendRadian) + sin(2 * bendRadian) + sin(3 * bendRadian) + sin(4 * bendRadian)) * dot(up, vec3(1, 0, 0));
            outWorldPos.z += layerHeight * (sin(bendRadian) + sin(2 * bendRadian) + sin(3 * bendRadian) + sin(4 * bendRadian)) * dot(up, vec3(0, 0, 1));
            outWorldPos.y = rootWorldPos.y - layerHeight - layerHeight * (cos(bendRadian) + cos(2 * bendRadian) + cos(3 * bendRadian) + cos(4 * bendRadian));
        }

        outFragColor = vec3(0, 0.5 + level * 0.04 + 0.1 * random, 0);
        mat3 rotationMatrix = rotateMatrix(axis, bendRadian);
        outWorldNormal = push.modelMatrix * vec4(rotationMatrix * up, 0);
        outWorldPos = push.modelMatrix * outWorldPos;
        gl_Position = ubo.projectionMatrix * ubo.viewMatrix * outWorldPos;
        EmitVertex();
    }
    EndPrimitive();
}
struct Light{
    vec4 position;
    vec4 direction;
    vec4 color;
// 0: point lights, 1: directional lights
    int lightCategory;
};

layout(set=0, binding=0) uniform GlobalUbo{
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    Light lights[10];
    int lightNum;    
    mat4 lightProjectionViewMatrix;
    float curTime;
    mat4 shadowViewMatrix[6];
    mat4 shadowProjMatrix;

} ubo;

int getCubeMapIndex(vec3 direction){
    float x = abs(direction.x);
    float y = abs(direction.y);
    float z = abs(direction.z);
    float max = max(max(x, y), z);
    if (max==x){
        if (direction.x>0){
            return 0;
        }else{
            return 1;
        }
    }else if (max==y){
        if (direction.y>0){
            return 2;
        }else{
            return 3;
        }
    }else{
        if (direction.z>0){
            return 4;
        }else{
            return 5;
        }
    }
}
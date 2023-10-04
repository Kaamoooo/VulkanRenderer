#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = gl_MaxGeometryOutputVertices) out;

layout(location=0) in vec3 inColor[];
layout(location=0) out vec3 outColor;

layout(push_constant) uniform PushConstantData{
    mat4 transform;
    vec3 color;
    vec3 forwardDir;
    int fractalIterations;
} pushConstantData;

vec4 vertices[200];
 
void main() {
    for (int i=0;i<200;i++){
        vertices[i]=vec4(0, 0, 0, 0);
    }

    int iterations=pushConstantData.fractalIterations;

    vec4 V0 = gl_in[0].gl_Position;
    vec4 V1 = gl_in[1].gl_Position;
    vec4 V2 = gl_in[2].gl_Position;

    float l0 = distance(V2, V1);
    float l1 = distance(V0, V2);
    float l2 = distance(V1, V0);

    float normalEdgeLength=l1;

    //整理三角形的顶点顺序，逆时针，V0-V1-V2
    vec4 tmp;
    float epsilon=0.001;
    if (abs(l0-l1)<epsilon){
        normalEdgeLength=l0;
        tmp=V0;
        V0=V2;
        V2=V1;
        V1=tmp;
    }
    if (abs(l0-l2)<epsilon){
        normalEdgeLength=l2;
        tmp=V2;
        V2=V0;
        V0=V1;
        V1=tmp;
    }

    int index=-1;

    vertices[++index]=V0;
    vertices[++index]=V1;
    vertices[++index]=V2;

    //1 1/3 1/9 
    float finalEdgeLength = normalEdgeLength / pow(3.0, float(iterations));


    //1 2 25 ...
    for (int i=0;i<240;i++){
        if (index<=0)break;
        vec4 v2=vertices[index--];
        vec4 v1=vertices[index--];
        vec4 v0=vertices[index--];
        float tmpEdgeLength = distance(v0, v1);
        if (tmpEdgeLength<epsilon)continue;
        if (abs(tmpEdgeLength-finalEdgeLength)<epsilon){
            if (iterations==0){
                gl_Position = pushConstantData.transform * v0;
                outColor=v0.xyz;
                EmitVertex();
                gl_Position = pushConstantData.transform * v1;
                outColor=v1.xyz;
                EmitVertex();
                gl_Position = pushConstantData.transform * v2;
                outColor=v2.xyz;
                EmitVertex();
                EndPrimitive();
                return;
            }

            if (iterations<3){
                index-=33;
            } else {
                index-=21;
            }
            int baseIndex=index+1;

            //5 10 1 2 16 23
            gl_Position = pushConstantData.transform * vertices[baseIndex+5];
            outColor=vertices[baseIndex+5].xyz;
            EmitVertex();
            gl_Position = pushConstantData.transform * vertices[baseIndex+10];
            outColor=vertices[baseIndex+10].xyz;
            EmitVertex();
            gl_Position = pushConstantData.transform * vertices[baseIndex+1];
            outColor=vertices[baseIndex+1].xyz;
            EmitVertex();
            gl_Position=pushConstantData.transform * vertices[baseIndex+2];
            outColor=vertices[baseIndex+2].xyz;
            EmitVertex();
            gl_Position=pushConstantData.transform * vertices[baseIndex+16];
            outColor=vertices[baseIndex+16].xyz;
            EmitVertex();
            gl_Position=pushConstantData.transform * vertices[baseIndex+23];
            outColor=vertices[baseIndex+23].xyz;
            EmitVertex();
            EndPrimitive();

            baseIndex+=24;
            //3 1 2 0 9 8
            //小于三次迭代才绘制内部立体部分
            if (iterations<3){
                gl_Position=pushConstantData.transform * vertices[baseIndex+3];
                outColor=vertices[baseIndex+3].xyz;
                EmitVertex();
                gl_Position=pushConstantData.transform * vertices[baseIndex+1];
                outColor=vertices[baseIndex+1].xyz;
                EmitVertex();
                gl_Position=pushConstantData.transform * vertices[baseIndex+2];
                outColor=vertices[baseIndex+2].xyz;
                EmitVertex();
                gl_Position=pushConstantData.transform * vertices[baseIndex];
                outColor=vertices[baseIndex].xyz;
                EmitVertex();
                gl_Position=pushConstantData.transform * vertices[baseIndex+9];
                outColor=vertices[baseIndex+9].xyz;
                EmitVertex();
                gl_Position=pushConstantData.transform * vertices[baseIndex+8];
                outColor=vertices[baseIndex+8].xyz;
                EmitVertex();
                EndPrimitive();
            }
            continue;
        }

        vec4 v20 = (v1-v0)/3+v0;
        vec4 v21=(v1-v0)*2/3+v0;
        vec4 v10=(v2-v0)/3+v0;
        vec4 v12=(v2-v0)*2/3+v0;
        vec4 v01=(v2-v1)/3+v1;
        vec4 v02=(v2-v1)*2/3+v1;

        vec4 v3=v10-v0+v20;
        vec4 v30=vec4(normalize(cross(vec3((v02-v3).xyz), vec3((v01-v3).xyz))), 0)*tmpEdgeLength/3+v3;
        vec4 v010=vec4(normalize(cross(vec3((v21-v01).xyz), vec3((v3-v01).xyz))), 0)*tmpEdgeLength/3+v01;
        vec4 v020=vec4(normalize(cross(vec3((v3-v02).xyz), vec3((v12-v02).xyz))), 0)*tmpEdgeLength/3+v02;


        //01 1 3 0 02 2
        //5 10 1 2 16 23

        //0 1 2
        //2 10 23

        vertices[++index]=v20;
        vertices[++index]=v3;
        vertices[++index]=v0;

        vertices[++index]=v3;
        vertices[++index]=v20;
        vertices[++index]=v01;

        vertices[++index]=v21;
        vertices[++index]=v01;
        vertices[++index]=v20;

        vertices[++index]=v21;
        vertices[++index]=v1;
        vertices[++index]=v01;

        vertices[++index]=v10;
        vertices[++index]=v0;
        vertices[++index]=v3;

        vertices[++index]=v3;
        vertices[++index]=v02;
        vertices[++index]=v10;

        vertices[++index]=v12;
        vertices[++index]=v10;
        vertices[++index]=v02;

        vertices[++index]=v12;
        vertices[++index]=v02;
        vertices[++index]=v2;


        if (iterations<3){
            //010 01 30 3 020 02
            //3 1 2 0 9 8
            vertices[++index]=v3;
            vertices[++index]=v01;
            vertices[++index]=v30;

            vertices[++index]=v010;
            vertices[++index]=v30;
            vertices[++index]=v01;

            vertices[++index]=v3;
            vertices[++index]=v30;
            vertices[++index]=v02;

            vertices[++index]=v020;
            vertices[++index]=v02;
            vertices[++index]=v30;

        } else {
            for (int k=0;k<12;k++){
                vertices[++index]=vec4(0, 0, 0, 0);
            }
        }
    }
}
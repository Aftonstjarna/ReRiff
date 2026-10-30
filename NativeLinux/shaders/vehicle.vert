#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 color;
layout(location=3) in vec2 material;
layout(location=4) in vec3 textureCoordinate;
layout(set=1,binding=0) uniform Camera { mat4 viewProjection; } camera;
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec3 worldNormal;
layout(location=2) out vec3 baseColor;
layout(location=3) out vec2 surface;
layout(location=4) out vec3 textureUV;
void main() {
    gl_Position=camera.viewProjection*vec4(position,1.0);
    worldPosition=position;
    worldNormal=normal;
    baseColor=color;
    surface=material;
    textureUV=textureCoordinate;
}

#version 450

layout (location = 0) out vec4 outColor;
layout (location = 0) in vec3 fragmentColor;

void main(){
    //    RGBA
    outColor = vec4(fragmentColor, 1.0);
}

#version 330 core

flat in vec3 outColor;
out vec4 color;

uniform vec3 objectColor;

void main()
{
    //color = vec4(objectColor, 1.0);
    color = vec4(outColor, 1.0);
}
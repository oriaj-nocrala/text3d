#version 330 core

layout (location = 0) in vec2 aPos;        // Vertex position
layout (location = 1) in vec2 aTexCoords;   // Texture coordinate

uniform mat4 transform; // Matriz para escalar/posicionar el quad

out vec2 TexCoords;      // Pass texture coordinate to fragment shader

void main() {
   gl_Position = transform * vec4(aPos, 0.0, 1.0);
   TexCoords = aTexCoords;
}
#version 330 core

layout (location = 0) in vec2 aPos;
uniform mat4 transform; // Matriz para escalar/posicionar

void main() {
   gl_Position = transform * vec4(aPos.x, aPos.y, 0.0, 1.0);
}
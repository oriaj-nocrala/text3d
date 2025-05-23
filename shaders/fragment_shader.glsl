#version 330 core

out vec4 FragColor;

in vec2 TexCoord; // Received from vertex shader

uniform sampler2D sdfTexture; // The SDF texture
uniform vec3 textColor;       // Base color for the text

void main() {
   // Sample the SDF texture (assuming single channel, e.g., GL_R8, so use .r)
   float distance = texture(sdfTexture, TexCoord).r;

   // Determine smoothing factor. fwidth() gives good results for sharpness.
   // A small fixed value can also be used if fwidth is problematic or for simpler implementations.
   float smoothing = fwidth(distance);
   // float smoothing = 0.04; // Alternative: fixed smoothing

   // Calculate alpha based on the distance and smoothing
   // Assumes SDF range is [0,1] and 0.5 is the contour.
   float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);

   // Final color: combine textColor with calculated alpha.
   // If textColor had an alpha component, you might want to multiply: FragColor = vec4(textColor.rgb, textColor.a * alpha);
   FragColor = vec4(textColor, alpha);
}
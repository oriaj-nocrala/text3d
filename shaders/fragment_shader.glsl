#version 330 core

in vec2 TexCoords; // Coordenadas de textura del vertex shader

out vec4 FragColor;

uniform sampler2D sdfTexture;   // Tu textura SDF (monocanal, GL_R8)
uniform vec3 textColor;           // Color base del texto
uniform float sdfEdgeValue;       // El valor en la textura que representa el contorno (ej. 0.5)
uniform float smoothingFactor;    // Factor para controlar el suavizado del borde

// --- Opcionales para efectos ---
uniform bool enableOutline;
uniform vec3 outlineColor;
uniform float outlineWidthSDF;     // Grosor del contorno en unidades SDF
uniform float outlineEdgeOffset;   // Desplazamiento del borde del contorno desde el borde del texto

uniform bool enableShadow;
uniform vec4 shadowColor;         // << CORREGIDO AQUÍ: de vec3 a vec4
// uniform vec2 shadowOffsetScreen; // Si decides usarlo, necesitarás convertirlo a UV
uniform float shadowSoftnessSDF;   // Suavizado de la sombra en unidades SDF

void main() {
    float distanceSample = texture(sdfTexture, TexCoords).r;
    float screenPixelRange = fwidth(distanceSample);
    float antialiasWidth = screenPixelRange * smoothingFactor;

    // --- Renderizado Base del Texto (con anti-aliasing) ---
    float textAlpha = smoothstep(sdfEdgeValue - antialiasWidth, 
                                 sdfEdgeValue + antialiasWidth, 
                                 distanceSample);

    vec3 finalColor = textColor;
    float finalAlpha = textAlpha;

    // --- Efecto de Contorno (Opcional) ---
    if (enableOutline) {
        float outlineOuterEdge = sdfEdgeValue + outlineEdgeOffset + outlineWidthSDF;
        float outlineInnerEdge = sdfEdgeValue + outlineEdgeOffset;

        float outlineAlphaPass = smoothstep(outlineInnerEdge - antialiasWidth, 
                                            outlineInnerEdge + antialiasWidth, 
                                            distanceSample) -
                                 smoothstep(outlineOuterEdge - antialiasWidth, 
                                            outlineOuterEdge + antialiasWidth, 
                                            distanceSample);
        float currentOutlineAlpha = clamp(outlineAlphaPass, 0.0, 1.0);
        
        finalColor = mix(finalColor, outlineColor, currentOutlineAlpha * (1.0 - textAlpha));
        finalAlpha = max(finalAlpha, currentOutlineAlpha);
    }
    
    // --- Efecto de Sombra (Opcional) ---
    if (enableShadow) {
        // Ejemplo de offset UV fijo para la sombra.
        // Podrías hacerlo un uniform vec2 shadowOffsetUV si quieres controlarlo desde C.
        vec2 shadowOffsetUV = vec2(0.003, -0.003); 
        
        float shadowDistanceSample = texture(sdfTexture, TexCoords - shadowOffsetUV).r;
        
        float shadowAntialiasWidth = screenPixelRange * shadowSoftnessSDF;
        float calculatedShadowAlpha = smoothstep(sdfEdgeValue - shadowAntialiasWidth, 
                                                 sdfEdgeValue + shadowAntialiasWidth, 
                                                 shadowDistanceSample);
        
        // Aplicar el alfa del color de la sombra (que ahora es un vec4)
        calculatedShadowAlpha *= shadowColor.a; 

        // Composición: la sombra se mezcla detrás del color actual (texto + contorno)
        // Usamos shadowColor.rgb porque finalColor es vec3
        vec3 blendedColorWithShadow = mix(shadowColor.rgb, finalColor, finalAlpha);
        float blendedAlphaWithShadow = finalAlpha + calculatedShadowAlpha * (1.0 - finalAlpha); // Composición de alfa

        finalColor = blendedColorWithShadow;
        finalAlpha = blendedAlphaWithShadow;
    }

    FragColor = vec4(finalColor, finalAlpha);

    // if (FragColor.a < 0.01) {
    //     discard;
    // }
}
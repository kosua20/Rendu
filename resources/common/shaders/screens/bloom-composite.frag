#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; 
uniform float mixFactor = 1.0;

layout(location = 0) out vec3 fragColor; ///< Scene color.

/**  */
void main(){
	
	vec3 color = textureLod(screenTexture, In.uv, 0.0).rgb;
	fragColor = mixFactor*color;
}

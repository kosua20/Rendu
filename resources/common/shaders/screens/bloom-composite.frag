#version 330

/// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Bloom texture.
uniform float mixFactor = 1.0; ///< Blend factor when compositing.

layout(location = 0) out vec3 fragColor; ///< Scene color.

/** Overlay the bloom result over the current content of the destination. */
void main(){
	
	vec3 color = textureLod(screenTexture, In.uv, 0.0).rgb;
	fragColor = mixFactor*color;
}

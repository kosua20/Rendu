#version 330

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler2D sceneTexture; ///< Scene texture.
layout(binding = 1) uniform sampler2D bloomTexture; ///< Bloom texture.
uniform float mixFactor = 1.0; ///< Blend factor when compositing.

layout(location = 0) out vec3 fragColor; ///< Scene color.

/** Overlay the bloom result over the current content of the destination. */
void main(){
	
	vec3 scene = textureLod(sceneTexture, In.uv, 0.0).rgb;
	vec3 bloom = textureLod(bloomTexture, In.uv, 0.0).rgb;
	fragColor = scene + mixFactor * bloom;
}

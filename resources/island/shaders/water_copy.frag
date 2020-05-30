#version 400

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

uniform float time;

layout(binding = 0) uniform sampler2D colorTexture; ///< Color to output.
layout(binding = 1) uniform sampler2D posTexture; ///< Position to output.
layout(binding = 2) uniform sampler2D caustics; ///< Position to output.
layout(binding = 3) uniform sampler2D normalMap; ///< Position to output.

layout(location = 0) out vec3 fragColor; ///< Color.
layout(location = 1) out vec3 fragPos; ///< Position.

/** Just pass the input image as-is, without any resizing. */
void main(){
	fragPos = textureLod(posTexture, In.uv, 0.0).rgb;
	// Compute world-space based UV coordinates.
	vec2 uvs = fragPos.xz + sin(0.1 * time * vec2(0.1, 0.7));
	// Fetch displacement vector from low-frequency normal map.
	vec2 disp =	texture(normalMap, 0.1*uvs).xy;
	// Fetch caustic intensity.
	float caust = texture(caustics, fragPos.xz + 2.0*sin(disp)).x;
	caust *= 10.0*caust;
	// Combine ocean floor color and caustics.
	fragColor = (1.0 + caust) *textureLod(colorTexture, In.uv, 0.0).rgb;
}

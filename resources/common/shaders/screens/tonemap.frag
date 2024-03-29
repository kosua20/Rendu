#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D screenTexture; ///< Image to tonemap.

layout(set = 0, binding = 0) uniform UniformBlock {
	float customExposure; ///< Exposure
	bool apply; ///< Apply the tonemapping operator (or just clamp).
};

layout(location = 0) out vec3 fragColor; ///< Color.

/** Reinhard tonemapping operator.
\param hdrColor input HDR color
\return the LDR color
*/
vec3 reinhard(vec3 hdrColor){
	return hdrColor / (1.0 + hdrColor);
}

/** Simple exposure-based tonemapping operator.
\param hdrColor input HDR color
\param exposure the current image overall exposure
\return the LDR color
*/
vec3 simpleExposure(vec3 hdrColor, float exposure){
	return 1.0 - exp(-hdrColor * exposure);
}

/** ACES tonemapping operator.
\param hdrColor input HDR color
\return the LDR color
*/
vec3 aces(vec3 hdrColor){
	return clamp(((0.9036 * hdrColor + 0.018) * hdrColor) / ((0.8748 * hdrColor + 0.354) * hdrColor + 0.14), 0.0, 1.0);
}

/** Cineon tonemapping operator.
\param hdrColor input HDR color
\return the LDR color
*/
vec3 cineon(vec3 hdrColor){
	vec3 shiftedColor = max(vec3(0.0), hdrColor - 0.004);
	return (shiftedColor * (6.2 * shiftedColor + 0.5)) / (shiftedColor * (6.2 * shiftedColor + 1.7) + 0.06);
}

/** Uncharted 2 tonemapping operator.
\param hdrColor input HDR color
\return the LDR color
*/
vec3 uncharted2(vec3 hdrColor){
	vec3 x = 2.0 * hdrColor; // Exposure bias.
	vec3 newColor = ((x * (0.15 * x + 0.05) + 0.004) / (x * (0.15 * x + 0.5) + 0.06)) - 0.02/0.3;
	return newColor * 1.3790642467; // White scale
}

/** Apply a tonemapping operator to bring a HDR image to LDR. */
void main(){
	
	vec3 finalColor = textureLod(sampler2D(screenTexture, sClampLinear),In.uv, 0.0).rgb;
	fragColor = apply ? simpleExposure(finalColor, customExposure) : clamp(finalColor, 0.0, 1.0);
	
	// Test if any component is still > 1, for debug purposes.
	//fragColor = any(greaterThan(fragColor, vec3(1.0))) ? vec3(1.0,0.0,0.0) : fragColor;
	
}

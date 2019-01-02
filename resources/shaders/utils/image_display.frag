#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to output.

uniform bool isHDR; ///< Is the image an HDR one.
uniform float exposure; ///< User selected exposure.
uniform bool gammaOutput; ///< Should gamma correction be applied.
uniform vec4 channelsFilter; ///< Which channels should be displayed.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Simple exposure-based tonemapping operator.
 \param hdrColor input HDR color
 \param exposure the current image overall exposure
 \return the LDR color
 */
vec3 simpleExposure(vec3 hdrColor, float exposure){
	return 1.0 - exp(-hdrColor * exposure);
}

/** Apply a standard gamma correction.
 \param color the color to correct
 \return gamma correct color for screens
 */
vec3 gamma(vec3 color){
	return pow(color, vec3(1.0/2.2));
}

/**
 Render the image, applying user-defined postprocessing.
 */
void main(){
	vec2 uv = In.uv;
	if(any(greaterThan(uv, vec2(1.0))) || any(lessThan(uv, vec2(0.0)))){
		discard;
	}
	
	fragColor = texture(screenTexture, uv);
	
	if(isHDR){
		vec3 exposedColor = simpleExposure(fragColor.rgb, exposure);
		fragColor.rgb = exposedColor;
	}
	
	if(gammaOutput){
		fragColor.rgb = gamma(fragColor.rgb);
	}
	
	// Apply channels filters.
	fragColor *= channelsFilter;
	
	// If alpha layer is disabled, set it to 1 instead of 0.
	if(channelsFilter[3] == 0.0){
		fragColor.a = 1.0;
	}
}

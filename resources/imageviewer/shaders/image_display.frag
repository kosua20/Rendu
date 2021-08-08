layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 1, binding = 0) uniform sampler2D screenTexture; ///< Image to output.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec4 channelsFilter; ///< Which channels should be displayed.
	float exposure; ///< User selected exposure.
	bool gammaOutput; ///< Should gamma correction be applied.
	bool isHDR; ///< Is the image an HDR one.
};

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
	return pow(color, vec3(2.2));
}

/**
 Render the image, applying user-defined postprocessing.
 */
void main(){
	vec2 uv = In.uv;
	if(any(greaterThan(uv, vec2(1.0))) || any(lessThan(uv, vec2(0.0)))){
		discard;
	}
	
	fragColor = textureLod(screenTexture, uv, 0.0);
	
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

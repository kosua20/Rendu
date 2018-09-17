#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to output.

uniform bool isHDR;
uniform float exposure;

layout(location = 0) out vec4 fragColor; ///< Color.

/** Simple exposure-based tonemapping operator.
 \param hdrColor input HDR color
 \param exposure the current image overall exposure
 \return the LDR color
 */
vec3 simpleExposure(vec3 hdrColor, float exposure){
	return 1.0 - exp(-hdrColor * exposure);
}

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
}

#version 330

/// Input: position in model space
in INTERFACE {
	vec3 pos;
} In ; ///< vec3 pos;

uniform vec3 shCoeffs[9]; ///< SH approximation of the environment irradiance.

layout(location = 0) out vec3 fragColor; ///< Color.


/** Evaluate the ambient irradiance (as SH coefficients) in a given direction. 
	\param wn the direction (normalized)
	\return the ambient irradiance
	*/
vec3 applySH(vec3 wn){
	return (shCoeffs[7] * wn.z + shCoeffs[4]  * wn.y + shCoeffs[8]  * wn.x + shCoeffs[3]) * wn.x +
		   (shCoeffs[5] * wn.z - shCoeffs[8]  * wn.y + shCoeffs[1]) * wn.y +
		   (shCoeffs[6] * wn.z + shCoeffs[2]) * wn.z +
		    shCoeffs[0];
}

/** Compute the environment ambient lighting contribution on the scene. */
void main(){
	vec3 envLighting = applySH(normalize(In.pos));
	fragColor = envLighting;
}

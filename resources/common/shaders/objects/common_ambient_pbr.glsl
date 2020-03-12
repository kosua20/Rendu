
/** Return the (pre-convolved) radiance for a given normal, view direction and
	material parameters.
	\param n the surface normal
	\param v the view direction
	\param inverseV inverse view direction
	\param roughness the surface roughness
	\param cubeMap the environment map texture
	\param upLod the maximal mip level to fetch
	\return the radiance value
	*/
vec3 radiance(vec3 n, vec3 v, float roughness, mat4 inverseV, samplerCube cubeMap, float upLod){
	// Compute local frame.
	vec3 r = -reflect(v,n);
	r = normalize((inverseV * vec4(r, 0.0)).xyz);
	
	vec3 specularColor = textureLod(cubeMap, r, upLod * roughness).rgb;
	return specularColor;
}

/** Evaluate the ambient irradiance (as SH coefficients) in a given direction. 
	\param wn the direction (normalized)
	\param coeffs the SH coefficients
	\return the ambient irradiance
	*/
vec3 applySH(vec3 wn, vec3 coeffs[9]){
	return (coeffs[7] * wn.z + coeffs[4]  * wn.y + coeffs[8]  * wn.x + coeffs[3]) * wn.x +
		   (coeffs[5] * wn.z - coeffs[8]  * wn.y + coeffs[1]) * wn.y +
		   (coeffs[6] * wn.z + coeffs[2]) * wn.z +
		    coeffs[0];
}

/** Estimate specular ambient occlusion. Based on "Moving Frostbite to Physically Based Rendering".
	\param diffuseAO diffuse visbility factor
	\param NdotV visibility/normal angle
	\param roughness linear material roughness
	\return the estimated specular visibility
*/
float approximateSpecularAO(float diffuseAO, float NdotV, float roughness){
	float specAO = pow(NdotV + diffuseAO, exp2(-16.0 * roughness - 1.0));
	return clamp(specAO - 1.0 + diffuseAO, 0.0, 1.0);
}

void ambientBrdf(vec3 baseColor, float metallic, float roughness, float NdotV, sampler2D brdfCoeffs, out vec3 diffuse, out vec3 specular){
	
	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	// Adjust Fresnel based on roughness.
	vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
    vec3 Fs = F0 + Fr * pow(1.0 - NdotV, 5.0);
	// Specular single scattering contribution (preintegrated).
	vec2 brdfParams = texture(brdfCoeffs, vec2(NdotV, roughness)).rg;
	specular = (brdfParams.x * Fs + brdfParams.y);
	
	// Account for multiple scattering.
	// Based on A Multiple-Scattering Microfacet Model for Real-Time Image-based Lighting, C. J. Fdez-Agüera, JCGT, 2019.
    float scatter = (1.0 - (brdfParams.x + brdfParams.y));
    vec3 Favg = F0 + (1.0 - F0) / 21.0;
    vec3 multi = scatter * specular * Favg / (1.0 - Favg * scatter);
	// Diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 single = (1.0 - metallic) * baseColor * (1.0 - F0);
	diffuse = single * (1.0 - specular - multi) + multi;
}

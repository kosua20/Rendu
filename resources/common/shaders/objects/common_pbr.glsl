
#include "common.glsl"

/** Estimate the position of the current fragment in view space based on its depth and camera parameters.
\param depth the depth of the fragment
\param uv the input uv
\param projMat the non trivial coefficients of the projection matrix
\return the view space position
*/
vec3 positionFromDepth(float depth, vec2 uv, vec4 projMat){
	float depth2 = 2.0 * depth - 1.0 ;
	vec2 ndcPos = 2.0 * uv - 1.0;
	// Linearize depth -> in view space.
	float viewDepth = - projMat.w / (depth2 + projMat.z);
	// Compute the x and y components in view space.
	return vec3(-ndcPos * viewDepth / projMat.xy , viewDepth);
}

/** Fresnel approximation.
	\param F0 fresnel based coefficient
	\param VdotH angle between the half and view directions
	\return the Fresnel term
*/
vec3 F(vec3 F0, float VdotH){
	float approx = exp2((-5.55473 * VdotH - 6.98316) * VdotH);
	return F0 + approx * (1.0 - F0);
}

/** GGX Distribution term.
	\param NdotH angle between the half and normal directions
	\param alpha the roughness squared
	\return the distribution term
*/
float D(float NdotH, float alpha){
	float halfDenum = NdotH * NdotH * (alpha * alpha - 1.0) + 1.0;
	float halfTerm = alpha / max(0.0001, halfDenum);
	return halfTerm * halfTerm * INV_M_PI;
}

/** Visibility term of GGX BRDF, V=G/(n.v)(n.l)
\param NdotL dot product of the light direction with the surface normal
\param NdotV dot product of the view direction with the surface normal
\param alpha squared roughness
\return the value of V
*/
float V(float NdotL, float NdotV, float alpha){
	// Correct version.
	float alpha2 = alpha * alpha;
	float visL = NdotV * sqrt((-NdotL * alpha2 + NdotL) * NdotL + alpha2);
	float visV = NdotL * sqrt((-NdotV * alpha2 + NdotV) * NdotV + alpha2);
    return 0.5 / max(0.0001, visV + visL);
}

/** Linearized visibility term of GGX BRDF, V=G/(n.v)(n.l)
\param NdotL dot product of the light direction with the surface normal
\param NdotV dot product of the view direction with the surface normal
\param alpha squared roughness
\return the value of V
*/
float Vfast(float NdotL, float NdotV, float alpha){
    float visV = NdotL * (NdotV * (1.0 - alpha) + alpha);
    float visL = NdotV * (NdotL * (1.0 - alpha) + alpha);
	return 0.5 / max(0.0001, visV + visL);
}

/** Evaluate the GGX BRDF for a given normal, view direction and
	material parameters.
	\param n the surface normal
	\param v the view direction
	\param l the light direction
	\param F0 the Fresnel coefficient
	\param roughness the surface roughness
	\return the BRDF value
*/
vec3 ggx(vec3 n, vec3 v, vec3 l, vec3 F0, float roughness){
	// Compute half-vector.
	vec3 h = normalize(v+l);
	// Compute all needed dot products.
	float NdotL = clamp(dot(n,l), 0.0, 1.0);
	float NdotV = clamp(dot(n,v), 0.0, 1.0);
	float NdotH = clamp(dot(n,h), 0.0, 1.0);
	float VdotH = clamp(dot(v,h), 0.0, 1.0);
	float alpha = max(0.0001, roughness*roughness);

	return D(NdotH, alpha) * Vfast(NdotL, NdotV, alpha) * F(F0, VdotH);
}

/** Compute an arbitrary sample of the 2D Hammersley sequence.
\param i the index in the hammersley sequence
\param scount the total number of samples
\return the i-th 2D sample
*/
vec2 hammersleySample(uint i, int scount) {
	uint bits = i;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float y = float(bits) * 2.3283064365386963e-10; // / 0x100000000
	return vec2(float(i)/float(scount), y);
}


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
vec3 applySH(vec3 wn, vec4 coeffs[9]){
	return (coeffs[7].rgb * wn.z + coeffs[4].rgb  * wn.y + coeffs[8].rgb  * wn.x + coeffs[3].rgb) * wn.x +
		   (coeffs[5].rgb * wn.z - coeffs[8].rgb  * wn.y + coeffs[1].rgb) * wn.y +
		   (coeffs[6].rgb * wn.z + coeffs[2].rgb) * wn.z +
		    coeffs[0].rgb;
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

/** Evaluate the global lighting contirbution from the ambient environment. Implements a multi-scattering compensation step, as described in A Multiple-Scattering Microfacet Model for Real-Time Image-based Lighting, C. J. Fdez-Agüera, JCGT, 2019.
 \param baseColor the surface color/Fresnel coefficient for metals
 \param metallic is the surface a metal or a dielectric
 \param roughness the linear material roughness
 \param NdotV visibility/normal angle
 \param brdfCoeffs precomputed BRDF linearized coefficients lookup table
 \param diffuse will contain the diffuse ambient contribution
 \param specular will contain the specular ambient contribution
 */
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
    float scatter = (1.0 - (brdfParams.x + brdfParams.y));
    vec3 Favg = F0 + (1.0 - F0) / 21.0;
    vec3 multi = scatter * specular * Favg / (1.0 - Favg * scatter);
	// Diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 single = (1.0 - metallic) * baseColor * (1.0 - F0);
	diffuse = single * (1.0 - specular - multi) + multi;
}


#include "utils.glsl"
#include "constants.glsl"
#include "geometry.glsl"
#include "samplers.glsl"
#include "materials.glsl"
#include "shadow_maps.glsl"

#define POINT 0
#define DIRECTIONAL 1
#define SPOT 2

/** \brief Environment probe data. */
struct Probe {
	vec3 position; ///< Probe location.
	vec3 center; ///< Parallax proxy box center.
	vec3 extent; ///< Parallax proxy box half-size (or -1 if at infinity).
	vec3 size; ///< Effect box half-size.
	vec2 orientationCosSin; ///< Precomputed orientation.
	float fade; ///< Size of the fading region.
	float maxLod; ///< Number of mips in the probe texture.
};

/** \brief Analytic light data. Some members might only be valid for some types of lights. */
struct Light {
	mat4 viewToLight; ///< View to light matrix.
	vec3 color; ///< Light tint.
	vec3 position; ///< Light position.
	vec3 direction; ///< Light direction.
	vec2 angles; ///< Cone inner and outer angles.
	float radius; ///< Effect radius.
	float farPlane; ///< Far plane distance in the shadow map.
	uint type; ///< Light type.
	uint shadowMode; ///< Shadow mode.
	uint layer; ///< Shadow map layer.
	float bias; ///< Shadow bias
};

/** Fresnel approximation.
	\param F0 fresnel based coefficient
	\param VdotH angle between the half and view directions
	\return the Fresnel term
*/
vec3 F(vec3 F0, float VdotH){
	return F0 + pow(1.0 - VdotH, 5) * (1.0 - F0);
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
	\param h the half vector between the view and light
	\param F0 the Fresnel coefficient
	\param roughness the surface roughness
	\return the BRDF value
*/
vec3 ggx(vec3 n, vec3 v, vec3 l, vec3 h, vec3 F0, float roughness){
	// Compute all needed dot products.
	float NdotL = clamp(dot(n,l), 0.0, 1.0);
	float NdotV = clamp(dot(n,v), 0.0, 1.0);
	float NdotH = clamp(dot(n,h), 0.0, 1.0);
	float VdotH = clamp(dot(v,h), 0.0, 1.0);
	float alpha = max(0.0001, roughness*roughness);

	return D(NdotH, alpha) * Vfast(NdotL, NdotV, alpha) * F(F0, VdotH);
}

/** Return the (pre-convolved) radiance for a given direction (in world space) and
	material parameters.
	\param r the direction to query (world space)
	\param p the surface point (world space)
	\param roughness the surface roughness
	\param cubeMap the environment map texture
	\param probe the probe parameters
	\return the radiance value
	*/
vec3 radiance(vec3 r, vec3 p, float roughness, textureCube cubeMap, Probe probe){

	// Compute the direction to fetch in the cubemap.
	if(probe.extent.x > 0.0){
		// We go from world to box frame.
		vec3 rBox = rotateY(r, probe.orientationCosSin);
		vec3 pBox = rotateY(p - probe.center, probe.orientationCosSin);
		vec2 roots;
		intersectBox(pBox, rBox, probe.extent, roots);
		float dist = max(roots.x, roots.y);
		vec3 hitPos = p + dist * r;
		r = (hitPos - probe.position);
	}
	vec3 specularColor = textureLod(samplerCube(cubeMap, sClampLinearLinear), toCube(r), probe.maxLod * roughness).rgb;
	return specularColor;
}

/** Compute the contribution of a probe to a given point in the scene, based on the probe effect box size, position and fading settings.
 \param p the position in the scene (world space)
 \param probe the probe parameters
 \return the contribution weight in [0,1]
 */
float probeWeight(vec3 p, Probe probe){
	// Compute distance to effect box (signed).
	vec3 pBox = rotateY(p - probe.position, probe.orientationCosSin);
	vec3 q = abs(pBox) - probe.size;
	float dist = length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
	float weight = 1.0 - clamp(dist / probe.fade, 0.0, 1.0);
	return weight;
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

/** Evaluate the global lighting contribution from the ambient environment. Implements a multi-scattering compensation step, as described in A Multiple-Scattering Microfacet Model for Real-Time Image-based Lighting, C. J. Fdez-Agüera, JCGT, 2019.
 \param material the surface point material parameters
 \param NdotV visibility/normal angle
 \param brdfCoeffs precomputed BRDF linearized coefficients lookup table
 \param diffuse will contain the diffuse ambient contribution
 \param specular will contain the specular ambient contribution
 */
void ambientBrdf(Material material, float NdotV, texture2D brdfCoeffs, out vec3 diffuse, out vec3 specular){
	vec3 baseColor = material.reflectance;
	float metallic = material.metalness;
	float roughness = material.roughness;

	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	// Adjust Fresnel based on roughness.
	vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
    vec3 Fs = F0 + Fr * pow(1.0 - NdotV, 5.0);
	// Specular single scattering contribution (preintegrated).
	vec2 brdfParams = texture(sampler2D(brdfCoeffs, sClampLinear), vec2(NdotV, roughness)).rg;
	specular = (brdfParams.x * Fs + brdfParams.y);

	// Account for multiple scattering.
    float scatter = (1.0 - (brdfParams.x + brdfParams.y));
    vec3 Favg = F0 + (1.0 - F0) / 21.0;
    vec3 multi = scatter * specular * Favg / (1.0 - Favg * scatter);
	// Diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 single = (1.0 - metallic) * baseColor * (1.0 - F0);
	diffuse = single * (1.0 - specular - multi) + multi;
}

/** Evaluate the lighting contribution from an analytic light source.
 \param material the surface point material parameters
 \param p the surface position (in world space)
 \param n the surface normal (in world space)
 \param v the outgoing view direction (in world space)
 \param r the reflected direction (in world space)
 \param NdotV the dot product of the view and normal directions
 \param probe the environment probe parameters
 \param envmap the environment probe texture
 \param envSH the environment probe irradiance coefficients
 \param brdfLUT the preintegrated BRDF lookup table
 \param diffuse will contain the diffuse ambient contribution
 \param specular will contain the specular ambient contribution
 */
void ambientLighting(Material material, vec3 p, vec3 n, vec3 v, vec3 r, float NdotV, Probe probe, textureCube envmap, vec4 envSH[9], texture2D brdfLUT, out vec3 diffuse, out vec3 specular){
	
	vec3 radiance = radiance(r, p, material.roughness, envmap, probe);

	vec3 irradiance = applySH(n, envSH);

	// BRDF contributions.
	ambientBrdf(material, NdotV, brdfLUT, diffuse, specular);

	// Specular AO.
	float aoSpecular = approximateSpecularAO(material.ao, NdotV, material.roughness);
	// Combine BRDF, incoming (ir)radiance and occlusion.
	diffuse = material.ao * diffuse * irradiance;
	specular = aoSpecular * specular * radiance;
}

/** Evaluate the lighting contribution from an analytic light source.
 \param material the surface point material parameters
 \param n the surface normal (in view space)
 \param v the outgoing view direction (in view space)
 \param l the incoming light direction (in view space)
 \param diffuse will contain the diffuse direct contribution
 \param specular will contain the specular direct contribution
 */
void directBrdf(Material material, vec3 n, vec3 v, vec3 l, out vec3 diffuse, out vec3 specular){

	// BRDF contributions.
	float metallic = material.metalness;
	vec3 baseColor = material.reflectance;

	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);

	// Orientation: basic diffuse shadowing.
	float orientation = max(0.0, dot(l,n));
	// Compute half-vector.
	vec3 h = normalize(v+l);

	// Normalized diffuse contribution. Metallic materials have no diffuse contribution.
	diffuse = orientation * INV_M_PI * (1.0 - metallic) * baseColor * (1.0 - F0);
	// Specular GGX contribution.
	specular = orientation * ggx(n, v, l, h, F0, material.roughness);

}

/** Compute a point light contribution for a given scene point.
 \param light the light information
 \param viewSpacePos the point position in view space
 \param shadowMap the cube shadow maps
 \param l will contain the light direction for the point
 \param shadowing will contain the shadowing factor
 \return true if the light contributes to the point shading
 */
bool applyPointLight(Light light, vec3 viewSpacePos, textureCubeArray shadowMap, out vec3 l, out float shadowing){

	shadowing = 1.0;

	vec3 deltaPosition = light.position - viewSpacePos;
	// Early exit if we are outside the sphere of influence.
	if(length(deltaPosition) > light.radius){
		return false;
	}
	// Light direction: from the surface point to the light point.
	l = normalize(deltaPosition);
	// Attenuation with increasing distance to the light.
	float localRadius2 = dot(deltaPosition, deltaPosition);
	float radiusRatio2 = localRadius2 / (light.radius * light.radius);
	float attenNum = clamp(1.0 - radiusRatio2, 0.0, 1.0);
	shadowing = attenNum*attenNum;

	// Shadowing.
	if(light.shadowMode != SHADOW_NONE){
		// Compute the light to surface vector in light centered space.
		// We only care about the direction, so we don't need the translation.
		vec3 deltaPositionWorld = -mat3(light.viewToLight) * deltaPosition;
		shadowing *= shadowCube(light.shadowMode, deltaPositionWorld, shadowMap, light.layer, light.farPlane, light.bias);
	}
	return true;
}

/** Compute a directional light contribution for a given scene point.
 \param light the light information
 \param viewSpacePos the point position in view space
 \param shadowMap the 2D shadow maps
 \param l will contain the light direction for the point
 \param shadowing will contain the shadowing factor
 \return true if the light contributes to the point shading
 */
bool applyDirectionalLight(Light light, vec3 viewSpacePos, texture2DArray shadowMap, out vec3 l, out float shadowing){

	shadowing = 1.0;
	l = normalize(-light.direction);
	// Shadowing
	if(light.shadowMode != SHADOW_NONE){
		vec3 lightSpacePosition = (light.viewToLight * vec4(viewSpacePos, 1.0)).xyz;
		lightSpacePosition.xy = 0.5 * lightSpacePosition.xy + 0.5;
		shadowing *= shadow(light.shadowMode, lightSpacePosition, shadowMap, light.layer, light.bias);
	}

	return true;
}

/** Compute a spot light contribution for a given scene point.
 \param light the light information
 \param viewSpacePos the point position in view space
 \param shadowMap the 2D shadow maps
 \param l will contain the light direction for the point
 \param shadowing will contain the shadowing factor
 \return true if the light contributes to the point shading
 */
bool applySpotLight(Light light, vec3 viewSpacePos, texture2DArray shadowMap, out vec3 l, out float shadowing){

	shadowing = 1.0;

	vec3 deltaPosition = light.position - viewSpacePos;
	float lightRadius = light.radius;
	// Early exit if we are outside the sphere of influence.
	if(length(deltaPosition) > light.radius){
		return false;
	}
	l = normalize(deltaPosition);
	// Compute the angle between the light direction and the (light, surface point) vector.
	float currentAngleCos = dot(l, -normalize(light.direction));
	// If we are outside the spotlight cone, no lighting.
	if(currentAngleCos < light.angles.y){
		return false;
	}
	// Compute the spotlight attenuation factor based on our angle compared to the inner and outer spotlight angles.
	float angleAttenuation = clamp((currentAngleCos - light.angles.y)/(light.angles.x - light.angles.y), 0.0, 1.0);
	// Attenuation with increasing distance to the light.
	float localRadius2 = dot(deltaPosition, deltaPosition);
	float radiusRatio2 = localRadius2/(light.radius*light.radius);
	float attenNum = clamp(1.0 - radiusRatio2, 0.0, 1.0);
	shadowing = angleAttenuation * attenNum * attenNum;

	// Shadowing
	if(light.shadowMode != SHADOW_NONE){
		vec4 lightSpacePosition = (light.viewToLight) * vec4(viewSpacePos,1.0);
		lightSpacePosition /= lightSpacePosition.w;
		lightSpacePosition.xy = 0.5 * lightSpacePosition.xy + 0.5;
		shadowing *= shadow(light.shadowMode, lightSpacePosition.xyz, shadowMap, light.layer, light.bias);
	}
	return true;
}

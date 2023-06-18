
#include "constants.glsl"
#include "shadow_maps.glsl"
#include "common_pbr.glsl"

#define MAX_LIGHTS_COUNT 50
#define MAX_PROBES_COUNT 4

/** \brief Represent a light in the forward renderer. */
struct GPUPackedLight {
	mat4 viewToLight; ///< View to light matrix.
	vec4 colorAndBias; ///< Light tint and shadow bias.
	vec4 positionAndRadius; ///< Light position and effect radius.
	vec4 directionAndPlane; ///< Light direction and far plane distance.
	vec4 typeModeLayer; ///< Light type, shadow mode and shadow map layer.
	vec4 angles; ///< Cone inner and outer angles.
};

/** \brief Represent an environment probe. */
struct GPUPackedProbe {
	vec4 positionAndMip; ///< The cubemap location and the mip count.
	vec4 sizeAndFade;	 ///< The cubemap box effect size, and the size of its fading region on edges.
	vec4 centerAndCos; ///< The cubemap parallax box center, and the cubemap parallax box orientation (precomputed cos).
	vec4 extentAndSin; ///< The cubemap parallax box half size, and the cubemap parallax box orientation (precomputed sin).
};

/** Convert a forward renderer packed probe to the generic shader representation.
 \param srcProbe the packed probe
 \return the populated probe
 */
Probe unpackProbe(GPUPackedProbe srcProbe){
	Probe probe;
	probe.position = srcProbe.positionAndMip.xyz;
	probe.center = srcProbe.centerAndCos.xyz;
	probe.extent = srcProbe.extentAndSin.xyz;
	probe.size = srcProbe.sizeAndFade.xyz;
	probe.orientationCosSin = vec2(srcProbe.centerAndCos.w, srcProbe.extentAndSin.w);
	probe.fade = srcProbe.sizeAndFade.w;
	probe.maxLod = srcProbe.positionAndMip.w;
	return probe;
}

/** Convert a forward renderer packed light to the generic shader representation.
 \param srcLight the packed light
 \return the populated light
 */
Light unpackLight(GPUPackedLight srcLight){
	Light light;
	light.viewToLight = srcLight.viewToLight;
	light.color = srcLight.colorAndBias.xyz;
	light.position = srcLight.positionAndRadius.xyz;
	light.direction = srcLight.directionAndPlane.xyz;
	light.angles = srcLight.angles.xy;
	light.radius = srcLight.positionAndRadius.w;
	light.farPlane = srcLight.directionAndPlane.w;
	light.type = uint(srcLight.typeModeLayer[0]);
	light.shadowMode = uint(srcLight.typeModeLayer[1]);
	light.layer = uint(srcLight.typeModeLayer[2]);
	light.bias = srcLight.colorAndBias.w;
	return light;
}

/** Compute a light contribution for a given point in forward shading.
 \param gpuLight the light packed information
 \param viewSpacePos the point position in view space
 \param viewSpaceN the surface normal in view space
 \param smapCube the cube shadow maps
 \param smap2D the 2D shadow maps
 \param l will contain the light direction for the point
 \param shadowing will contain the shadowing factor
 \return true if the light contributes to the point shading
 */
bool applyLight(GPUPackedLight gpuLight, vec3 viewSpacePos, vec3 viewSpaceN, textureCubeArray smapCube, texture2DArray smap2D, out vec3 l, out float shadowing){

	Light light = unpackLight(gpuLight);

	if(light.type == POINT){
		return applyPointLight(light, viewSpacePos, viewSpaceN, smapCube, l, shadowing);
	} else if(light.type == DIRECTIONAL){
		return applyDirectionalLight(light, viewSpacePos, viewSpaceN, smap2D, l, shadowing);
	} else if(light.type == SPOT){
		return applySpotLight(light, viewSpacePos, viewSpaceN, smap2D, l, shadowing);
	}
	return true;
}

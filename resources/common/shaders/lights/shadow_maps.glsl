#include "utils.glsl"
#include "samplers.glsl"

#define SHADOW_NONE 0
#define SHADOW_BASIC 1
#define SHADOW_PCF 2
#define SHADOW_VARIANCE 3

/** Compute the lighting factor based on shadow map.
	\param lightSpacePosition fragment position in light space
	\param smap the shadow map
	\param layer the texture layer to read from
	\param bias the bias to apply
	\return the lighting factor (0.0 = in shadow)
*/
float shadowBasic(vec3 lightSpacePosition, texture2DArray smap, uint layer, float bias){
	// Avoid shadows when falling outside the shadow map.
	if(any(greaterThan(abs(lightSpacePosition.xy-0.5), vec2(0.5)))){
		return 1.0;
	}
	// Read depth from shadow map.
	float depth = textureLod(sampler2DArray(smap, sClampLinear), vec3(lightSpacePosition.xy, layer), 0.0).r;
	if(depth >= 1.0){
		// No information in the depthmap: no occluder.
		return 1.0;
	}
	// Initial probability of light.
	return float(lightSpacePosition.z <= depth + bias);
}


/** Compute the lighting factor based on shadow map.
	\param lightToPosDir direction from the light to the fragment position, in world space.
	\param smap the shadow cube map
	\param layer the texture layer to read from
	\param farPlane distance to the light projection far plane
	\param bias the bias to apply
	\return the lighting factor (0.0 = in shadow)
*/
float shadowBasicCube(vec3 lightToPosDir, textureCubeArray smap, uint layer, float farPlane, float bias){
	// Read depth from shadow map.
	float depth = textureLod(samplerCubeArray(smap, sClampLinear), toCube(lightToPosDir, layer), 0.0).r;
	if(depth >= 1.0){
		// No information in the depthmap: no occluder.
		return 1.0;
	}
	// Initial probability of light.
	// We have to correct for the frustum size.
	float dist = length(lightToPosDir)/farPlane;
	return float(dist <= depth + bias);
}

/** Compute the lighting factor based on shadow map.
	\param lightSpacePosition fragment position in light space
	\param smap the shadow map
	\param layer the texture layer to read from
	\param bias the bias to apply
	\return the lighting factor (0.0 = in shadow)
*/
float shadowPCF(vec3 lightSpacePosition, texture2DArray smap, uint layer, float bias){
	// Avoid shadows when falling outside the shadow map.
	if(any(greaterThan(abs(lightSpacePosition.xy-0.5), vec2(0.5)))){
		return 1.0;
	}

	vec2 textureSize = textureSize(smap, 0).xy;
	vec2 texelSize = 1.0 / textureSize;

	float totalOcclusion = 0.0;
	float totalWeight = 0.0;

	// Read first and second moment from shadow map.
	for(int y = -1; y <= 1; y += 1){
		for(int x = -1; x <= 1; x += 1){

			vec2 coords = lightSpacePosition.xy + vec2(x,y) * texelSize;
			vec4 depths = textureGather(sampler2DArray(smap, sClampNear), vec3(coords, layer), 0);
			bvec4 valids = lessThan(depths, vec4(1.0));
			float invWeight = (abs(x)+abs(y)) * 0.1 + 1.0;
			bvec4 visibles = lessThanEqual(lightSpacePosition.zzzz, depths + invWeight * bias);
			vec4 occlusions = mix(vec4(1.0), vec4(visibles), valids);

			vec2 texelPos = coords * textureSize - 0.5;
			vec2 texelFloor = floor(texelPos);
			vec2 texelFrac = texelPos - texelFloor;

			float mix0 = mix(occlusions.w, occlusions.z, texelFrac.x);
		    float mix1 = mix(occlusions.x, occlusions.y, texelFrac.x);

			totalOcclusion += mix(mix0, mix1, texelFrac.y);
			totalWeight += 1.0;
		}
	}
	totalOcclusion /= totalWeight;
    return totalOcclusion;
}

/** Convert a unit direction to a cubemap face and local UVs
 \param dir the direction to sample
 \param uDir will contain the axis corresponding to the U coordinate
 \param vDir will contain the axis corresponding to the V coordinate
 \return the face index and the UV coordinates
 */
vec3 cubemapToFaceIdAndUV(vec3 dir, out vec3 uDir, out vec3 vDir){
	vec3 absDir = abs(dir);
	int side = 0;
	float x  = 0.0;
	float y  = 0.0;
	float denom = 1.0;
	if(absDir.x >= absDir.y && absDir.x >= absDir.z) {
		denom = absDir.x;
		y	  = dir.y;
		vDir = vec3(0.0,1.0, 0.0);
		// X faces.
		if(dir.x >= 0.0) {
			side = 0;
			x	 = -dir.z;
			uDir = vec3(0.0,0.0,-1.0);
		} else {
			side = 1;
			x	 = dir.z;
			uDir = vec3(0.0,0.0,1.0);
		}
		
	} else if(absDir.y >= absDir.x && absDir.y >= absDir.z) {
		denom = absDir.y;
		x	  = dir.x;
		uDir = vec3(1.0,0.0,0.0);
		// Y faces.
		if(dir.y >= 0.0) {
			side = 2;
			y	 = -dir.z;
			vDir = vec3(0.0,0.0,-1.0);
		} else {
			side = 3;
			y	 = dir.z;
			vDir = vec3(0.0,0.0,1.0);
		}
	} else if(absDir.z >= absDir.x && absDir.z >= absDir.y) {
		denom = absDir.z;
		y	  = dir.y;
		vDir = vec3(0.0,1.0,0.0);
		// Z faces.
		if(dir.z >= 0.0) {
			side = 4;
			x	 = dir.x;
		uDir = vec3(1.0,0.0,0.0);
		} else {
			side = 5;
			x	 = -dir.x;
			uDir = vec3(-1.0,0.0,0.0);
		}
	}
	return vec3(side, 0.5 * vec2(x, y) / denom + 0.5);
}

/** Compute the lighting factor based on shadow map.
	\param lightToPosDir direction from the light to the fragment position, in world space.
	\param smap the shadow cube map
	\param layer the texture layer to read from
	\param farPlane distance to the light projection far plane
	\param bias the bias to apply
	\return the lighting factor (0.0 = in shadow)
*/
float shadowPCFCube(vec3 lightToPosDir, textureCubeArray smap, uint layer, float farPlane, float bias){

	// Avoid shadows when falling outside the shadow map.

	vec2 textureSize = textureSize(smap, 0).xy;
	vec2 texelSize = 1.0 / textureSize;

	float totalOcclusion = 0.0;
	float totalWeight = 0.0;
	float dist = length(lightToPosDir)/farPlane;

	vec3 uDir, vDir;
	vec3 faceAndUvs = cubemapToFaceIdAndUV(lightToPosDir, uDir, vDir);

	// Read first and second moment from shadow map.
	for(int y = -1; y <= 1; y += 1){
		for(int x = -1; x <= 1; x += 1){

			vec3 coords = normalize(lightToPosDir + x * texelSize.x * uDir + y * texelSize.y * vDir);

			vec4 depths = textureGather(samplerCubeArray(smap, sClampNear), toCube(coords, layer), 0);

			bvec4 valids = lessThan(depths, vec4(1.0));
			float invWeight = abs(x)+abs(y)+1.0;
			bvec4 visibles = lessThanEqual(vec4(dist), depths + invWeight * bias);
			vec4 occlusions = mix(vec4(1.0), vec4(visibles), valids);
			vec3 uu, vv;
			vec3 newUvs = cubemapToFaceIdAndUV(coords, uu, vv);

			vec2 texelPos = (newUvs.yz) * textureSize - 0.5;
			vec2 texelFloor = floor(texelPos);
			vec2 texelFrac = texelPos - texelFloor;

			float mix0 = mix(occlusions.w, occlusions.z, texelFrac.x);
		    float mix1 = mix(occlusions.x, occlusions.y, texelFrac.x);

			totalOcclusion += mix(mix0, mix1, texelFrac.y);
			totalWeight += 1.0;
		}
	}
	totalOcclusion /= totalWeight;
    return totalOcclusion;
}

/** Compute the lighting factor based on shadow map, using variance shadow mapping.
	\param lightSpacePosition fragment position in light space
	\param smap the shadow map
	\param layer the texture layer to read from
	\return the lighting factor (0.0 = in shadow)
*/
float shadowVSM(vec3 lightSpacePosition, texture2DArray smap, uint layer){
	// Avoid shadows when falling outside the shadow map.
	if(any(greaterThan(abs(lightSpacePosition.xy-0.5), vec2(0.5)))){
		return 1.0;
	}
	float probabilityMax = 1.0;
	// Read first and second moment from shadow map.
	vec2 moments = textureLod(sampler2DArray(smap, sClampLinear), vec3(lightSpacePosition.xy, layer), 0.0).rg;
	if(moments.x >= 1.0){
		// No information in the depthmap: no occluder.
		return 1.0;
	}
	// Initial probability of light.
	float probability = float(lightSpacePosition.z <= moments.x);
	// Compute variance.
	float variance = moments.y - (moments.x * moments.x);
	variance = max(variance, 0.00001);
	// Delta of depth.
	float d = lightSpacePosition.z - moments.x;
	// Use Chebyshev to estimate bound on probability.
	probabilityMax = variance / (variance + d*d);
	probabilityMax = max(probability, probabilityMax);
	// Limit light bleeding by rescaling and clamping the probability factor.
	probabilityMax = clamp( (probabilityMax - 0.1) / (1.0 - 0.1), 0.0, 1.0);
	return probabilityMax;
}

/** Compute the lighting factor based on shadow map, using variance shadow mapping.
	\param lightToPosDir direction from the light to the fragment position, in world space.
	\param smap the shadow cube map
	\param layer the texture layer to read from
	\param farPlane distance to the light projection far plane
	\return the lighting factor (0.0 = in shadow)
*/
float shadowVSMCube(vec3 lightToPosDir, textureCubeArray smap, uint layer, float farPlane){
	float probabilityMax = 1.0;
	// Read first and second moment from shadow map.
	vec2 moments = textureLod(samplerCubeArray(smap, sClampLinear), toCube(lightToPosDir, layer), 0.0).rg;
	if(moments.x >= 1.0){
		// No information in the depthmap: no occluder.
		return 1.0;
	}
	// Initial probability of light.
	// We have to correct for the frustum size.
	float dist = length(lightToPosDir)/farPlane;
	float probability = float(dist <= moments.x);
	// Compute variance.
	float variance = moments.y - (moments.x * moments.x);
	variance = max(variance, 0.00001);
	// Delta of depth.
	float d = dist - moments.x;
	// Use Chebyshev to estimate bound on probability.
	probabilityMax = variance / (variance + d*d);
	probabilityMax = max(probability, probabilityMax);
	// Limit light bleeding by rescaling and clamping the probability factor.
	probabilityMax = clamp( (probabilityMax - 0.1) / (1.0 - 0.1), 0.0, 1.0);
	return probabilityMax;
}


/** Compute the lighting factor based on shadow map.
	\param mode the shadow mapping technique (basic or VSM)
	\param lightSpacePosition fragment position in light space
	\param smap the shadow map
	\param layer the texture layer to read from
	\param bias the bias to apply
	\return the lighting factor (0.0 = in shadow)
*/
float shadow(uint mode, vec3 lightSpacePosition, texture2DArray smap, uint layer, float bias){
	if(mode == SHADOW_BASIC){
		return shadowBasic(lightSpacePosition, smap, layer, bias);
	}
	if(mode == SHADOW_PCF){
		return shadowPCF(lightSpacePosition, smap, layer, bias);
	}
	if(mode == SHADOW_VARIANCE) {
		return shadowVSM(lightSpacePosition, smap, layer);
	}
	return 1.0;
}


/** Compute the lighting factor based on shadow map.
\param mode the shadow mapping technique (basic or VSM)
	\param lightToPosDir direction from the light to the fragment position, in world space.
	\param smap the shadow cube map
	\param layer the texture layer to read from
	\param farPlane distance to the light projection far plane
	\param bias the bias to apply
	\return the lighting factor (0.0 = in shadow)
*/
float shadowCube(uint mode, vec3 lightToPosDir, textureCubeArray smap, uint layer, float farPlane, float bias){
	if(mode == SHADOW_BASIC) {
		return shadowBasicCube(lightToPosDir, smap, layer, farPlane, bias);
	}
	if(mode == SHADOW_PCF) {
		return shadowPCFCube(lightToPosDir, smap, layer, farPlane, bias);
	}
	if(mode == SHADOW_VARIANCE) {
		return shadowVSMCube(lightToPosDir, smap, layer, farPlane);
	}
	return 1.0;
}

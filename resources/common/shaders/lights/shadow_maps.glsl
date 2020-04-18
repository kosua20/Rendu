
#define SHADOW_NONE 0
#define SHADOW_BASIC 1
#define SHADOW_VARIANCE 2

/** Compute the lighting factor based on shadow map.
	\param lightSpacePosition fragment position in light space
	\param smap the shadow map
	\param layer the texture layer to read from
	\param bias the bias to apply
	\return the lighting factor (0.0 = in shadow)
*/
float shadowBasic(vec3 lightSpacePosition, sampler2DArray smap, int layer, float bias){
	// Avoid shadows when falling outside the shadow map.
	if(any(greaterThan(abs(lightSpacePosition.xy-0.5), vec2(0.5)))){
		return 1.0;
	}
	// Read first and second moment from shadow map.
	float depth = textureLod(smap, vec3(lightSpacePosition.xy, layer), 0.0).r;
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
float shadowBasicCube(vec3 lightToPosDir, samplerCubeArray smap, int layer, float farPlane, float bias){
	// Read first and second moment from shadow map.
	float depth = textureLod(smap, vec4(lightToPosDir, layer), 0.0).r;
	if(depth >= 1.0){
		// No information in the depthmap: no occluder.
		return 1.0;
	}
	// Initial probability of light.
	// We have to correct for the frustum size.
	float dist = length(lightToPosDir)/farPlane;
	return float(dist <= depth + bias);
}

/** Compute the lighting factor based on shadow map, using variance shadow mapping.
	\param lightSpacePosition fragment position in light space
	\param smap the shadow map
	\param layer the texture layer to read from
	\return the lighting factor (0.0 = in shadow)
*/
float shadowVSM(vec3 lightSpacePosition, sampler2DArray smap, int layer){
	// Avoid shadows when falling outside the shadow map.
	if(any(greaterThan(abs(lightSpacePosition.xy-0.5), vec2(0.5)))){
		return 1.0;
	}
	float probabilityMax = 1.0;
	// Read first and second moment from shadow map.
	vec2 moments = textureLod(smap, vec3(lightSpacePosition.xy, layer), 0.0).rg;
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
float shadowVSMCube(vec3 lightToPosDir, samplerCubeArray smap, int layer, float farPlane){
	float probabilityMax = 1.0;
	// Read first and second moment from shadow map.
	vec2 moments = textureLod(smap, vec4(lightToPosDir, layer), 0.0).rg;
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
float shadow(int mode, vec3 lightSpacePosition, sampler2DArray smap, int layer, float bias){
	if(mode == SHADOW_BASIC){
		return shadowBasic(lightSpacePosition, smap, layer, bias);
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
float shadowCube(int mode, vec3 lightToPosDir, samplerCubeArray smap, int layer, float farPlane, float bias){
	if(mode == SHADOW_BASIC) {
		return shadowBasicCube(lightToPosDir, smap, layer, farPlane, bias);
	}
	if(mode == SHADOW_VARIANCE) {
		return shadowVSMCube(lightToPosDir, smap, layer, farPlane);
	}
	return 1.0;
}

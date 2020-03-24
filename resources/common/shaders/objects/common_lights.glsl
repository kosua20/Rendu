
#include "common.glsl"
#include "shadow_maps.glsl"

#define POINT 0
#define DIRECTIONAL 1
#define SPOT 2

#define MAX_LIGHTS_COUNT 5

struct GPULight {
	mat4 viewToLight;
	vec4 colorAndBias;
	vec4 positionAndRadius;
	vec4 directionAndPlane;
	vec4 typeModeAngles;
};


bool applyLight(GPULight light, vec3 viewSpacePos, /*samplerCube shadowMap3D, sampler2D shadowMap2D,*/ out vec3 l, out float shadowing){

	shadowing = 1.0;

	int lightType = int(light.typeModeAngles[0]);
	if(lightType == POINT){
		vec3 deltaPosition = light.positionAndRadius.xyz - viewSpacePos;
		// Early exit if we are outside the sphere of influence.
		float lightRadius = light.positionAndRadius.w;
		if(length(deltaPosition) > lightRadius){
			return false;
		}
		// Light direction: from the surface point to the light point.
		l = normalize(deltaPosition);
		// Attenuation with increasing distance to the light.
		float localRadius2 = dot(deltaPosition, deltaPosition);
		float radiusRatio2 = localRadius2/(lightRadius*lightRadius);
		float attenNum = clamp(1.0 - radiusRatio2, 0.0, 1.0);
		shadowing = attenNum*attenNum;
		// Shadowing.
		int shadowMode = int(light.typeModeAngles[1]);
		/*if(shadowMode != SHADOW_NONE){
			// Compute the light to surface vector in light centered space.
			// We only care about the direction, so we don't need the translation.
			vec3 deltaPositionWorld = -mat3(light.viewToLight) * deltaPosition;
			shadowing *= shadowCube(shadowMode, deltaPositionWorld, shadowMap3D, light.directionAndPlane.w, light.colorAndBias.w);
		}*/
	} else if(lightType == DIRECTIONAL){
		l = normalize(-light.directionAndPlane.xyz);
		// Shadowing
		int shadowMode = int(light.typeModeAngles[1]);
		/*if(shadowMode != SHADOW_NONE){
			vec3 lightSpacePosition = 0.5 * ( mat3(light.viewToLight) * vec4(viewSpacePos, 1.0)).xyz + 0.5;
			shadowing *= shadow(shadowMode, lightSpacePosition, shadowMap2D, light.colorAndBias.w);
		}*/
	} else if(lightType == SPOT){
		vec3 deltaPosition = light.positionAndRadius.xyz - viewSpacePos;
		float lightRadius = light.positionAndRadius.w;
		// Early exit if we are outside the sphere of influence.
		if(length(deltaPosition) > lightRadius){
			return false;
		}
		l = normalize(deltaPosition);
		// Compute the angle between the light direction and the (light, surface point) vector.
		float currentAngleCos = dot(l, -normalize(light.directionAndPlane.xyz));
		vec2 intOutAnglesCos = light.typeModeAngles.zw;
		// If we are outside the spotlight cone, no lighting.
		if(currentAngleCos < intOutAnglesCos.y){
			return false;
		}
		// Compute the spotlight attenuation factor based on our angle compared to the inner and outer spotlight angles.
		float angleAttenuation = clamp((currentAngleCos - intOutAnglesCos.y)/(intOutAnglesCos.x - intOutAnglesCos.y), 0.0, 1.0);
		// Attenuation with increasing distance to the light.
		float localRadius2 = dot(deltaPosition, deltaPosition);
		float radiusRatio2 = localRadius2/(lightRadius*lightRadius);
		float attenNum = clamp(1.0 - radiusRatio2, 0.0, 1.0);
		shadowing = angleAttenuation * attenNum * attenNum;

		// Shadowing
		int shadowMode = int(light.typeModeAngles[1]);
		/*if(shadowMode != SHADOW_NONE){
			vec4 lightSpacePosition = mat3(light.viewToLight) * vec4(viewSpacePos,1.0);
			lightSpacePosition /= lightSpacePosition.w;
			shadowing *= shadow(shadowMode, 0.5*lightSpacePosition.xyz+0.5, shadowMap2D, light.colorAndBias.w);
		}*/
	}
	return true;
}

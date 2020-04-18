#include "common.glsl"

const float atmosphereGroundRadius = 6371e3; ///< Radius of the planet.
const float atmosphereTopRadius = 6471e3; ///< Radius of the atmosphere.
const float sunIntensity = 20.0; ///< Sun intensity.
const vec3 sunColor = vec3(1.474, 1.8504, 1.91198); ///< Sun direct color.
const vec3 kRayleigh = vec3(5.5e-6, 13.0e-6, 22.4e-6); ///< Rayleigh coefficients.
const float kMie = 21e-6; ///< Mie coefficients.
const float heightRayleigh = 8000.0; ///< Mie characteristic height.
const float heightMie = 1200.0; ///< Mie characteristic height.
const float gMie = 0.758; ///< Mie g constant.

const float sunAngularRadius = 0.04675; ///< Sun angular radius.
const float sunAngularRadiusCos = 0.998; ///< Cosine of the sun angular radius.

#define SAMPLES_COUNT_ATMO 16

/** Check if a sphere of a given radius is intersected by a ray defined by an 
	origin wrt to the sphere center and a normalized direction.
	\param rayOrigin the origin of the ray
	\param rayDir the direction of the ray (normalized)
	\param radius the radius of the sphere to intersect
	\param roots will contain the two roots of the associated polynomial, ordered.
	\return true if there is intersection.
	\warning The intersection can be in the negative direction along the ray. Check the sign of the roots to know.
*/
bool intersects(vec3 rayOrigin, vec3 rayDir, float radius, out vec2 roots){
	float a = dot(rayDir,rayDir);
	float b = dot(rayOrigin, rayDir);
	float c = dot(rayOrigin, rayOrigin) - radius*radius;
	float delta = b*b - a*c;
	// No intersection if the polynome has no real roots.
	if(delta < 0.0){
		return false;
	}
	// If it intersects, return the two roots.
	float dsqrt = sqrt(delta);
	roots = (-b+vec2(-dsqrt,dsqrt))/a;
	return true;
}

/** Compute the Rayleigh phase.
	\param cosAngle Cosine of the angle between the ray and the light directions
	\return the phase
*/
float rayleighPhase(float cosAngle){
	const float k = 1.0/(4.0*M_PI);
	return k * 3.0/4.0 * (1.0 + cosAngle*cosAngle);
}

/** Compute the Mie phase.
	\param cosAngle Cosine of the angle between the ray and the light directions
	\return the phase
*/
float miePhase(float cosAngle){
	const float k = 1.0/(4.0*M_PI);
	float g2 = gMie*gMie;
	return k * 3.0 * (1.0-g2) / (2.0 * (2.0 + g2)) * (1.0 + cosAngle*cosAngle) / pow(1 + g2 - 2.0 * gMie * cosAngle, 3.0/2.0);
}

/** Compute the radiance for a given ray, based on the atmosphere scattering model.
	\param rayOrigin the ray origin
	\param rayDir the ray direction
	\param sunDir the light direction
	\param scatterTable the precomputed secondary scattering lookup table
	\return the estimated radiance
*/
vec3 computeAtmosphereRadiance(vec3 rayOrigin, vec3 rayDir, vec3 sunDir, sampler2D scatterTable){
	// Check intersection with atmosphere.
	vec2 interTop, interGround;
	bool didHitTop = intersects(rayOrigin, rayDir, atmosphereTopRadius, interTop);
	// If no intersection with the atmosphere, it's the dark void of space.
	if(!didHitTop){
		return vec3(0.0);
	}
	// Now intersect with the planet.
	bool didHitGround = intersects(rayOrigin, rayDir, atmosphereGroundRadius, interGround);
	// Distance to the closest intersection.
	float distanceToInter = min(interTop.y, didHitGround ? interGround.x : 0.0);
	// Divide the distance traveled through the atmosphere in SAMPLES_COUNT_ATMO parts.
	float stepSize = (distanceToInter - interTop.x)/SAMPLES_COUNT_ATMO;
	// Angle between the sun direction and the ray.
	float cosViewSun = dot(rayDir, sunDir);
	
	// Accumulate optical distance for both scatterings.
	float rayleighDist = 0.0;
	float mieDist = 0.0;
	// Accumulate contributions for both scatterings.
	vec3 rayleighScatt = vec3(0.0);
	vec3 mieScatt = vec3(0.0);
	vec3 transmittance = vec3(0.0);
	
	// March along the ray.
	for(int i = 0; i < SAMPLES_COUNT_ATMO; ++i){
		// Compute the current position along the ray, ...
		vec3 currPos = rayOrigin + (i+0.5) * stepSize * rayDir;
		// ...and its distance to the ground (as we are in planet space).
		float currHeight = length(currPos) - atmosphereGroundRadius;
		// ... there is an artifact similar to clipping when close to the planet surface if we allow for negative heights.
		if(i == SAMPLES_COUNT_ATMO-1 && currHeight < 0.0){
			currHeight = 0.0;
		}
		// Compute density based on the characteristic height of Rayleigh and Mie.
		float rayleighStep = exp(-currHeight/heightRayleigh) * stepSize;
		float mieStep = exp(-currHeight/heightMie) * stepSize;
		// Accumulate optical distances.
		rayleighDist += rayleighStep;
		mieDist += mieStep;
		
		vec3 directAttenuation = exp(-(kMie * (mieDist) + kRayleigh * (rayleighDist)));
		
		// The secondary attenuation lookup table is parametrized by
		// the height in the atmosphere, and the cosine of the vertical angle with the sun.
		float relativeHeight = (length(currPos) - atmosphereGroundRadius) / (atmosphereTopRadius - atmosphereGroundRadius);
		float relativeCosAngle = -0.5*sunDir.y+0.5;
		// Compute UVs, scaled to read at the center of pixels.
		vec2 attenuationUVs = (1.0-1.0/512.0)*vec2(relativeHeight, relativeCosAngle)+0.5/512.0;
		vec3 secondaryAttenuation = texture(scatterTable, attenuationUVs).rgb;
		
		// Final attenuation.
		vec3 attenuation = directAttenuation * secondaryAttenuation;
		// Accumulate scatterings.
		rayleighScatt += rayleighStep * attenuation;
		mieScatt += mieStep * attenuation;
		transmittance += directAttenuation;
	}
	
	// Final scattering participations.
	vec3 rayleighParticipation = kRayleigh * rayleighPhase(cosViewSun) * rayleighScatt;
	vec3 mieParticipation = kMie * miePhase(cosViewSun) * mieScatt;
	
	// The sun itself if we're looking at it.
	vec3 sunRadiance = vec3(0.0);
	bool didHitGroundForward = didHitGround && interGround.y > 0;
	if(!didHitGroundForward && dot(rayDir, sunDir) > sunAngularRadiusCos){
		sunRadiance = sunColor / (M_PI * sunAngularRadius * sunAngularRadius);
	}
	
	return sunIntensity * (rayleighParticipation + mieParticipation) + transmittance * sunRadiance;
}


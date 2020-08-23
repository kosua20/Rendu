#include "constants.glsl"

/** Atmosphere parameters. */
struct AtmosphereParameters {
	vec3  sunColor; ///< Sun direct color.
	vec3  kRayleigh; ///< Rayleigh coefficients.
	float groundRadius; ///< Radius of the planet.
	float topRadius; ///< Radius of the atmosphere.
	float sunIntensity; ///< Sun intensity.
	float kMie; ///< Mie coefficients.
	float heightRayleigh; ///< Mie characteristic height.
	float heightMie; ///< Mie characteristic height.
	float gMie; ///< Mie g constant.
	float sunAngularRadius; ///< Sun angular radius.
	float sunAngularRadiusCos; ///< Cosine of the sun angular radius.
};

AtmosphereParameters defaultAtmosphere = AtmosphereParameters(vec3(1.474, 1.8504, 1.91198), vec3(5.5e-6, 13.0e-6, 22.4e-6), 6371e3, 6471e3, 20.0, 21e-6, 8000.0, 1200.0, 0.758, 0.04675, 0.998);

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
	\param gMie the Mie atmosphere parameter
	\return the phase
*/
float miePhase(float cosAngle, float gMie){
	const float k = 1.0/(4.0*M_PI);
	float g2 = gMie*gMie;
	return k * 3.0 * (1.0-g2) / (2.0 * (2.0 + g2)) * (1.0 + cosAngle*cosAngle) / pow(1 + g2 - 2.0 * gMie * cosAngle, 3.0/2.0);
}

/** Compute the radiance for a given ray, based on the atmosphere scattering model.
	\param rayOrigin the ray origin in ground space
	\param rayDir the ray direction
	\param sunDir the light direction
	\param scatterTable the precomputed secondary scattering lookup table
	\param params atmosphere physics parameters
	\return the estimated radiance
*/
vec3 computeAtmosphereRadiance(vec3 rayOrigin, vec3 rayDir, vec3 sunDir, sampler2D scatterTable, AtmosphereParameters params) {
	// Set the planet center to the origin.
	rayOrigin += vec3(0.0, params.groundRadius, 0.0);
	// Check intersection with atmosphere.
	vec2 interTop, interGround;
	bool didHitTop = intersects(rayOrigin, rayDir, params.topRadius, interTop);
	// If no intersection with the atmosphere, it's the dark void of space.
	if(!didHitTop){
		return vec3(0.0);
	}
	// Now intersect with the planet.
	bool didHitGround = intersects(rayOrigin, rayDir, params.groundRadius, interGround);
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
		float currHeight = length(currPos) - params.groundRadius;
		// ... there is an artifact similar to clipping when close to the planet surface if we allow for negative heights.
		if(i == SAMPLES_COUNT_ATMO-1 && currHeight < 0.0){
			currHeight = 0.0;
		}
		// Compute density based on the characteristic height of Rayleigh and Mie.
		float rayleighStep = exp(-currHeight / params.heightRayleigh) * stepSize;
		float mieStep = exp(-currHeight / params.heightMie) * stepSize;
		// Accumulate optical distances.
		rayleighDist += rayleighStep;
		mieDist += mieStep;
		
		vec3 directAttenuation = exp(-(params.kMie * (mieDist) + params.kRayleigh * (rayleighDist)));
		
		// The secondary attenuation lookup table is parametrized by
		// the height in the atmosphere, and the cosine of the vertical angle with the sun.
		float relativeHeight = (length(currPos) - params.groundRadius) / (params.topRadius - params.groundRadius);
		float relativeCosAngle = -0.5 * sunDir.y + 0.5;
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
	vec3 rayleighParticipation = params.kRayleigh * rayleighPhase(cosViewSun) * rayleighScatt;
	vec3 mieParticipation = params.kMie * miePhase(cosViewSun, params.gMie) * mieScatt;
	
	// The sun itself if we're looking at it.
	vec3 sunRadiance = vec3(0.0);
	bool didHitGroundForward = didHitGround && interGround.y > 0;
	if(!didHitGroundForward && dot(rayDir, sunDir) > params.sunAngularRadiusCos){
		sunRadiance = params.sunColor / (M_PI * params.sunAngularRadius * params.sunAngularRadius);
	}
	
	return params.sunIntensity * (rayleighParticipation + mieParticipation) + transmittance * sunRadiance;
}


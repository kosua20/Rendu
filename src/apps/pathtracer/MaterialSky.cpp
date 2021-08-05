#include "MaterialSky.hpp"
#include "resources/ResourcesManager.hpp"

const Sky::AtmosphereParameters MaterialSky::sky;

glm::vec3 MaterialSky::eval(const glm::vec3 & rayOrigin, const glm::vec3 & rayDir, const glm::vec3 & sunDir){

	// We move to the planet model space, where its center is in (0,0,0).
	const glm::vec3 planetPos = rayOrigin + glm::vec3(0.0f, sky.groundRadius, 0.0f) + glm::vec3(0.0f, 1.0f, 0.0f);
	static const Texture * scatterTable = Resources::manager().getTexture("scattering-precomputed", {Layout::RGBA32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::CPU);

	// Check intersection with atmosphere.
	glm::vec2 interTop(0.0f);
	glm::vec2 interGround(0.0f);
	const bool didHitTop = Intersection::sphere(planetPos, rayDir, sky.topRadius, interTop);
	// If no intersection with the atmosphere, it's the dark void of space.
	if(!didHitTop){
		return glm::vec3(0.0f);
	}
	// Now intersect with the planet.
	const bool didHitGround = Intersection::sphere(planetPos, rayDir, sky.groundRadius, interGround);
	// Distance to the closest intersection.
	const float distanceToInter = std::min(interTop.y, didHitGround ? interGround.x : 0.0f);
	// Divide the distance traveled through the atmosphere in samplesCount parts.
	const float stepSize = (distanceToInter - interTop.x)/float(samplesCount);
	// Angle between the sun direction and the ray.
	const float cosViewSun = glm::dot(rayDir, sunDir);

	// Accumulate optical distance for both scatterings.
	float rayleighDist = 0.0f;
	float mieDist = 0.0f;
	// Accumulate contributions for both scatterings.
	glm::vec3 rayleighScatt = glm::vec3(0.0f);
	glm::vec3 mieScatt = glm::vec3(0.0f);
	glm::vec3 transmittance = glm::vec3(0.0f);

	// March along the ray.
	for(uint i = 0; i < samplesCount; ++i){
		// Compute the current position along the ray, ...
		const glm::vec3 currPos = planetPos + (float(i)+0.5f) * stepSize * rayDir;
		// ...and its distance to the ground (as we are in planet space).
		float currHeight = glm::length(currPos) - sky.groundRadius;
		// ... there is an artifact similar to clipping when close to the planet surface if we allow for negative heights.
		if(i == (samplesCount-1) && currHeight < 0.0f){
			currHeight = 0.0f;
		}
		// Compute density based on the characteristic height of Rayleigh and Mie.
		const float rayleighStep = std::exp(-currHeight/sky.heightRayleigh) * stepSize;
		const float mieStep = std::exp(-currHeight/sky.heightMie) * stepSize;
		// Accumulate optical distances.
		rayleighDist += rayleighStep;
		mieDist += mieStep;

		const glm::vec3 directAttenuation = glm::exp(-(mieDist * glm::vec3(sky.kMie) + rayleighDist * sky.kRayleigh));

		// The secondary attenuation lookup table is parametrized by
		// the height in the atmosphere, and the cosine of the vertical angle with the sun.
		const float relativeHeight = (glm::length(currPos) - sky.groundRadius) / (sky.topRadius - sky.groundRadius);
		const float relativeCosAngle = -0.5f*sunDir.y+0.5f;
		// Compute UVs, scaled to read at the center of pixels.
		// Assume the scattering table is 512x512.
		glm::vec2 attenuationUVs = (511.0f/512.0f)*glm::vec2(relativeHeight, relativeCosAngle)+0.5f/512.0f;
		// Additional safety clamp required based on the CPU bilinear interpolation.
		attenuationUVs = glm::clamp(attenuationUVs, 0.5f/512.0f, 511.0f/512.0f);
		const glm::vec3 secondaryAttenuation = scatterTable->images[0].rgbl(attenuationUVs.x, attenuationUVs.y);

		// Final attenuation.
		const glm::vec3 attenuation = directAttenuation * secondaryAttenuation;
		// Accumulate scatterings.
		rayleighScatt += rayleighStep * attenuation;
		mieScatt += mieStep * attenuation;
		transmittance += directAttenuation;
	}

	// Final scattering participations.
	const glm::vec3 rayleighParticipation = rayleighPhase(cosViewSun) * sky.kRayleigh * rayleighScatt;
	const glm::vec3 mieParticipation = sky.kMie * miePhase(cosViewSun) * mieScatt;

	// The sun itself if we're looking at it.
	glm::vec3 sunRadiance = glm::vec3(0.0f);
	const bool didHitGroundForward = didHitGround && interGround.y > 0.0f;
	if(!didHitGroundForward && glm::dot(rayDir, sunDir) > sky.sunRadiusCos){
		sunRadiance = sky.sunColor / (glm::pi<float>() * sky.sunRadius * sky.sunRadius);
	}

	return sky.sunIntensity * (rayleighParticipation + mieParticipation) + transmittance * sunRadiance;
}

float MaterialSky::rayleighPhase(float cosAngle){
	const float k = 1.0f/(4.0f*glm::pi<float>());
	return k * 3.0f/4.0f * (1.0f + cosAngle*cosAngle);
}

float MaterialSky::miePhase(float cosAngle){
	const float k = 1.0f/(4.0f*glm::pi<float>());
	float g2 = sky.gMie*sky.gMie;
	return k * 3.0f * (1.0f-g2) / (2.0f * (2.0f + g2)) * (1.0f + cosAngle*cosAngle) / std::pow(1.0f + g2 - 2.0f * sky.gMie * cosAngle, 3.0f/2.0f);
}

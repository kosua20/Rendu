#include "MaterialGGX.hpp"
#include "system/System.hpp"
#include "generation/Random.hpp"

glm::vec3 MaterialGGX::F(glm::vec3 F0, float VdotH){
	return F0 + std::pow(1.0f - VdotH, 5.0f) * (glm::vec3(1.0f) - F0);
}

float MaterialGGX::D(float NdotH, float alpha){
	const float halfDenum = NdotH * NdotH * (alpha * alpha - 1.0f) + 1.0f;
	const float halfTerm = alpha / std::max(0.0001f, halfDenum);
	return halfTerm * halfTerm * glm::one_over_pi<float>();
}

float MaterialGGX::V(float NdotL, float NdotV, float alpha){
	// Correct version.
	const float alpha2 = alpha * alpha;
	const float visL = NdotV * std::sqrt((-NdotL * alpha2 + NdotL) * NdotL + alpha2);
	const float visV = NdotL * std::sqrt((-NdotV * alpha2 + NdotV) * NdotV + alpha2);
	return 0.5f / std::max(0.0001f, visV + visL);
}


float MaterialGGX::alphaFromRoughness(float roughness){
	const float roughClamp = std::max(0.045f, roughness);
	const float alpha = std::max(0.0001f, roughClamp*roughClamp);
	return alpha;
}

glm::vec3 MaterialGGX::GGX(const glm::vec3 & wo, const glm::vec3 & baseColor, float alpha, float metallic, const glm::vec3 & wi, float * pdf){

	const glm::vec3 h = glm::normalize(wi + wo);
	const float NdotH = std::max(h.z, 0.0f);
	const float VdotH = std::max(glm::dot(wi,h), 0.0f);
	const float NdotL = std::max(wo.z, 0.0f);
	const float NdotV = std::max(wi.z, 0.0f);

	// Evaluate D(h) separately (useful for PDF estimation).
	const float Dh = D(NdotH, alpha);
	if(pdf){
		const float hPdf = Dh * NdotH;
		*pdf = hPdf / (4.0f * std::max(0.0001f, VdotH));
	}
	// Evaluate the total BRDF and weight it.
	const glm::vec3 F0 = glm::mix(glm::vec3(0.04f), baseColor, metallic);
	const glm::vec3 specular = Dh * V(NdotL, NdotV, alpha) * F(F0, VdotH);
	const glm::vec3 diffuse = (1.0f - metallic) * glm::one_over_pi<float>() * baseColor * (1.0f - F0);
	// Multi scattering adjustment hack.
	const glm::vec3 multiAdj = glm::vec3(1.0f) + (2.0f * alpha * alpha * NdotV) * F0;
	const glm::vec3 brdf = (diffuse + specular * multiAdj) * NdotV;
	return brdf;
}

glm::vec3 MaterialGGX::sampleAndEval(const glm::vec3 & wo, const glm::vec3 & baseColor, float roughness, float metallic, glm::vec3 & wi){

	const float probaSpecular = glm::mix(1.0f / (glm::dot(baseColor, glm::vec3(1.0f)) / 3.0f + 1.0f), 1.0f,  metallic);
	const float alpha = alphaFromRoughness(roughness);

	if(Random::Float() < probaSpecular){
		// Sample specular lobe.
		const float a2 = alpha * alpha;
		const float x = Random::Float();
		// for dielectrics, Walter et al. have a roughness rescaling hack.
		// alpha * (1.2f - 0.2f * std::sqrt(std::abs(wi.z)));
		const float phiH = Random::Float() * glm::two_pi<float>();
		const float cosThetaHSqr = std::min((1.0f - x) / ((a2 - 1.0f) * x + 1.0f), 1.0f);
		const float cosThetaH = std::sqrt(cosThetaHSqr);
		const float sinThetaH = std::sqrt(1.0f - cosThetaHSqr);
		const glm::vec3 lh(sinThetaH * cos(phiH), sinThetaH * sin(phiH), cosThetaH);
		// wi is outgoing here. wo is outgoing also.
		wi = 2.0f * glm::dot(wo, lh) * lh - wo;
	} else {
		// Else sample diffuse lobe.
		wi = Random::sampleCosineHemisphere();
		if(wo.z < 0.0f){
			wi.z *= -1.0f;
		}
	}
	if(wi.z < 0.0f){
		return glm::vec3(0.0f);
	}

	float pdfSpec = 0.0f;
	const glm::vec3 brdf = GGX(wo, baseColor, alpha, metallic, wi, &pdfSpec);

	// Evaluate the total PDF.
	const float pdf = glm::mix(glm::one_over_pi<float>() * std::max(wi.z, 0.0f), pdfSpec, probaSpecular);
	if(pdf == 0.0f){
		return glm::vec3(0.0f);
	}
	return brdf / pdf;
}

glm::vec3 MaterialGGX::eval(const glm::vec3 & wo, const glm::vec3 & baseColor, float roughness, float metallic, const glm::vec3 & wi){
	if(wi.z < 0.0f){
		return glm::vec3(0.0f);
	}
	const float alpha = alphaFromRoughness(roughness);
	const glm::vec3 brdf = GGX(wo, baseColor, alpha, metallic, wi, nullptr);
	return brdf;
}

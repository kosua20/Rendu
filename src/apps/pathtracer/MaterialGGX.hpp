#pragma once
#include "scene/Scene.hpp"
#include "Common.hpp"

/**
 \brief CPU methods for evaluating the Cook-Torrance BRDF (Lambert+GGX/Towbridge-Reitz) for a
 given set of parameters, and sample a ray following the distribution of normals.
 \ingroup PathtracerDemo
 */
class MaterialGGX {
public:
	
	/** Constructor. (deleted) */
	MaterialGGX() = delete;

	/** Sample a random direction based on the shape of the BRDF diffuse and specular lobes. Both directions are expressed in the local frame and have the surface point as origin.
	 \param wo the outgoing ray direction (usually direction towards the camera)
	 \param baseColor the surface albedo (for dieletrics) or specular tint (for conductors)
	 \param roughness the linear roughness of the surface
	 \param metallic the metallicness of the surface (usually 0 or 1).
	 \param wi will contain the sampled incoming ray direction (usually direction towards a light/surface)
	 \return the BRDF evaluated for the sampled direction, weighted by its PDF
	 */
	static glm::vec3 sampleAndEval(const glm::vec3 & wo, const glm::vec3 & baseColor, float roughness, float metallic, glm::vec3 & wi);

	/** Evaluate the BRDF value for a given set of directions and parameters. Both directions are expressed in the local frame and have the surface point as origin.
	\param wo the outgoing ray direction (usually direction towards the camera)
	\param baseColor the surface albedo (for dieletrics) or specular tint (for conductors)
	\param roughness the linear roughness of the surface
	\param metallic the metallicness of the surface (usually 0 or 1)
	\param wi the incoming ray direction (usually direction towards a light/surface)
	\return the BRDF evaluated for the sampled direction.
	*/
	static glm::vec3 eval(const glm::vec3 & wo, const glm::vec3 & baseColor, float roughness, float metallic, const glm::vec3 & wi);

private:

	/** Schlick-Fresnel approximation.
	 \param F0 Fresnel coefficient at normal incidence
	 \param VdotH cosine of the angle between the view direction and the (view,light) half vector
	 \return the Fresnel coefficient at the given view incidence.
	 */
	static glm::vec3 F(glm::vec3 F0, float VdotH);

	/** Evaluate the normal distribution term.
	 \param NdotH cosine of the angle between the surface normal and the (view,light) half vector
	 \param alpha the surface perceptual roughness
	 \return the intensity based on the microfacets orientation.
	 */
	static float D(float NdotH, float alpha);

	/** Evaluate the visibility term.
	\param NdotL cosine of the angle between the view direction and the light direction
	\param NdotV cosine of the angle between the surface normal and the view direction
	\param alpha the surface perceptual roughness
	\return the intensity based on inter-shadowing of the microfacets.
	*/
	static float V(float NdotL, float NdotV, float alpha);

	/** Convert linear roughness to perceptual.
	 \param roughness the linear roughness
	 \return the perceptual roughness
	 */
	static float alphaFromRoughness(float roughness);

	/** Evaluate the specular GGX lobe BRDF.
	 \param wo the outgoing ray direction (usually direction towards the camera)
	 \param baseColor the surface albedo (for dieletrics) or specular tint (for conductors)
	 \param roughness the linear roughness of the surface
	 \param metallic the metallicness of the surface (usually 0 or 1)
	 \param wi the incoming ray direction (usually direction towards a light/surface)
	 \param pdf if non null, will contain the PDF of the incoming direction
	 \return the BRDF evaluated for the sampled direction.
	 */
	static glm::vec3 GGX(const glm::vec3 & wo, const glm::vec3 & baseColor, float alpha, float metallic, const glm::vec3 & wi, float * pdf);
};

#pragma once

#include "Common.hpp"
#include "graphics/GPUTypes.hpp"
#include "graphics/GPUObjects.hpp"


/** \brief Manage all samplers for GPU textures.
 * Samplers are shared between all shader programs, and directly specified in the shaders based on use. All samplers are stored in a unique, shared descriptor set appended to all other sets (set #3).
 * \sa GPUShaders::Common::Samplers
 * \ingroup Graphics
 */
class SamplerLibrary {
public:

	/** Initialize the samplers. */
	void init();

	/** Clean all samplers */
	void clean();

	/** \return the sampler descriptor set layout */
	VkDescriptorSetLayout getLayout() const { return _layout; }

	/** \return the sampler descriptor set */
	VkDescriptorSet getSetHandle() const { return _set.handle; }

	/** \return a basic sampler for use when displaying textures in ImGui
	 \note It is a linearly interpolated sampler with clamped UVs (sClampLinear)
	 */
	VkSampler getDefaultSampler() const { return _samplers[2]; }

private:

	/** \brief Sampler parameters */
	struct SamplerSettings {
		Filter filter; ///< Min/mag/mip filtering.
		Wrap wrapping; ///< Adress wrapping.
		bool useLods; ///< Use mip LODs.
		bool anisotropy; ///< Use anisotropy.
	};

	/** Create a sampler based on the sampling parameters.
	 \param settings the sampler configuration
	 \return the created sampler
	 */
	static VkSampler setupSampler(const SamplerSettings & settings);

	std::vector<VkSampler> _samplers; ///< Texture samplers.
	VkDescriptorSetLayout _layout; ///< Samplers descriptor set layout.
	DescriptorSet _set; ///< Samplers descriptor set allocation.
};

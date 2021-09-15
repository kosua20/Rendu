#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "graphics/SamplerLibrary.hpp"


void SamplerLibrary::init(){
	GPUContext* context = GPU::getInternal();

	// Create samplers.
	const std::vector<SamplerSettings> settings = {
		{ Filter::NEAREST_NEAREST, Wrap::CLAMP,  false, false }, // sClampNear
		{ Filter::NEAREST_NEAREST, Wrap::REPEAT, false, false }, // sRepeatNear
		{ Filter::LINEAR_NEAREST,  Wrap::CLAMP,  false, false }, // sClampLinear
		{ Filter::LINEAR_NEAREST,  Wrap::REPEAT, false, false }, // sRepeatLinear
		{ Filter::NEAREST_NEAREST, Wrap::CLAMP,  true,  true  }, // sClampNearNear
		{ Filter::NEAREST_NEAREST, Wrap::REPEAT, true,  true  }, // sRepeatNearNear
		{ Filter::LINEAR_NEAREST,  Wrap::CLAMP,  true,  true  }, // sClampLinearNear
		{ Filter::LINEAR_NEAREST,  Wrap::REPEAT, true,  true  }, // sRepeatLinearNear
		{ Filter::NEAREST_LINEAR,  Wrap::CLAMP,  true,  true  }, // sClampNearLinear
		{ Filter::NEAREST_LINEAR,  Wrap::REPEAT, true,  true  }, // sRepeatNearLinear
		{ Filter::LINEAR_LINEAR,   Wrap::CLAMP,  true,  true  }, // sClampLinearLinear
		{ Filter::LINEAR_LINEAR,   Wrap::REPEAT, true,  true  }, // sRepeatLinearLinear
	};


	for(const SamplerSettings& samplerSettings : settings){
		const VkSampler sampler = setupSampler(samplerSettings);
		_samplers.push_back(sampler);
	}

	// Create descriptor set layout.
	const size_t samplerCount = _samplers.size();

	std::vector<VkDescriptorSetLayoutBinding> bindings(samplerCount);
	for(size_t sid = 0; sid < samplerCount; ++sid){
		bindings[sid] = {};
		bindings[sid].binding = sid;
		bindings[sid].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindings[sid].descriptorCount = 1;
		bindings[sid].stageFlags = VK_SHADER_STAGE_ALL;
		bindings[sid].pImmutableSamplers = &_samplers[sid];
	}

	VkDescriptorSetLayoutCreateInfo setInfo{};
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.flags = 0;
	setInfo.pBindings = bindings.data();
	setInfo.bindingCount = uint32_t(bindings.size());

	if(vkCreateDescriptorSetLayout(context->device, &setInfo, nullptr, &_layout) != VK_SUCCESS){
		Log::Error() << Log::GPU << "Unable to create sampler set layout." << std::endl;
	}

	// Create descriptor set.
	_set = context->descriptorAllocator.allocateSet(_layout);
}


void SamplerLibrary::clean(){
	GPUContext* context = GPU::getInternal();
	context->descriptorAllocator.freeSet(_set);
	vkDestroyDescriptorSetLayout(context->device, _layout, nullptr);
	for(VkSampler& sampler : _samplers){
		vkDestroySampler(context->device, sampler, nullptr);
	}
}

VkSampler SamplerLibrary::setupSampler(const SamplerSettings& settings) {
	// Convert to Vulkan enums.
	const VkSamplerAddressMode address = VkUtils::getGPUWrapping(settings.wrapping);
	VkFilter imgFiltering; VkSamplerMipmapMode mipFiltering;
	VkUtils::getGPUFilters(settings.filter, imgFiltering, mipFiltering);

	GPUContext* context = GPU::getInternal();

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = imgFiltering;
	samplerInfo.minFilter = imgFiltering;
	samplerInfo.addressModeU = address;
	samplerInfo.addressModeV = address;
	samplerInfo.addressModeW = address;
	samplerInfo.anisotropyEnable = settings.anisotropy ? VK_TRUE: VK_FALSE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = mipFiltering;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = settings.useLods ? VK_LOD_CLAMP_NONE : 0.25f;
	// See Vulkan specification for emulation of GL_NEAREST/GL_LINEAR
	// (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkSamplerCreateInfo.html)

	VkSampler sampler = VK_NULL_HANDLE;
	if(vkCreateSampler(context->device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create a sampler." << std::endl;
	}
	return sampler;
}

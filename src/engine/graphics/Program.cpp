#include "graphics/Program.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "resources/ResourcesManager.hpp"
#include <set>
uint Program::ALL_MIPS = 0xFFFF;

#include <cstring>


Program::Program(const std::string & name, const std::string & vertexContent, const std::string & fragmentContent, const std::string & tessControlContent, const std::string & tessEvalContent) :
	_name(name), _type(Type::GRAPHICS) {
	reload(vertexContent, fragmentContent, tessControlContent, tessEvalContent);
}

Program::Program(const std::string & name, const std::string & computeContent) :
	_name(name), _type(Type::COMPUTE) {
	reload(computeContent);
}

void Program::reload(const std::string & vertexContent, const std::string & fragmentContent, const std::string & tessControlContent, const std::string & tessEvalContent) {
	if(_type != Type::GRAPHICS){
		Log::Error() << Log::GPU << _name << " is not a graphics program." << std::endl;
		return;
	}

	clean();
	_reloaded = true;
	
	const std::string debugName = _name;
	GPU::createGraphicsProgram(*this, vertexContent, fragmentContent, tessControlContent, tessEvalContent, debugName);
	reflect();
}

void Program::reload(const std::string & computeContent) {
	if(_type != Type::COMPUTE){
		Log::Error() << Log::GPU << _name << " is not a compute program." << std::endl;
		return;
	}
	clean();
	_reloaded = true;

	const std::string debugName = _name;
	GPU::createComputeProgram(*this, computeContent, debugName);
	reflect();

}

void Program::reflect(){
	// Reflection information has been populated. Merge uniform infos, build descriptor layout, prepare descriptors.

#ifdef LOG_REFLECTION

	static const std::unordered_map<Program::UniformDef::Type, std::string> typeNames = {
		{ Program::UniformDef::Type::BOOL, "BOOL" },
		{ Program::UniformDef::Type::BVEC2, "BVEC2" },
		{ Program::UniformDef::Type::BVEC3, "BVEC3" },
		{ Program::UniformDef::Type::BVEC4, "BVEC4" },
		{ Program::UniformDef::Type::INT, "INT" },
		{ Program::UniformDef::Type::IVEC2, "IVEC2" },
		{ Program::UniformDef::Type::IVEC3, "IVEC3" },
		{ Program::UniformDef::Type::IVEC4, "IVEC4" },
		{ Program::UniformDef::Type::UINT, "UINT" },
		{ Program::UniformDef::Type::UVEC2, "UVEC2" },
		{ Program::UniformDef::Type::UVEC3, "UVEC3" },
		{ Program::UniformDef::Type::UVEC4, "UVEC4" },
		{ Program::UniformDef::Type::FLOAT, "FLOAT" },
		{ Program::UniformDef::Type::VEC2, "VEC2" },
		{ Program::UniformDef::Type::VEC3, "VEC3" },
		{ Program::UniformDef::Type::VEC4, "VEC4" },
		{ Program::UniformDef::Type::MAT2, "MAT2" },
		{ Program::UniformDef::Type::MAT3, "MAT3" },
		{ Program::UniformDef::Type::MAT4, "MAT4" },
		{ Program::UniformDef::Type::OTHER, "OTHER" },
	};

	Log::Info()  << "-- Reflection: ----" << std::endl;

	uint id = 0;
	for(const auto& stage : _stages){

		Log::Info() << "Stage: " << id << std::endl;
		for(const auto& buffer : stage.buffers){
			Log::Info() << "* Buffer: (" << buffer.set << ", " << buffer.binding << ")" << std::endl;
			for(const auto& member : buffer.members){
				const auto& loc = member.locations[0];
				Log::Info() << "\t" << member.name  << " at (" << loc.set << ", " << loc.binding << ") off " << loc.offset << " of type " << typeNames.at(member.type) << std::endl;
			}
		}

		for(const auto& sampler : stage.samplers){
			Log::Info() << "* Sampler" << sampler.name << " at (" << sampler.set << ", " << sampler.binding << ")" << std::endl;
		}
		++id;
	}

#endif

	// Merge all uniforms

	for(const auto& stage : _stages){
		for(const auto& buffer : stage.buffers){
			const uint set = buffer.set;

			// We only internally manage dynamic UBOs, in set UNIFORMS_SET.
			if(set != UNIFORMS_SET){
				// The other buffer set is just initialized.
				if(set != BUFFERS_SET){
					Log::Error() << "Low frequency UBOs should be in set " << BUFFERS_SET << ", skipping." << std::endl;
					continue;
				}
				if(_staticBuffers.count(buffer.binding) != 0){
					if(_staticBuffers.at(buffer.binding).name != buffer.name){
						Log::Warning() << Log::GPU << "Program " << name() << ": Buffer already created, collision between stages for set " << buffer.set << " at binding " << buffer.binding << "." << std::endl;
						continue;
					}
				}

				_staticBuffers[buffer.binding] = StaticBufferState();
				_staticBuffers[buffer.binding].name = buffer.name;
				_staticBuffers[buffer.binding].storage = buffer.storage;
				_staticBuffers[buffer.binding].count = buffer.count;
				_staticBuffers[buffer.binding].buffers.resize(buffer.count, VK_NULL_HANDLE);
				_staticBuffers[buffer.binding].offsets.resize(buffer.count, 0);
				continue;
			}


			if(_dynamicBuffers.count(buffer.binding) != 0){
				Log::Warning() << Log::GPU << "Program " << name() << ": Buffer already created, collision between stages for set " << buffer.set << " at binding " << buffer.binding << "." << std::endl;
				continue;
			}

			_dynamicBuffers[buffer.binding].buffer.reset(new UniformBuffer<char>(buffer.size, UniformFrequency::DYNAMIC));
			_dynamicBuffers[buffer.binding].dirty = true;
			
			// Add uniforms to look-up table.
			for(const auto& uniform : buffer.members){
				auto uniDef = _uniforms.find(uniform.name);
				if(uniDef == _uniforms.end()){
					_uniforms[uniform.name] = uniform;
				} else {
					uniDef->second.locations.emplace_back(uniform.locations[0]);
				}
			}
		}

		for(const auto& image : stage.images){
			const uint set = image.set;

			if(set != IMAGES_SET){
				Log::Error() << "Program " << name() << ": Image should be in set " << IMAGES_SET << " only, ignoring." << std::endl;
				continue;
			}

			if(_textures.count(image.binding) != 0){
				if(_textures.at(image.binding).name != image.name){
					Log::Warning() << Log::GPU << "Program " << name() << ": Image already created, collision between stages for set " << image.set << " at binding " << image.binding << "." << std::endl;
					continue;
				}
			}
			const Texture* defaultTex = Resources::manager().getDefaultTexture(image.shape);

			_textures[image.binding] = TextureState();
			_textures[image.binding].name = image.name;
			_textures[image.binding].count = image.count;
			_textures[image.binding].shape = image.shape;
			_textures[image.binding].storage = image.storage;
			_textures[image.binding].textures.resize(image.count);
			_textures[image.binding].views.resize(image.count);
			for(uint tid = 0; tid < image.count; ++tid){
				_textures[image.binding].textures[tid] = defaultTex;
				_textures[image.binding].views[tid] = _textures[image.binding].textures[tid]->gpu->view;
			}
			_textures[image.binding].mip = Program::ALL_MIPS;
		}
	}

	GPUContext* context = GPU::getInternal();

	_dirtySets.fill(false);
	if(!_dynamicBuffers.empty()){
		_dirtySets[UNIFORMS_SET] = true;
	}
	if(!_textures.empty()){
		_dirtySets[IMAGES_SET] = true;
	}

	_state.setLayouts.resize(_currentSets.size());

	// Basic uniforms buffer descriptors will use a dynamic offset.
	uint maxDescriptorCount = 0;
	{
		std::vector<VkDescriptorSetLayoutBinding> bindingLayouts;
		for(const auto& buffer : _dynamicBuffers){
			VkDescriptorSetLayoutBinding bufferBinding{};
			bufferBinding.binding = buffer.first;
			bufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			bufferBinding.descriptorCount = 1;
			bufferBinding.stageFlags = VK_SHADER_STAGE_ALL;
			bindingLayouts.emplace_back(bufferBinding);
			maxDescriptorCount = std::max(uint(buffer.first) + 1u, maxDescriptorCount);
		}

		VkDescriptorSetLayoutCreateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.flags = 0;
		setInfo.pBindings = bindingLayouts.data();
		setInfo.bindingCount = uint32_t(bindingLayouts.size());

		if(vkCreateDescriptorSetLayout(context->device, &setInfo, nullptr, &_state.setLayouts[UNIFORMS_SET]) != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create set layout." << std::endl;
		}
	}

	// Textures.
	{
		std::vector<VkDescriptorSetLayoutBinding> bindingLayouts;
		for(const auto& image : _textures){
			VkDescriptorSetLayoutBinding imageBinding{};
			imageBinding.binding = image.first;
			imageBinding.descriptorType = image.second.storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			imageBinding.descriptorCount = image.second.count;
			imageBinding.stageFlags = VK_SHADER_STAGE_ALL;
			bindingLayouts.emplace_back(imageBinding);
		}

		VkDescriptorSetLayoutCreateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.flags = 0;
		setInfo.pBindings = bindingLayouts.data();
		setInfo.bindingCount = uint32_t(bindingLayouts.size());

		if(vkCreateDescriptorSetLayout(context->device, &setInfo, nullptr, &_state.setLayouts[IMAGES_SET]) != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create set layout." << std::endl;
		}
	}

	// Static buffers.
	{
		std::vector<VkDescriptorSetLayoutBinding> bindingLayouts;
		for(const auto& buffer : _staticBuffers){
			VkDescriptorSetLayoutBinding bufferBinding{};
			bufferBinding.binding = buffer.first;
			bufferBinding.descriptorType = buffer.second.storage ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bufferBinding.descriptorCount = buffer.second.count;
			bufferBinding.stageFlags = VK_SHADER_STAGE_ALL;
			bindingLayouts.emplace_back(bufferBinding);
		}

		VkDescriptorSetLayoutCreateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.flags = 0;
		setInfo.pBindings = bindingLayouts.data();
		setInfo.bindingCount = uint32_t(bindingLayouts.size());

		if(vkCreateDescriptorSetLayout(context->device, &setInfo, nullptr, &_state.setLayouts[BUFFERS_SET]) != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create set layout." << std::endl;
		}
	}

	// Samplers
	{
		_state.setLayouts[SAMPLERS_SET] = context->samplerLibrary.getLayout();
	}

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = uint32_t(_state.setLayouts.size());
	layoutInfo.pSetLayouts = _state.setLayouts.data();
	layoutInfo.pushConstantRangeCount = 0;
	layoutInfo.pPushConstantRanges = nullptr;
	if(vkCreatePipelineLayout(context->device, &layoutInfo, nullptr, &_state.layout) != VK_SUCCESS){
		Log::Error() << Log::GPU << "Unable to create pipeline layout." << std::endl;
	}

	// Initialize dynamic UBO descriptors.
	_currentOffsets.assign(_dynamicBuffers.size(), 0);
	// We need to store the offsets in order.
	uint currentIndex = 0;
	for(int did = 0; did < int(maxDescriptorCount); ++did){
		auto desc = _dynamicBuffers.find(did);
		if(desc == _dynamicBuffers.end()){
			continue;
		}
		desc->second.descriptorIndex = currentIndex;
		++currentIndex;
	}
	// Dynamic uniforms are allocated only once.
	context->descriptorAllocator.freeSet(_currentSets[UNIFORMS_SET]);
	_currentSets[UNIFORMS_SET] = context->descriptorAllocator.allocateSet(_state.setLayouts[UNIFORMS_SET]);

	std::vector<VkDescriptorBufferInfo> infos(_dynamicBuffers.size());
	std::vector<VkWriteDescriptorSet> writes;
	uint tid = 0;
	for(const auto& buffer : _dynamicBuffers){
		infos[tid] = {};
		infos[tid].buffer = buffer.second.buffer->gpu->buffer;
		infos[tid].offset = 0;
		infos[tid].range = buffer.second.buffer->baseSize();

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = _currentSets[UNIFORMS_SET].handle;
		write.dstBinding = buffer.first;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		write.pBufferInfo = &infos[tid];
		writes.push_back(write);
		++tid;
	}
	vkUpdateDescriptorSets(context->device, uint32_t(writes.size()), writes.data(), 0, nullptr);

}

void Program::transitionResourcesTo(Program::Type type){
	VkCommandBuffer& commandBuffer = GPU::getInternal()->getRenderCommandBuffer();

	if(type == Type::GRAPHICS){
		// We need to ensure all images are in the shader read only optimal layout,
		// and that all writes to buffers are complete, whether we will use them
		// as index/verte/uniform/storage buffers.
		
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		const VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dstStage, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr );

		// Move all textures to shader read only optimal.
		for(const auto& texInfos : _textures){
			// Transition proper subresource.
			const uint mip = texInfos.second.mip;
			const VkImageLayout tgtLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			for(const Texture* tex : texInfos.second.textures){
				if(mip == Program::ALL_MIPS){
					VkUtils::textureLayoutBarrier(commandBuffer, *tex, tgtLayout);
				} else {
					VkUtils::mipLayoutBarrier(commandBuffer, *tex, tgtLayout, mip);
				}
			}
		}
	} else if(type == Type::COMPUTE){

		// For buffers, two possible cases.
		// Either it was last used in a graphics program, and can't have been modified.
		// Or it was used in a previous compute pass, and a memory barrier should be enough.
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT ;
		const VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dstStage, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr );

		// Move sampled textures to shader read only optimal, and storage to general layout.
		// \warning We might be missing some masks in the layout barrier when moving from a compute to a compute, investigate.
		for(const auto& texInfos : _textures){
			// Transition proper subresource.
			const uint mip = texInfos.second.mip;
			const VkImageLayout tgtLayout = texInfos.second.storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			for(const Texture* tex : texInfos.second.textures){
				if(mip == Program::ALL_MIPS){
					VkUtils::textureLayoutBarrier(commandBuffer, *tex, tgtLayout);
				} else {
					VkUtils::mipLayoutBarrier(commandBuffer, *tex, tgtLayout, mip);
				}
			}
		}

	}

}

void Program::update(){
	GPUContext* context = GPU::getInternal();

	// Upload all dirty uniform buffers, and the offsets.
	if(_dirtySets[UNIFORMS_SET]){
		for(const auto& buffer : _dynamicBuffers){
			if(buffer.second.dirty){
				buffer.second.buffer->upload();
			}
			_currentOffsets[buffer.second.descriptorIndex] = uint32_t(buffer.second.buffer->currentOffset());
		}
		_dirtySets[UNIFORMS_SET] = false;
	}

	// Update the texture descriptors
	if(_dirtySets[IMAGES_SET]){

		// We can't just update the current descriptor set as it might be in use.
		context->descriptorAllocator.freeSet(_currentSets[IMAGES_SET]);
		_currentSets[IMAGES_SET] = context->descriptorAllocator.allocateSet(_state.setLayouts[IMAGES_SET]);

		std::vector<std::vector<VkDescriptorImageInfo>> imageInfos(_textures.size());
		std::vector<VkWriteDescriptorSet> writes;
		uint tid = 0;
		for(const auto& image : _textures){
			imageInfos[tid].resize(image.second.count);
			const VkImageLayout tgtLayout = image.second.storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			for(uint did = 0; did < image.second.count; ++did){
				imageInfos[tid][did].imageView = image.second.views[did];
				imageInfos[tid][did].imageLayout = tgtLayout;
			}

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = _currentSets[IMAGES_SET].handle;
			write.dstBinding = image.first;
			write.dstArrayElement = 0;
			write.descriptorCount = image.second.count;
			write.descriptorType = image.second.storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageInfo = &imageInfos[tid][0];
			writes.push_back(write);
			++tid;
		}

		vkUpdateDescriptorSets(context->device, uint32_t(writes.size()), writes.data(), 0, nullptr);
		_dirtySets[IMAGES_SET] = false;
	}

	// Update static buffer descriptors.
	if(_dirtySets[BUFFERS_SET]){
		// We can't just update the current descriptor set as it might be in use.
		context->descriptorAllocator.freeSet(_currentSets[BUFFERS_SET]);
		_currentSets[BUFFERS_SET] = context->descriptorAllocator.allocateSet(_state.setLayouts[BUFFERS_SET]);

		std::vector<std::vector<VkDescriptorBufferInfo>> infos(_staticBuffers.size());
		std::vector<VkWriteDescriptorSet> writes;
		uint tid = 0;
		for(const auto& buffer : _staticBuffers){
			infos[tid].resize(buffer.second.count);
			/// \todo Should we use the real buffer current offset here, if an update happened under the hood more than once per frame?
			for(uint did = 0; did < buffer.second.count; ++did){
				VkBuffer rawBuffer = buffer.second.buffers[did];
				if(rawBuffer != VK_NULL_HANDLE){
					infos[tid][did].buffer = rawBuffer;
					infos[tid][did].offset = buffer.second.offsets[did];
				} else {
					infos[tid][did].buffer = buffer.second.buffers[buffer.second.lastSet];
					infos[tid][did].offset = buffer.second.offsets[buffer.second.lastSet];
				}

				infos[tid][did].range = buffer.second.size;
			}
			
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = _currentSets[BUFFERS_SET].handle;
			write.dstBinding = buffer.first;
			write.dstArrayElement = 0;
			write.descriptorCount = buffer.second.count;
			write.descriptorType = buffer.second.storage ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.pBufferInfo = &infos[tid][0];
			writes.push_back(write);
			++tid;
		}

		vkUpdateDescriptorSets(context->device, uint32_t(writes.size()), writes.data(), 0, nullptr);
		_dirtySets[BUFFERS_SET] = false;
	}

	// Bind the descriptor sets.
	const VkPipelineBindPoint bindPoint = _type == Type::COMPUTE ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
	VkCommandBuffer& commandBuffer = context->getRenderCommandBuffer();

	// Set UNIFORMS_SET needs updated offsets.
	vkCmdBindDescriptorSets(commandBuffer, bindPoint, _state.layout, UNIFORMS_SET, 1, &_currentSets[UNIFORMS_SET].handle, uint32_t(_currentOffsets.size()), _currentOffsets.data());

	// Bind static samplers dummy set SAMPLERS_SET.
	const VkDescriptorSet samplersHandle = context->samplerLibrary.getSetHandle();
	vkCmdBindDescriptorSets(commandBuffer, bindPoint, _state.layout, SAMPLERS_SET, 1, &samplersHandle, 0, nullptr);

	// Other sets are bound if present.
	if(_currentSets[IMAGES_SET].handle != VK_NULL_HANDLE){
		vkCmdBindDescriptorSets(commandBuffer, bindPoint, _state.layout, IMAGES_SET, 1, &_currentSets[IMAGES_SET].handle, 0, nullptr);
	}
	if(_currentSets[BUFFERS_SET].handle != VK_NULL_HANDLE){
		vkCmdBindDescriptorSets(commandBuffer, bindPoint, _state.layout, BUFFERS_SET, 1, &_currentSets[BUFFERS_SET].handle, 0, nullptr);
	}

}

bool Program::reloaded() const {
	return _reloaded;
}

bool Program::reloaded(bool absorb){
	const bool wasReloaded = _reloaded;
	if(absorb){
		_reloaded = false;
	}
	return wasReloaded;
}

void Program::use() const {
	GPU::bindProgram(*this);
}

void Program::clean() {
	
	GPU::clean(*this);
	// Clear CPU infos.
	_uniforms.clear();
	_textures.clear();
	_dynamicBuffers.clear();
	_staticBuffers.clear();
	_state.setLayouts.clear();
	_state.layout = VK_NULL_HANDLE;
	_dirtySets.fill(false);

	for(uint i = 0; i < _currentSets.size(); ++i){
		// Skip the shared static samplers set.
		if(i == SAMPLERS_SET){
			continue;
		}
		GPU::getInternal()->descriptorAllocator.freeSet(_currentSets[i]);
		_currentSets[i].handle = VK_NULL_HANDLE;
		_currentSets[i].pool = 0;
	}
	_currentOffsets.clear();
}

void Program::buffer(const UniformBufferBase& buffer, uint slot){
	auto existingBuff = _staticBuffers.find(slot);
	if(existingBuff != _staticBuffers.end()) {
		StaticBufferState& refBuff = existingBuff->second;
		assert(refBuff.count == 1);
		if((refBuff.buffers[0] != buffer.gpu->buffer) || (refBuff.offsets[0] != buffer.currentOffset()) || (refBuff.size != buffer.baseSize())){
			refBuff.buffers[0] = buffer.gpu->buffer;
			refBuff.offsets[0] = uint(buffer.currentOffset());
			refBuff.size = uint(buffer.baseSize());
			_dirtySets[BUFFERS_SET] = true;
		}
	}
}

void Program::buffer(const Buffer& buffer, uint slot){
	auto existingBuff = _staticBuffers.find(slot);
	if(existingBuff != _staticBuffers.end()) {
		StaticBufferState& refBuff = existingBuff->second;
		assert(refBuff.count == 1);
		if((refBuff.buffers[0] != buffer.gpu->buffer) || (refBuff.size != buffer.size)){
			refBuff.buffers[0] = buffer.gpu->buffer;
			refBuff.offsets[0] = 0;
			refBuff.size = uint(buffer.size);
			_dirtySets[BUFFERS_SET] = true;
		}
	}
}

void Program::bufferArray(const std::vector<const Buffer * >& buffers, uint slot){
	auto existingBuff = _staticBuffers.find(slot);
	if(existingBuff != _staticBuffers.end()) {
		StaticBufferState& refBuff = existingBuff->second;
		const uint buffCount = buffers.size();
		assert(buffCount <= refBuff.count);

		for(uint did = 0; did < buffCount; ++did){
			const Buffer& buffer = *buffers[did];
			if((refBuff.buffers[did] != buffer.gpu->buffer) || (refBuff.size != buffer.size)){
				refBuff.buffers[did] = buffer.gpu->buffer;
				refBuff.offsets[did] = 0;
				refBuff.size = uint(buffer.size);
				refBuff.lastSet = did;
				_dirtySets[BUFFERS_SET] = true;
			}
		}
	}
}

void Program::texture(const Texture& texture, uint slot, uint mip){
	auto existingTex = _textures.find(slot);
	if(existingTex != _textures.end()) {
		TextureState & refTex = existingTex->second;
		assert(refTex.count == 1);
		// Find the view we need.
		assert(mip == Program::ALL_MIPS || mip < texture.gpu->levelViews.size());
		VkImageView& view = mip == Program::ALL_MIPS ? texture.gpu->view : texture.gpu->levelViews[mip];

		if(refTex.views[0] != view){
			refTex.textures[0] = &texture;
			refTex.views[0] = view;
			refTex.mip = mip;
			_dirtySets[IMAGES_SET] = true;
		}
	}
}

void Program::textureArray(const std::vector<const Texture *> & textures, uint slot, uint mip){
	auto existingTex = _textures.find(slot);
	if(existingTex != _textures.end()) {
		TextureState & refTex = existingTex->second;
		const uint texCount = textures.size();
		assert(texCount <= refTex.count);

		for(uint did = 0; did < texCount; ++did){
			// Find the view we need.
			assert(mip == Program::ALL_MIPS || mip < textures[did]->gpu->levelViews.size());

			VkImageView& view = mip == Program::ALL_MIPS ? textures[did]->gpu->view : textures[did]->gpu->levelViews[mip];
			if(refTex.views[did] != view){
				refTex.textures[did] = textures[did];
				refTex.views[did] = view;
				refTex.mip = mip;
				_dirtySets[IMAGES_SET] = true;
			}
		}
	}
}

void Program::texture(const Texture* texture, uint slot, uint mip){
	Program::texture(*texture, slot, mip);
}

void Program::defaultTexture(uint slot){
	auto existingTex = _textures.find(slot);
	if(existingTex != _textures.end()) {
		// Reset the texture by binding the default one with the appropriate shape.
		const Texture * defaultTex = Resources::manager().getDefaultTexture(existingTex->second.shape);
		if(existingTex->second.count == 1){
			texture(defaultTex, slot);
		} else {
			std::vector<const Texture *> texs(existingTex->second.count, defaultTex);
			textureArray(texs, slot);
		}
	}
}

void Program::textures(const std::vector<const Texture *> & textures, size_t startingSlot){
	const uint texCount = uint(textures.size());
	for(uint tid = 0; tid < texCount; ++tid){
		const uint slot = uint(startingSlot) + tid;
		texture(*textures[tid], slot);
	}
}

void Program::uniform(const std::string & name, bool t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const int val = t ? 1 : 0;
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &val, sizeof(int));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, int t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t, sizeof(int));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, uint t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t, sizeof(uint));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, float t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t, sizeof(float));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::vec2 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::vec2));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::vec3 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::vec3));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::vec4 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::vec4));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::ivec2 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::ivec2));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::ivec3 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::ivec3));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::ivec4 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::ivec4));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::mat3 & t) {
	Log::Warning() << "Deprecated due to alignment issues." << std::endl;
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0][0], sizeof(glm::mat3));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::mat4 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0][0], sizeof(glm::mat4));
		}
		updateUniformMetric();
	}
}

void Program::getUniform(const std::string & name, bool & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		int val = *reinterpret_cast<const int*>(retrieveUniform(loc));
		t = bool(val);
	}
}

void Program::getUniform(const std::string & name, int & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const int*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, uint & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const uint*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, float & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const float*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, glm::vec2 & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const glm::vec2*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, glm::vec3 & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const glm::vec3*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, glm::vec4 & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const glm::vec4*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, glm::ivec2 & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const glm::ivec2*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, glm::ivec3 & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const glm::ivec3*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, glm::ivec4 & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const glm::ivec4*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, glm::mat3 & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const glm::mat3*>(retrieveUniform(loc));
	}
}

void Program::getUniform(const std::string & name, glm::mat4 & t) const {
	const auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const UniformDef::Location& loc = uni->second.locations[0];
		t = *reinterpret_cast<const glm::mat4*>(retrieveUniform(loc));
	}
}

const glm::uvec3 & Program::size() const {
	return _stages[uint(ShaderType::COMPUTE)].size;
}

void Program::Stage::reset(){
	images.clear();
	buffers.clear();
	module = VK_NULL_HANDLE;
}

void Program::updateUniformMetric() const {
#define UDPATE_METRICS
#ifdef UDPATE_METRICS
	//GPU::_metrics.uniforms += 1;
#endif
}

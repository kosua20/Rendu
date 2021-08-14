#include "graphics/Program.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "resources/ResourcesManager.hpp"
#include <set>


Program::Program(const std::string & name, const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent) : _name(name) {
	reload(vertexContent, fragmentContent, geometryContent, tessControlContent, tessEvalContent);
}

void Program::reload(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent) {

	clean();
	_reloaded = true;
	
	const std::string debugName = _name;
	GPU::createProgram(*this, vertexContent, fragmentContent, geometryContent, tessControlContent, tessEvalContent, debugName);

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

			// We only internally manage dynamic UBOs, in set 0.
			if(set != 0){
				// Other sets are just initialized.
				if(set != 2){
					Log::Error() << "Low frequency UBOs should be in set 2, skipping." << std::endl;
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
				continue;
			}


			if(_dynamicBuffers.count(buffer.binding) != 0){
				Log::Warning() << Log::GPU << "Program " << name() << ": Buffer already created, collision between stages for set " << buffer.set << " at binding " << buffer.binding << "." << std::endl;
				continue;
			}

			_dynamicBuffers[buffer.binding].buffer.reset(new UniformBuffer<char>(buffer.size, DataUse::DYNAMIC));
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

		for(const auto& image : stage.samplers){
			const uint set = image.set;

			if(set != 1){
				Log::Error() << "Program " << name() << ": Sampler image should be in set 1 only, ignoring." << std::endl;
				continue;
			}

			if(_textures.count(image.binding) != 0){
				if(_textures.at(image.binding).name != image.name){
					Log::Warning() << Log::GPU << "Program " << name() << ": Sampler image already created, collision between stages for set " << image.set << " at binding " << image.binding << "." << std::endl;
					continue;
				}
			}
			const Texture* defaultTex = Resources::manager().getDefaultTexture(image.shape);

			_textures[image.binding] = TextureState();
			_textures[image.binding].name = image.name;
			_textures[image.binding].view = defaultTex->gpu->view;
			_textures[image.binding].sampler = defaultTex->gpu->sampler;
			_textures[image.binding].shape = image.shape;
		}
	}

	GPUContext* context = GPU::getInternal();

	_dirtySets.fill(false);
	if(!_dynamicBuffers.empty()){
		_dirtySets[0] = true;
	}
	if(!_textures.empty()){
		_dirtySets[1] = true;
	}
	_state.setLayouts.resize(_dirtySets.size());

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
		setInfo.bindingCount = bindingLayouts.size();

		if(vkCreateDescriptorSetLayout(context->device, &setInfo, nullptr, &_state.setLayouts[0]) != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create set layout." << std::endl;
		}
	}

	// Texture and samplers.
	{
		std::vector<VkDescriptorSetLayoutBinding> bindingLayouts;
		for(const auto& image : _textures){
			VkDescriptorSetLayoutBinding imageBinding{};
			imageBinding.binding = image.first;
			imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageBinding.descriptorCount = 1;
			imageBinding.stageFlags = VK_SHADER_STAGE_ALL;
			bindingLayouts.emplace_back(imageBinding);
		}

		VkDescriptorSetLayoutCreateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.flags = 0;
		setInfo.pBindings = bindingLayouts.data();
		setInfo.bindingCount = bindingLayouts.size();

		if(vkCreateDescriptorSetLayout(context->device, &setInfo, nullptr, &_state.setLayouts[1]) != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create set layout." << std::endl;
		}
	}

	// Static buffers.
	{
		std::vector<VkDescriptorSetLayoutBinding> bindingLayouts;
		for(const auto& buffer : _staticBuffers){
			VkDescriptorSetLayoutBinding bufferBinding{};
			bufferBinding.binding = buffer.first;
			bufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bufferBinding.descriptorCount = 1;
			bufferBinding.stageFlags = VK_SHADER_STAGE_ALL;
			bindingLayouts.emplace_back(bufferBinding);
		}

		VkDescriptorSetLayoutCreateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.flags = 0;
		setInfo.pBindings = bindingLayouts.data();
		setInfo.bindingCount = bindingLayouts.size();

		if(vkCreateDescriptorSetLayout(context->device, &setInfo, nullptr, &_state.setLayouts[2]) != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create set layout." << std::endl;
		}
	}

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = _state.setLayouts.size();
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

	context->descriptorAllocator.freeSet(_currentSets[0]);
	_currentSets[0] = context->descriptorAllocator.allocateSet(_state.setLayouts[0]);

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
		write.dstSet = _currentSets[0].handle;
		write.dstBinding = buffer.first;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		write.pBufferInfo = &infos[tid];
		writes.push_back(write);
		++tid;
	}
	vkUpdateDescriptorSets(context->device, writes.size(), writes.data(), 0, nullptr);

}


void Program::validate() const {

}

void Program::saveBinary(const std::string & ) const {
	//Resources::saveRawDataToExternalFile(outputPath + "_" + _name + "_" + std::to_string(uint(format)) + ".bin", &binary[0], binary.size());
}

void Program::update(){
	GPUContext* context = GPU::getInternal();

	// Upload all dirty uniform buffers, and the offsets.
	if(_dirtySets[0]){
		for(const auto& buffer : _dynamicBuffers){
			if(buffer.second.dirty){
				buffer.second.buffer->upload();
			}
			_currentOffsets[buffer.second.descriptorIndex] = buffer.second.buffer->currentOffset();
		}
		_dirtySets[0] = false;
	}

	// Update the texture descriptors
	if(_dirtySets[1]){

		// We can't just update the current descriptor set as it might be in use.
		context->descriptorAllocator.freeSet(_currentSets[1]);
		_currentSets[1] = context->descriptorAllocator.allocateSet(_state.setLayouts[1]);

		std::vector< VkDescriptorImageInfo> imageInfos(_textures.size());
		std::vector< VkWriteDescriptorSet> writes;
		uint tid = 0;
		for(const auto& image : _textures){
			imageInfos[tid] = {};
			imageInfos[tid].imageView = image.second.view;
			imageInfos[tid].sampler = image.second.sampler;
			imageInfos[tid].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = _currentSets[1].handle;
			write.dstBinding = image.first;
			write.dstArrayElement = 0;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.pImageInfo = &imageInfos[tid];
			writes.push_back(write);
			++tid;
		}

		vkUpdateDescriptorSets(context->device, writes.size(), writes.data(), 0, nullptr);
		_dirtySets[1] = false;
	}

	// Update static buffer descriptors.
	if(_dirtySets[2]){
		// We can't just update the current descriptor set as it might be in use.
		context->descriptorAllocator.freeSet(_currentSets[2]);
		_currentSets[2] = context->descriptorAllocator.allocateSet(_state.setLayouts[2]);

		std::vector<VkDescriptorBufferInfo> infos(_staticBuffers.size());
		std::vector<VkWriteDescriptorSet> writes;
		uint tid = 0;
		for(const auto& buffer : _staticBuffers){
			infos[tid] = {};
			// \todo Should we use the real buffer current offset here, if an update happened under the hood ?
			infos[tid].buffer = buffer.second.buffer;
			infos[tid].offset = buffer.second.offset;
			infos[tid].range = buffer.second.size;
			
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = _currentSets[2].handle;
			write.dstBinding = buffer.first;
			write.dstArrayElement = 0;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.pBufferInfo = &infos[tid];
			writes.push_back(write);
			++tid;
		}

		vkUpdateDescriptorSets(context->device, writes.size(), writes.data(), 0, nullptr);
		_dirtySets[2] = false;
	}

	// Bind the descriptor sets.
	
	// Set 0 needs updated offsets.
	vkCmdBindDescriptorSets(context->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _state.layout, 0, 1, &_currentSets[0].handle, _currentOffsets.size(), _currentOffsets.data());

	// Other sets are bound if present.
	for(uint sid = 1; sid < _currentSets.size(); ++sid){
		if(_currentSets[sid].handle != VK_NULL_HANDLE){
			vkCmdBindDescriptorSets(context->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _state.layout, sid, 1, &_currentSets[sid].handle, 0, nullptr);
		}
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
		GPU::getInternal()->descriptorAllocator.freeSet(_currentSets[i]);
		_currentSets[i].handle = VK_NULL_HANDLE;
		_currentSets[i].pool = 0;
	}
	_currentOffsets.clear();
}

void Program::buffer(const UniformBufferBase& buffer, uint slot){
	auto existingBuff = _staticBuffers.find(slot);
	if(existingBuff != _staticBuffers.end()) {
		const StaticBufferState& refBuff = existingBuff->second;
		if((refBuff.buffer != buffer.gpu->buffer) || (refBuff.offset != buffer.currentOffset()) || (refBuff.size != buffer.baseSize())){
			_staticBuffers[slot].buffer = buffer.gpu->buffer;
			_staticBuffers[slot].offset = buffer.currentOffset();
			_staticBuffers[slot].size = buffer.baseSize();
			_dirtySets[2] = true;
		}
	}
}


void Program::texture(const Texture& texture, uint slot, uint mip){
	auto existingTex = _textures.find(slot);
	if(existingTex != _textures.end()) {
		const TextureState & refTex = existingTex->second;
		// Find the view we need.
		assert(mip == 0xFFFF || mip < texture.gpu->levelViews.size());
		VkImageView& view = mip == 0xFFFF ? texture.gpu->view : texture.gpu->levelViews[mip];

		if((refTex.view != view) || (refTex.sampler != texture.gpu->sampler)){
			_textures[slot].view = view;
			_textures[slot].sampler = texture.gpu->sampler;
			_dirtySets[1] = true;
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
		texture(Resources::manager().getDefaultTexture(existingTex->second.shape), slot);
	}
}

void Program::textures(const std::vector<const Texture *> & textures, size_t startingSlot){
	const uint texCount = uint(textures.size());
	for(uint tid = 0; tid < texCount; ++tid){
		const uint slot = startingSlot + tid;
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

/*void Program::uniform(const std::string & name, size_t count, const float * t) {
	// The name (including "[0]") should be present in the list.
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			char* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, t, sizeof(float) * count);
		}
		updateUniformMetric();
	}
}*/

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

void Program::updateUniformMetric() const {
#define UDPATE_METRICS
#ifdef UDPATE_METRICS
	//GPU::_metrics.uniforms += 1;
#endif
}

#include "graphics/Program.hpp"
#include "graphics/GPU.hpp"
#include "resources/ResourcesManager.hpp"




Program::Program(const std::string & name, const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent) : _name(name) {
	reload(vertexContent, fragmentContent, geometryContent, tessControlContent, tessEvalContent);
}

void Program::reload(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent) {

	clean();
	_reloaded = true;
	
	const std::string debugName = _name;
	GPU::createProgram(*this, vertexContent, fragmentContent, geometryContent, tessControlContent, tessEvalContent, debugName);

	// Reflection information has been populated. Merge uniform infos, build descriptor layout, prepare descriptors.
#define LOG_REFLECTION
#ifdef LOG_REFLECTION

	static const std::map<Program::UniformDef::Type, std::string> typeNames = {
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

			// Allocate a CPU buffer with the proper size.
			if(_buffers.count(buffer.set) == 0){
				_buffers[buffer.set] = {};
			}
			auto& setMap = _buffers.at(buffer.set);

			if(setMap.count(buffer.binding) != 0){
				Log::Warning() << Log::GPU << "Buffer already created, collision between stages." << std::endl;
				continue;
			}
			setMap[buffer.binding].resize(buffer.size);
			
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
	}
	
	// Build state for the pipeline state objects.
	_state.stages.clear();

	static const std::map<ShaderType, VkShaderStageFlagBits> stageBits = {
		{ShaderType::VERTEX, VK_SHADER_STAGE_VERTEX_BIT},
		{ShaderType::GEOMETRY, VK_SHADER_STAGE_GEOMETRY_BIT},
		{ShaderType::FRAGMENT, VK_SHADER_STAGE_FRAGMENT_BIT},
		{ShaderType::TESSCONTROL, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
		{ShaderType::TESSEVAL, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
	};
	for(uint sid = 0; sid < uint(ShaderType::COUNT); ++sid){
		const ShaderType type = ShaderType(sid);
		const VkShaderModule& module = stage(type).module;
		if(module == VK_NULL_HANDLE){
			continue;
		}
		_state.stages.emplace_back();
		_state.stages.back().stage = stageBits.at(type);
		_state.stages.back().module = module;
	}

	for(auto& stage : _state.stages){
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.pName = "main";
	}

}

void Program::validate() const {

}

void Program::saveBinary(const std::string & outputPath) const {

	//Resources::saveRawDataToExternalFile(outputPath + "_" + _name + "_" + std::to_string(uint(format)) + ".bin", &binary[0], binary.size());
}

void Program::use() const {
	GPU::bindProgram(*this);
}

void Program::clean() {
	GPU::clean(*this);
}

void Program::uniform(const std::string & name, bool t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		const int val = t ? 1 : 0;
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &val, sizeof(int));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, int t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t, sizeof(int));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, uint t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t, sizeof(uint));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, float t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t, sizeof(float));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, size_t count, const float * t) {
	// The name (including "[0]") should be present in the list.
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, t, sizeof(float) * count);
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::vec2 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::vec2));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::vec3 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::vec3));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::vec4 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::vec4));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::ivec2 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::ivec2));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::ivec3 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::ivec3));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::ivec4 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0], sizeof(glm::ivec4));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::mat3 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
			std::memcpy(dst, &t[0][0], sizeof(glm::mat3));
		}
		updateUniformMetric();
	}
}

void Program::uniform(const std::string & name, const glm::mat4 & t) {
	auto uni = _uniforms.find(name);
	if(uni != _uniforms.end()) {
		for(const UniformDef::Location& loc : uni->second.locations){
			uchar* dst = retrieveUniformNonConst(loc);
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

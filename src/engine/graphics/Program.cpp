#include "graphics/Program.hpp"
#include "graphics/GPU.hpp"
#include "resources/ResourcesManager.hpp"




Program::Program(const std::string & name, const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent) : _name(name) {
	reload(vertexContent, fragmentContent, geometryContent, tessControlContent, tessEvalContent);
}

void Program::reload(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent) {

	GPU::clean(*this);

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

void Program::uniform(const std::string & name, bool t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform1i(_uniforms.at(name), int(t));
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, int t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform1i(_uniforms.at(name), t);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, uint t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform1ui(_uniforms.at(name), t);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, float t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform1f(_uniforms.at(name), t);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, size_t count, const float * t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform1fv(_uniforms.at(name), GLsizei(count), t);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, const glm::vec2 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform2fv(_uniforms.at(name), 1, &t[0]);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, const glm::vec3 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform3fv(_uniforms.at(name), 1, &t[0]);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, const glm::vec4 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform4fv(_uniforms.at(name), 1, &t[0]);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, const glm::ivec2 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform2iv(_uniforms.at(name), 1, &t[0]);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, const glm::ivec3 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform3iv(_uniforms.at(name), 1, &t[0]);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, const glm::ivec4 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform4iv(_uniforms.at(name), 1, &t[0]);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, const glm::mat3 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniformMatrix3fv(_uniforms.at(name), 1, GL_FALSE, &t[0][0]);
	//	updateUniformMetric();
	//}
}

void Program::uniform(const std::string & name, const glm::mat4 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glUniformMatrix4fv(_uniforms.at(name), 1, GL_FALSE, &t[0][0]);
	//	updateUniformMetric();
	//}
}

void Program::uniformBuffer(const std::string & name, size_t slot) const {
	//if(_uniforms.count(name) != 0) {
		//glUniformBlockBinding(_id, _uniforms.at(name), GLuint(slot));
		//	updateUniformMetric();
		//}
}

void Program::uniformTexture(const std::string & name, size_t slot) const {
	//if(_uniforms.count(name) != 0) {
		//glUniform1i(_uniforms.at(name), int(slot));
		//	updateUniformMetric();
		//}
}

void Program::getUniform(const std::string & name, bool & t) const {
	//if(_uniforms.count(name) != 0) {
	//	int val = int(t);
		//glGetUniformiv(_id, _uniforms.at(name), &val);
	//	t = bool(val);
	//}
}

void Program::getUniform(const std::string & name, int & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformiv(_id, _uniforms.at(name), &t);
	//}
}

void Program::getUniform(const std::string & name, uint & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformuiv(_id, _uniforms.at(name), &t);
	//}
}

void Program::getUniform(const std::string & name, float & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformfv(_id, _uniforms.at(name), &t);
	//}
}

void Program::getUniform(const std::string & name, glm::vec2 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformfv(_id, _uniforms.at(name), &t[0]);
	//}
}

void Program::getUniform(const std::string & name, glm::vec3 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformfv(_id, _uniforms.at(name), &t[0]);
	//}
}

void Program::getUniform(const std::string & name, glm::vec4 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformfv(_id, _uniforms.at(name), &t[0]);
	//}
}

void Program::getUniform(const std::string & name, glm::ivec2 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformiv(_id, _uniforms.at(name), &t[0]);
	//}
}

void Program::getUniform(const std::string & name, glm::ivec3 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformiv(_id, _uniforms.at(name), &t[0]);
	//}
}

void Program::getUniform(const std::string & name, glm::ivec4 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformiv(_id, _uniforms.at(name), &t[0]);
		//}
}

void Program::getUniform(const std::string & name, glm::mat3 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformfv(_id, _uniforms.at(name), &t[0][0]);
		//}
}

void Program::getUniform(const std::string & name, glm::mat4 & t) const {
	//if(_uniforms.count(name) != 0) {
		//glGetUniformfv(_id, _uniforms.at(name), &t[0][0]);
		//}
}

void Program::updateUniformMetric() const {
#define UDPATE_METRICS
#ifdef UDPATE_METRICS
	//GPU::_metrics.uniforms += 1;
#endif
}

#include "ProgramInfos.h"

#include <iostream>

#include "GLUtilities.h"

ProgramInfos::ProgramInfos(){
	_id = 0;
	_uniforms.clear();
	_textures.clear();
}

ProgramInfos::ProgramInfos(const std::string & vertexContent, const std::string & fragmentContent){
	_id = GLUtilities::createProgram(vertexContent, fragmentContent);
	_uniforms.clear();
	_textures.clear();
}

void ProgramInfos::registerUniform(const std::string & name){
	if(_uniforms.count(name) > 0){
		// Already setup, ignore.
		return;
	}
	glUseProgram(_id);
	_uniforms[name] = glGetUniformLocation(_id, name.c_str());
	glUseProgram(0);
}

void ProgramInfos::registerUniforms(const std::vector<std::string>& names)
{
	glUseProgram(_id);
	for (const auto & name : names) {
		if (_uniforms.count(name) > 0) {
			// Already setup, ignore.
			continue;
		}
		_uniforms[name] = glGetUniformLocation(_id, name.c_str());
	}
	glUseProgram(0);
}

void ProgramInfos::registerTexture(const std::string & name, int slot){
	if(_uniforms.count(name) > 0){
		// Already setup, ignore.
		return;
	}
	glUseProgram(_id);
	_uniforms[name] = glGetUniformLocation(_id, name.c_str());
	_textures[name] = slot;
	glUniform1i(_uniforms[name], slot);
	glUseProgram(0);
}

void ProgramInfos::registerUniform(const std::string & name, const glm::vec3 & val) {
	
	_vec3s[name] = val;
	glUseProgram(_id);
	if (_uniforms.count(name) == 0) {
		_uniforms[name] = glGetUniformLocation(_id, name.c_str());
	}
	glUniform3fv(_uniforms[name], 1, &(_vec3s[name][0]));
	glUseProgram(0);
}

void ProgramInfos::reload(const std::string & vertexContent, const std::string & fragmentContent)
{
	_id = GLUtilities::createProgram(vertexContent, fragmentContent);
	glUseProgram(_id);
	for (auto & uni : _uniforms) {
		_uniforms[uni.first] = glGetUniformLocation(_id, uni.first.c_str());
		if (_textures.count(uni.first) > 0) {
			glUniform1i(_uniforms[uni.first], _textures[uni.first]);
		} else if (_vec3s.count(uni.first) > 0) {
			glUniform3fv(_uniforms[uni.first], 1, &(_vec3s[uni.first][0]));
		}
	}
	glUseProgram(0);
}


ProgramInfos::~ProgramInfos(){
	glDeleteProgram(_id);
}

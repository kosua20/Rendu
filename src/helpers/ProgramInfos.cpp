#include "ProgramInfos.h"

#include <iostream>

#include "GLUtilities.h"
#include "ResourcesManager.h"

ProgramInfos::ProgramInfos(){
	_id = 0;
	_uniforms.clear();
	_textures.clear();
}

ProgramInfos::ProgramInfos(const std::string & vertexName, const std::string & fragmentName){
	_vertexName = vertexName;
	_fragmentName = fragmentName;
	
	const std::string vertexContent = Resources::manager().getShader(_vertexName, Resources::Vertex);
	const std::string fragmentContent = Resources::manager().getShader(_fragmentName, Resources::Fragment);
	
	_id = GLUtilities::createProgram(vertexContent, fragmentContent);
	_uniforms.clear();
	_textures.clear();
	
	// Get the number of active uniforms and their maximum length.
	GLint count = 0;
	GLint size = 0;
	glGetProgramiv(_id, GL_ACTIVE_UNIFORMS, &count);
	glGetProgramiv(_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &size);
	
	glUseProgram(_id);
	for(GLuint i = 0; i < count; ++i){
		// Get infos (name, name length, type,...) of each uniform.
		std::vector<GLchar> uname(size);
		GLenum utype;
		GLint usize = 0;
		GLsizei ulength = 0;
		glGetActiveUniform(_id, i, size, &ulength, &usize, &utype, &uname[0]);
		const std::string name(&uname[0]);
		// Skip empty or default uniforms (starting with 'gl_').
		if(usize == 0 || name.size() == 0 || (name.size() > 3 && name.substr(0,3) == "gl_")){
			continue;
		}
		// Register uniform using its name.
		// /!\ the uniform location can be different from the uniform ID.
		_uniforms[name] = glGetUniformLocation(_id, name.c_str());
		// If the size of the uniform is > 1, we have an array.
		if(usize > 1){
			// Extract the array name from the 'name[0]' string.
			const std::string subname = name.substr(0, name.find_first_of("["));
			// Get the location of the other array elements.
			for(GLsizei j = 1; j < usize; ++j){
				const std::string vname = subname + "[" + std::to_string(j) + "]";
				_uniforms[vname] = glGetUniformLocation(_id, vname.c_str());
			}
		}
	}
	glUseProgram(0);
	checkGLError();
}


const GLint ProgramInfos::uniform(const std::string & name) const {
	if(_uniforms.count(name) > 0) {
		return _uniforms.at(name);
	}
	// glUniform*(-1,...) won't trigger any error and will simply be ignored.
	return -1;
}

void ProgramInfos::registerTexture(const std::string & name, int slot){
	// Store the slot to which the texture will be associated.
	glUseProgram(_id);
	_textures[name] = slot;
	glUniform1i(_uniforms[name], slot);
	glUseProgram(0);
	checkGLError();
}

void ProgramInfos::cacheUniformArray(const std::string & name, const std::vector<glm::vec3> & vals) {
	// Store the vec3s elements in a cache, to avoid re-setting them at each frame.
	glUseProgram(_id);
	for(size_t i = 0; i < vals.size(); ++i){
		const std::string elementName = name + "[" + std::to_string(i) + "]";
		_vec3s[elementName] = vals[i];
		glUniform3fv(_uniforms[elementName], 1, &(_vec3s[elementName][0]));
	}
	glUseProgram(0);
	checkGLError();
}

void ProgramInfos::reload()
{
	const std::string vertexContent = Resources::manager().getShader(_vertexName, Resources::Vertex);
	const std::string fragmentContent = Resources::manager().getShader(_fragmentName, Resources::Fragment);
	_id = GLUtilities::createProgram(vertexContent, fragmentContent);
	// For each stored uniform, update its location, and update textures slots and cached values.
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

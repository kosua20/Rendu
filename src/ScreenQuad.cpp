#include "ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"

#include <stdio.h>
#include <vector>



ScreenQuad::ScreenQuad(){}

ScreenQuad::~ScreenQuad(){}

void ScreenQuad::init(const std::string & shaderRoot){
	
	// Load the shaders
	_program = Resources::manager().getProgram(shaderRoot, "passthrough", shaderRoot);
	
	// Load geometry.
	loadGeometry();
	
	_program->registerTexture("screenTexture", 0);
	
	checkGLError();
}

void ScreenQuad::init(GLuint textureId, const std::string & shaderRoot){
	
	// Load the shaders
	_program = Resources::manager().getProgram(shaderRoot, "passthrough", shaderRoot);

	// Load geometry.
	loadGeometry();
	
	// Link the texture of the framebuffer for this program.
	_textureIds.push_back(textureId);
	_program->registerTexture("screenTexture", 0);
	
	checkGLError();
	
}

void ScreenQuad::init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot){
	
	// Load the shaders
	_program = Resources::manager().getProgram(shaderRoot, "passthrough", shaderRoot);
	
	loadGeometry();
	
	// Link the texture of the framebuffer for this program.
	GLint currentTextureSlot = 0;
	for(auto& texture : textureIds){
		_textureIds.push_back(texture.second);
		_program->registerTexture(texture.first, currentTextureSlot);
		currentTextureSlot += 1;
	}

	checkGLError();
	
}

void ScreenQuad::loadGeometry(){

	// Create an array buffer to host the geometry data.
	//GLuint vbo = 0;
	//glGenBuffers(1, &vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
	
	// Generate an empty VAO (imposed by the OpenGL spec).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);

	glBindVertexArray(0);
	
}

void ScreenQuad::draw() const {
	
	// Select the program (and shaders).
	glUseProgram(_program->id());
	
	// Active screen texture.
	for(GLuint i = 0;i < _textureIds.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, _textureIds[i]);
	}
	
	// Draw with an empty VAO (mandatory)
	glBindVertexArray(_vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	
	glBindVertexArray(0);
	glUseProgram(0);
}

void ScreenQuad::draw(const glm::vec2& invScreenSize) const {
	
	// Select the program (and shaders).
	glUseProgram(_program->id());
	
	// Inverse screen size uniform.
	glUniform2fv(_program->uniform("inverseScreenSize"), 1, &(invScreenSize[0]));
	
	draw();
	
}

void ScreenQuad::draw(const GLuint textureId) const {
	
	// Select the program (and shaders).
	glUseProgram(_program->id());
	
	// Override stored textures.
	glActiveTexture(GL_TEXTURE0 );
	glBindTexture(GL_TEXTURE_2D, textureId);
	
	// Draw with an empty VAO (mandatory)
	glBindVertexArray(_vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	
	glBindVertexArray(0);
	glUseProgram(0);
}

void ScreenQuad::draw(const GLuint textureId, const glm::vec2& invScreenSize) const {
	
	// Select the program (and shaders).
	glUseProgram(_program->id());
	
	// Inverse screen size uniform.
	glUniform2fv(_program->uniform("inverseScreenSize"), 1, &(invScreenSize[0]));
	
	draw(textureId);
}




void ScreenQuad::clean() const {
	glDeleteVertexArrays(1, &_vao);
}



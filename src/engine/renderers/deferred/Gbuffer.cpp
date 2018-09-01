#include "Gbuffer.hpp"

Gbuffer::Gbuffer(unsigned int width, unsigned int height) {
	_width = width;
	_height = height;
	
	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	
	// Create the textures.
	// Albedo
	GLuint albedoId, normalId, depthId, effectsId;
	
	glGenTextures(1, &albedoId);
	glBindTexture(GL_TEXTURE_2D, albedoId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (GLsizei)_width , (GLsizei)_height, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedoId, 0);
	_textureIds[TextureType::Albedo] = albedoId;
	
	glGenTextures(1, &normalId);
	glBindTexture(GL_TEXTURE_2D, normalId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, (GLsizei)_width , (GLsizei)_height, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalId, 0);
	_textureIds[TextureType::Normal] = normalId;
	
	glGenTextures(1, &effectsId);
	glBindTexture(GL_TEXTURE_2D, effectsId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, (GLsizei)_width , (GLsizei)_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, effectsId, 0);
	_textureIds[TextureType::Effects] = effectsId;
	
	glGenTextures(1, &depthId);
	glBindTexture(GL_TEXTURE_2D, depthId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, (GLsizei)_width , (GLsizei)_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthId, 0);
	_textureIds[TextureType::Depth] = depthId;
	
	
	//Register which color attachments to draw to.
	GLenum drawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, drawBuffers);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Gbuffer::~Gbuffer(){ clean(); }

void Gbuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
}

void Gbuffer::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const std::vector<GLuint> Gbuffer::textureIds(const std::vector<TextureType>& included) const {
	
	//bool includeAll = (included.size() == 0);
	std::vector<GLuint> texs;
	texs.reserve(included.size());
	
	for(const auto & texRequest : included){
		if(_textureIds.count(texRequest) > 0){
			texs.push_back(_textureIds.at(texRequest));
		} else {
			Log::Warning() << "Missing texture in Gbuffer." << std::endl;
			texs.push_back(0);
		}
	}
	return texs;
}


void Gbuffer::resize(unsigned int width, unsigned int height){
	_width = width;
	_height = height;
	
	
	// Resize the texture.
	glBindTexture(GL_TEXTURE_2D, _textureIds[TextureType::Albedo]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (GLsizei)_width , (GLsizei)_height, 0, GL_RGBA, GL_FLOAT, 0);
	
	glBindTexture(GL_TEXTURE_2D, _textureIds[TextureType::Normal]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, (GLsizei)_width , (GLsizei)_height, 0, GL_RGB, GL_FLOAT, 0);
	
	glBindTexture(GL_TEXTURE_2D, _textureIds[TextureType::Effects]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)_width , (GLsizei)_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	
	glBindTexture(GL_TEXTURE_2D, _textureIds[TextureType::Depth]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, (GLsizei)_width , (GLsizei)_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
}

void Gbuffer::resize(glm::vec2 size){
	resize((unsigned int)size[0], (unsigned int)size[1]);
}

void Gbuffer::clean() const {
	for(auto& tex : _textureIds){
		glDeleteTextures(1, &(tex.second));
	}
	glDeleteFramebuffers(1, &_id);
}


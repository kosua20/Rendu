#include <stdio.h>
#include <iostream>

#include "helpers/ProgramUtilities.h"
#include "Framebuffer.h"

Framebuffer::Framebuffer() : _width(0),  _height(0){}

Framebuffer::Framebuffer(int width, int height) : _width(width),  _height(height){}

Framebuffer::~Framebuffer(){ clean(); }

void Framebuffer::bind(){
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
}

void Framebuffer::unbind(){
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::setup(){
	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	
	// Create the texture to store the result.
	glGenTextures(1, &_idColor);
	glBindTexture(GL_TEXTURE_2D, _idColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA,  GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// Link the texture to the first color attachment (ie output) of the framebuffer.
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 ,GL_TEXTURE_2D, _idColor, 0);
	
	// Create the renderbuffer (depth buffer + color(s) buffer(s)).
	glGenRenderbuffers(1, &_idRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, _idRenderbuffer);
	// Setup the depth buffer storage.
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, _width, _height);
	// Link the renderbuffer to the framebuffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _idRenderbuffer);
	
	//Register which color attachments to draw to.
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int width, int height){
	_width = width;
	_height = height;
	// Resize the renderbuffer.
	glBindRenderbuffer(GL_RENDERBUFFER, _idRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, _width, _height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	// Resize the texture.
	glBindTexture(GL_TEXTURE_2D, _idColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
}

void Framebuffer::clean(){
	glDeleteRenderbuffers(1, &_idRenderbuffer);
	glDeleteTextures(1, &_idColor);
	glDeleteFramebuffers(1, &_id);
}


#include "Renderer2D.hpp"
#include "../../input/Input.hpp"

#include "../../helpers/GLUtilities.hpp"

#include <stdio.h>
#include <vector>


Renderer2D::~Renderer2D(){}

Renderer2D::Renderer2D(Config & config, const std::string & shaderName, const int width, const int height, const GLenum format, const GLenum type, const GLenum preciseFormat, const std::string & outputPath) : Renderer(config) {
	
	_outputPath = outputPath;
	
	_resultFramebuffer = std::make_shared<Framebuffer>(width, height, format, type, preciseFormat, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	
	checkGLError();

	// GL options
	glDisable(GL_DEPTH_TEST);
	checkGLError();
	
	_resultScreen.init(shaderName);
	checkGLError();
	
}


void Renderer2D::draw() {

	_resultFramebuffer->bind();
	
	glViewport(0,0,_resultFramebuffer->width(), _resultFramebuffer->height());
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	_resultScreen.draw();
	
	glFlush();
	glFinish();
	
	GLUtilities::saveFramebuffer(_resultFramebuffer, (unsigned int)_resultFramebuffer->width(), (unsigned int)_resultFramebuffer->height(), _outputPath);

	_resultFramebuffer->unbind();
}

void Renderer2D::update(){
	Renderer::update();
	// Update nothing.
}

void Renderer2D::physics(double fullTime, double frameTime){
	// Simulate nothing.
}


void Renderer2D::clean() const {
	Renderer::clean();
	// Clean objects.
	_resultScreen.clean();
	_resultFramebuffer->clean();
	
}


void Renderer2D::resize(int width, int height){
	// Do nothing.
}




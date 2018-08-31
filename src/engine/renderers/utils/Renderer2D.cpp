#include "Renderer2D.hpp"
#include "../../input/Input.hpp"
#include "../../helpers/GLUtilities.hpp"



Renderer2D::~Renderer2D(){}

Renderer2D::Renderer2D(Config & config, const std::string & shaderName, const unsigned int width, const unsigned int height, const GLenum format, const GLenum type, const GLenum preciseFormat) : Renderer(config) {
	glDisable(GL_DEPTH_TEST);
	_resultFramebuffer = std::make_shared<Framebuffer>(width, height, format, type, preciseFormat, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	_resultProgram = Resources::manager().getProgram2D(shaderName);
	checkGLError();
}


void Renderer2D::draw() {
	glDisable(GL_DEPTH_TEST);
	_resultFramebuffer->bind();
	
	glViewport(0,0,_resultFramebuffer->width(), _resultFramebuffer->height());
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_resultProgram->id());
	ScreenQuad::draw();
	
	glFlush();
	glFinish();

	_resultFramebuffer->unbind();
	glEnable(GL_DEPTH_TEST);
}

void Renderer2D::save(const std::string & outputPath){
	GLUtilities::saveFramebuffer(_resultFramebuffer, (unsigned int)_resultFramebuffer->width(), (unsigned int)_resultFramebuffer->height(), outputPath, false);
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
	_resultFramebuffer->clean();
	
}


void Renderer2D::resize(unsigned int width, unsigned int height){
	// Do nothing.
}




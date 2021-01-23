#include "processing/BilateralBlur.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"

BilateralBlur::BilateralBlur(const std::string & name) : _name(name) {

	_filter	 = Resources::manager().getProgram2D("bilateral");
	checkGLError();
}

// Draw function
void BilateralBlur::process(const glm::mat4 & projection, const Texture * texture, const Texture * depthTex, const Texture * normalTex, Framebuffer & framebuffer) {

	GLUtilities::setDepthState(false);
	GLUtilities::setBlendState(false);
	GLUtilities::setCullState(true, Faces::BACK);

	if(!_intermediate || _intermediate->descriptor() != framebuffer.descriptor()){
		_intermediate.reset(new Framebuffer(framebuffer.width(), framebuffer.height(), framebuffer.descriptor(), false, _name + " Bilateral blur"));
	}

	if(framebuffer.width() != _intermediate->width() || framebuffer.height() != _intermediate->height()){
		resize(framebuffer.width(), framebuffer.height());
	}
	_filter->use();
	GLUtilities::bindTexture(depthTex, 1);
	GLUtilities::bindTexture(normalTex, 2);
	_filter->uniform("invDstSize", 1.0f/glm::vec2(framebuffer.width(), framebuffer.height()));
	_filter->uniform("projParams", glm::vec2( projection[2][2], projection[3][2]));
	framebuffer.setViewport();

	_intermediate->bind();
	_filter->uniform("axis", 0);
	GLUtilities::bindTexture(texture, 0);
	ScreenQuad::draw();

	framebuffer.bind();
	_filter->uniform("axis", 1);
	GLUtilities::bindTexture(_intermediate->texture(), 0);
	ScreenQuad::draw();
}

void BilateralBlur::resize(unsigned int width, unsigned int height) const {
	_intermediate->resize(width, height);
}

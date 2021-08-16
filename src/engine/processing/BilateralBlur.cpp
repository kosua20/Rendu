#include "processing/BilateralBlur.hpp"
#include "graphics/GPUTypes.hpp"
#include "graphics/GPU.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"

BilateralBlur::BilateralBlur(const std::string & name) : _name(name) {
	_filter	 = Resources::manager().getProgram2D("bilateral");
}

// Draw function
void BilateralBlur::process(const glm::mat4 & projection, const Texture * texture, const Texture * depthTex, const Texture * normalTex, Framebuffer & framebuffer) {

	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	if(!_intermediate || _intermediate->descriptor() != framebuffer.descriptor()){
		_intermediate.reset(new Framebuffer(framebuffer.width(), framebuffer.height(), framebuffer.descriptor(), false, _name + " Bilateral blur"));
	}

	if(framebuffer.width() != _intermediate->width() || framebuffer.height() != _intermediate->height()){
		resize(framebuffer.width(), framebuffer.height());
	}
	_filter->use();
	_filter->texture(depthTex, 1);
	_filter->texture(normalTex, 2);
	_filter->uniform("invDstSize", 1.0f/glm::vec2(framebuffer.width(), framebuffer.height()));
	_filter->uniform("projParams", glm::vec2( projection[2][2], projection[3][2]));
	framebuffer.setViewport();

	_intermediate->bind(Framebuffer::Operation::DONTCARE);
	_filter->uniform("axis", 0);
	_filter->texture(texture, 0);
	ScreenQuad::draw();

	framebuffer.bind(Framebuffer::Operation::DONTCARE);
	_filter->uniform("axis", 1);
	_filter->texture(_intermediate->texture(), 0);
	ScreenQuad::draw();
}

void BilateralBlur::resize(unsigned int width, unsigned int height) const {
	_intermediate->resize(width, height);
}

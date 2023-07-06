#include "processing/BilateralBlur.hpp"
#include "graphics/GPUTypes.hpp"
#include "graphics/GPU.hpp"
#include "resources/ResourcesManager.hpp"

BilateralBlur::BilateralBlur(const std::string & name) : _intermediate(name + "Bilateral blur") {
	_filter	 = Resources::manager().getProgram2D("bilateral");
}

// Draw function
void BilateralBlur::process(const glm::mat4 & projection, const Texture& src, const Texture& depthTex, const Texture& normalTex, Texture & dst) {
	
	GPUMarker marker("Bilateral blur");
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	if(!_intermediate.gpu || _intermediate.format != dst.format){
		_intermediate.setupAsDrawable(dst.format, dst.width, dst.height);
	}

	if(dst.width != _intermediate.width || dst.height != _intermediate.height){
		resize(dst.width, dst.height);
	}
	_filter->use();
	_filter->texture(depthTex, 1);
	_filter->texture(normalTex, 2);
	_filter->uniform("invDstSize", 1.0f/glm::vec2(dst.width, dst.height));
	_filter->uniform("projParams", glm::vec2( projection[2][2], projection[3][2]));

	GPU::setViewport(_intermediate);
	GPU::beginRender(Load::Operation::DONTCARE, &_intermediate);
	_filter->uniform("axis", 0);
	_filter->texture(src, 0);
	GPU::drawQuad();
	GPU::endRender();

	GPU::beginRender(Load::Operation::DONTCARE, &dst);
	_filter->uniform("axis", 1);
	_filter->texture(_intermediate, 0);
	GPU::drawQuad();
	GPU::endRender();
}

void BilateralBlur::resize(uint width, uint height) {
	_intermediate.resize(width, height);
}

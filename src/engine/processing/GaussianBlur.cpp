#include "processing/GaussianBlur.hpp"
#include "graphics/GPU.hpp"
#include "resources/ResourcesManager.hpp"


GaussianBlur::GaussianBlur(uint radius, uint downscale, const std::string & name) : _downscale(downscale) {
	_passthrough 		= Resources::manager().getProgram("passthrough");
	_blurProgramDown	= Resources::manager().getProgram2D("blur-dual-filter-down");
	_blurProgramUp		= Resources::manager().getProgram2D("blur-dual-filter-up");

	for(size_t i = 0; i < radius; ++i) {
		_levels.emplace_back(name + " Gaussian blur level " + std::to_string(i));
	}
}

void GaussianBlur::process(const Texture& src, Texture & dst) {
	if(_levels.empty()) {
		return;
	}

	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	const uint width = dst.width / _downscale;
	const uint height = dst.height / _downscale;
	if(!_levels[0].gpu || _levels[0].format != dst.format){
		for(size_t i = 0; i < _levels.size(); ++i) {
			_levels[i].setupAsDrawable(dst.format, uint(width / std::pow(2, i)), uint(height / std::pow(2, i)));
		}
	}
	if(_levels[0].width != width || _levels[0].height != height){
		resize(dst.width, dst.height);
	}

	// First, copy the input texture to the first texture level.
	GPU::bind(Load::Operation::DONTCARE, &_levels[0]);
	GPU::setViewport(_levels[0]);

	_passthrough->use();
	_passthrough->texture(src, 0);
	GPU::drawQuad();

	// Downscale filter.
	_blurProgramDown->use();
	for(size_t d = 1; d < _levels.size(); ++d) {
		GPU::bind(glm::vec4(0.0f), &_levels[d]);
		GPU::setViewport(_levels[d]);
		_blurProgramDown->texture(_levels[d - 1], 0);
		GPU::drawQuad();
	}

	// Upscale filter.
	_blurProgramUp->use();
	for(int d = int(_levels.size()) - 2; d >= 0; --d) {
		GPU::bind(glm::vec4(0.0f), &_levels[d]);
		GPU::setViewport(_levels[d]);
		_blurProgramUp->texture(_levels[d + 1], 0);
		GPU::drawQuad();
	}
	// Copy from the last texture used to the destination.
	GPU::blit(_levels[0], dst, Filter::LINEAR);
}

void GaussianBlur::resize(uint width, uint height) {
	const uint dwidth = width/_downscale;
	const uint dheight = height/_downscale;
	for(size_t i = 0; i < _levels.size(); ++i) {
		const unsigned int hwidth  = uint(dwidth / std::pow(2, i));
		const unsigned int hheight = uint(dheight / std::pow(2, i));
		_levels[i].resize(hwidth, hheight);
	}
}

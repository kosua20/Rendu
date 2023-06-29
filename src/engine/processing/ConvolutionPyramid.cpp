#include "processing/ConvolutionPyramid.hpp"
#include "graphics/GPU.hpp"

ConvolutionPyramid::ConvolutionPyramid(uint width, uint height, uint inoutPadding) :
	_shifted("Conv. pyramid shift"), _padding(int(inoutPadding)) {

	// Convolution pyramids filters and scaling operations.
	_downscale = Resources::manager().getProgram2D("downscale");
	_upscale   = Resources::manager().getProgram2D("upscale");
	_filter	= Resources::manager().getProgram2D("filter");
	_padder	= Resources::manager().getProgram2D("passthrough-shift");

	// Pre and post process texture.
	// Output is at the basic required size.
	_shifted.setupAsDrawable(Layout::RGBA32F, width, height);

	// Resolution of the pyramid takes into account the filter padding.
	_resolution = glm::ivec2(width + 2 * _padding, height + 2 * _padding);

	// Create a series of textures smaller and smaller.
	const int depth = int(std::ceil(std::log2(std::min(_resolution[0], _resolution[1]))));
	_levelsIn.reserve(depth);
	_levelsOut.reserve(depth);
	// Initial padded size.
	int levelWidth  = _resolution[0] + 2 * _size;
	int levelHeight = _resolution[1] + 2 * _size;
	// Generate textures pyramids.
	for(size_t i = 0; i < size_t(depth); ++i) {
		_levelsIn.emplace_back("Conv. pyramid in " + std::to_string(i));
		_levelsIn.back().setupAsDrawable(Layout::RGBA32F, levelWidth, levelHeight);

		_levelsOut.emplace_back("Conv. pyramid out " + std::to_string(i));
		_levelsOut.back().setupAsDrawable(Layout::RGBA32F, levelWidth, levelHeight);
		// Downscaling and padding.
		levelWidth /= 2;
		levelHeight /= 2;
		levelWidth += 2 * _size;
		levelHeight += 2 * _size;
	}
}

void ConvolutionPyramid::process(const Texture& texture) {
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);
	
	// Pad by the size of the filter.
	GPU::bind(glm::vec4(0.0f), &_levelsIn[0]);
	// Shift the viewport and fill the padded region with 0s.
	GPU::setViewport(_size, _size, int(_levelsIn[0].width) - 2 * _size, int(_levelsIn[0].height) - 2 * _size);
	// Transfer the boundary content.
	_padder->use();
	_padder->uniform("padding", _size);
	_padder->texture(texture, 0);
	GPU::drawQuad();

	// Then iterate over all levels, cascading down the filtered results.
	/// \note Those filters are separable, and could be applied in two passes (vertical and horizontal) to reduce the texture fetches count.
	// Send parameters.
	_downscale->use();
	_downscale->uniform("h1[0]", _h1[0]);
	_downscale->uniform("h1[1]", _h1[1]);
	_downscale->uniform("h1[2]", _h1[2]);
	_downscale->uniform("h1[3]", _h1[3]);
	_downscale->uniform("h1[4]", _h1[4]);

	// Do: l[i] = downscale(filter(l[i-1], h1))
	for(size_t i = 1; i < _levelsIn.size(); ++i) {
		GPU::bind(glm::vec4(0.0f), &_levelsIn[i]);
		// Shift the viewport and fill the padded region with 0s.
		GPU::setViewport(_size, _size, int(_levelsIn[i].width) - 2 * _size, int(_levelsIn[i].height) - 2 * _size);
		// Filter and downscale.
		_downscale->texture(_levelsIn[i - 1], 0);
		GPU::drawQuad();
	}

	// Filter the last level with g.
	// Send parameters.
	_filter->use();
	_filter->uniform("g[0]", _g[0]);
	_filter->uniform("g[1]", _g[1]);
	_filter->uniform("g[2]", _g[2]);

	// Do:  f[end] = filter(l[end], g)
	const auto & lastLevel = _levelsOut.back();
	GPU::bind(Load::Operation::DONTCARE, &lastLevel);
	GPU::setViewport(lastLevel);
	_filter->texture(_levelsIn.back(), 0);
	GPU::drawQuad();

	// Flatten the pyramid from the bottom, combining the filtered current result and the next level.
	_upscale->use();
	_upscale->uniform("h1[0]", _h1[0]);
	_upscale->uniform("h1[1]", _h1[1]);
	_upscale->uniform("h1[2]", _h1[2]);
	_upscale->uniform("h1[3]", _h1[3]);
	_upscale->uniform("h1[4]", _h1[4]);
	_upscale->uniform("g[0]", _g[0]);
	_upscale->uniform("g[1]", _g[1]);
	_upscale->uniform("g[2]", _g[2]);
	_upscale->uniform("h2", _h2);

	// Do: f[i] = filter(l[i], g) + filter(upscale(f[i+1], h2)
	for(int i = int(_levelsOut.size() - 2); i >= 0; --i) {
		GPU::bind(Load::Operation::DONTCARE, &_levelsOut[i]);
		GPU::setViewport(_levelsOut[i]);
		// Upscale with zeros, filter and combine.
		_upscale->texture(_levelsIn[i], 0);
		_upscale->texture(_levelsOut[i + 1], 1);
		GPU::drawQuad();
	}

	// Compensate the initial padding.
	GPU::bind(Load::Operation::DONTCARE, &_shifted);
	GPU::setViewport(_shifted);
	_padder->use();
	// Need to also compensate for the potential extra padding.
	_padder->uniform("padding", -_size - _padding);
	_padder->texture(_levelsOut[0], 0);
	GPU::drawQuad();
}

void ConvolutionPyramid::setFilters(const float h1[5], float h2, const float g[3]) {
	_h1[0] = h1[0];
	_h1[1] = h1[1];
	_h1[2] = h1[2];
	_h1[3] = h1[3];
	_h1[4] = h1[4];
	_h2	= h2;
	_g[0]  = g[0];
	_g[1]  = g[1];
	_g[2]  = g[2];
}

void ConvolutionPyramid::resize(uint width, uint height) {
	_shifted.resize(width, height);
	// Resolution of the pyramid takes into account the filter padding.
	_resolution = glm::ivec2(width + 2 * _padding, height + 2 * _padding);

	const int newDepth = std::max(int(std::ceil(std::log2(std::min(_resolution[0], _resolution[1])))), 1);
	// Create a series of textures smaller and smaller.
	_levelsIn.clear(); _levelsIn.reserve(newDepth);
	_levelsOut.clear(); _levelsOut.reserve(newDepth);
	// Initial padded size.
	int levelWidth  = _resolution[0] + 2 * _size;
	int levelHeight = _resolution[1] + 2 * _size;

	// Generate texture pyramids.
	for(int i = 0; i < newDepth; ++i) {
		_levelsIn.emplace_back("Conv. pyramid in " + std::to_string(i));
		_levelsIn.back().setupAsDrawable(Layout::RGBA32F, levelWidth, levelHeight);
		_levelsOut.emplace_back("Conv. pyramid out " + std::to_string(i));
		_levelsOut.back().setupAsDrawable(Layout::RGBA32F, levelWidth, levelHeight);

		// Downscaling and padding.
		levelWidth /= 2;
		levelHeight /= 2;
		levelWidth += 2 * _size;
		levelHeight += 2 * _size;
	}
}

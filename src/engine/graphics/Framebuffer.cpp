#include "graphics/Framebuffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "renderers/DebugViewer.hpp"


Framebuffer::Framebuffer(uint width, uint height, const Layout & format, const std::string & name) :
	Framebuffer(TextureShape::D2, width, height, 1, 1, std::vector<Layout>(1, format), name) {
}

Framebuffer::Framebuffer(uint width, uint height, const std::vector<Layout> & formats, const std::string & name) :
	Framebuffer(TextureShape::D2, width, height, 1, 1, formats, name) {
}

Framebuffer::Framebuffer(TextureShape shape, uint width, uint height, uint depth, uint mips, const std::vector<Layout> & formats, const std::string & name) : _depth("Depth ## " + name), _name(name),
	_width(width), _height(height), _mips(mips) {


	// Check that the shape is supported.
	_shape = shape;
	if(_shape != TextureShape::D2 && _shape != TextureShape::Array2D && _shape != TextureShape::Cube && _shape != TextureShape::ArrayCube){
		Log::Error() << Log::GPU << "Unsupported framebuffer shape." << std::endl;
		return;
	}
	if(shape == TextureShape::D2){
		_layers = 1;
	} else if(shape == TextureShape::Cube){
		_layers = 6;
	} else if(shape == TextureShape::ArrayCube){
		_layers = 6 * depth;
	} else {
		_layers = depth;
	}

	uint cid = 0;
	for(const Layout & format : formats) {
		// Create the color texture to store the result.
		const bool isDepthComp		  = format == Layout::DEPTH_COMPONENT16 || format == Layout::DEPTH_COMPONENT24 || format == Layout::DEPTH_COMPONENT32F;
		const bool hasStencil = format == Layout::DEPTH24_STENCIL8 || format == Layout::DEPTH32F_STENCIL8;

		if(isDepthComp || hasStencil) {
			_hasDepth	  = true;
			Texture::setupAsFramebuffer(_depth, format, _width, _height, _mips, shape, depth);


		} else {
			_colors.emplace_back("Color " + std::to_string(cid++) + " ## " + _name);
			Texture & tex = _colors.back();
			Texture::setupAsFramebuffer(tex, format, _width, _height, _mips, shape, depth);
		}
	}
	DebugViewer::trackDefault(this);
}


void Framebuffer::bind(const Load& colorOp, const Load& depthOp, const Load& stencilOp) const {
	bind(0, 0, colorOp, depthOp, stencilOp);
}

void Framebuffer::bind(uint layer, uint mip, const Load& colorOp, const Load& depthOp, const Load& stencilOp) const {

	GPU::bindFramebuffer(layer, mip, depthOp, stencilOp, colorOp, _hasDepth ? &_depth : nullptr, _colors.size() > 0 ? &_colors[0] : nullptr,  _colors.size() > 1 ? &_colors[1] : nullptr,  _colors.size() > 2 ? &_colors[2] : nullptr,  _colors.size() > 3 ? &_colors[3] : nullptr);
}

void Framebuffer::setViewport() const {
	GPU::setViewport(0, 0, int(_width), int(_height));
}

void Framebuffer::resize(uint width, uint height) {
	_width  = width;
	_height = height;

	// Resize the renderbuffer.
	if(_hasDepth) {
		_depth.width  = _width;
		_depth.height = _height;
		GPU::setupTexture(_depth, _depth.gpu->typedFormat, true);
	}

	// Resize the textures.
	for(Texture & color : _colors) {
		color.width  = _width;
		color.height = _height;
		GPU::setupTexture(color, color.gpu->typedFormat, true);
	}
}

void Framebuffer::resize(const glm::ivec2 & size) {
	resize(uint(size[0]), uint(size[1]));
}

void Framebuffer::clear(const glm::vec4 & color, float depth){
	const uint colorCount = _colors.size();
	for(uint cid = 0; cid < colorCount; ++cid ){
		GPU::clearTexture(_colors[cid], color);
	}

	if(_hasDepth){
		GPU::clearDepth(_depth, depth);
	}

}

glm::vec4 Framebuffer::read(const glm::uvec2 & pos) {
	if(_colors.empty()){
		return _readColor;
	}

	_readTask = GPU::downloadTextureAsync( _colors[0], pos, glm::uvec2(2), 1, [this](const Texture& result){
		_readColor = result.images[0].rgba(0, 0);
	});

	// Return the value from the previous frame.
	return _readColor;
}

const Layout & Framebuffer::format(unsigned int i) const {
   return _colors[i].gpu->typedFormat;
}

uint Framebuffer::attachments() const {
	return uint(_colors.size());
}


Framebuffer::~Framebuffer() {
	GPU::cancelAsyncOperation(_readTask);
	DebugViewer::untrackDefault(this);
}


#include "processing/BoxBlur.hpp"
#include "graphics/GPU.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/Library.hpp"
#include "resources/ResourcesManager.hpp"

BoxBlur::BoxBlur(bool approximate, const std::string & name) : _name(name) {

	const std::string suffix = approximate ? "-approx" : "";
	_blur2D = Resources::manager().getProgram2D("box-blur-2d" + suffix);
	_blurArray = Resources::manager().getProgram2D("box-blur-2d-array" + suffix);
	_blurCube = Resources::manager().getProgram("box-blur-cube" + suffix, "box-blur-cube", "box-blur-cube" + suffix);
	_blurCubeArray = Resources::manager().getProgram("box-blur-cube-array" + suffix, "box-blur-cube","box-blur-cube-array" + suffix);

}

// Draw function
void BoxBlur::process(const Texture * texture, Framebuffer & framebuffer) {

	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);
	
	// Detect changes of descriptor.
	if(!_intermediate || _intermediate->format() != framebuffer.format()){
		_intermediate.reset(new Framebuffer(framebuffer.width(), framebuffer.height(), framebuffer.format(), _name + " Box blur"));
	}
	// Detect changes of size.
	if(_intermediate->width() != framebuffer.width() || _intermediate->height() != framebuffer.height()){
		resize(framebuffer.width() , framebuffer.height());
	}

	_intermediate->setViewport();
	const TextureShape & tgtShape = framebuffer.shape();
	if(tgtShape == TextureShape::D2){
		_blur2D->use();
		_intermediate->bind(Load::Operation::DONTCARE);
		_blur2D->texture(texture, 0);
		ScreenQuad::draw();
		GPU::blit(*_intermediate->texture(0), *framebuffer.texture(0), Filter::NEAREST);

	} else if(tgtShape == TextureShape::Array2D){
		_blurArray->use();
		for(size_t lid = 0; lid < framebuffer.depth(); ++lid){
			_intermediate->bind(Load::Operation::DONTCARE);
			_blurArray->uniform("layer", int(lid));
			_blurArray->texture(texture, 0);
			ScreenQuad::draw();
			GPU::blit(*_intermediate->texture(0), *framebuffer.texture(0), 0, lid, Filter::NEAREST);
		}
	} else if(tgtShape == TextureShape::Cube){
		_blurCube->use();
		_blurCube->uniform("invHalfSize", 2.0f/float(texture->width));
		for(size_t fid = 0; fid < 6; ++fid){
			_intermediate->bind(Load::Operation::DONTCARE);
			_blurCube->uniform("up", Library::boxUps[fid]);
			_blurCube->uniform("right", Library::boxRights[fid]);
			_blurCube->uniform("center", Library::boxCenters[fid]);
			_blurCube->texture(texture, 0);
			ScreenQuad::draw();
			GPU::blit(*_intermediate->texture(0), *framebuffer.texture(0), 0, fid, Filter::NEAREST);
		}
	} else if(tgtShape == TextureShape::ArrayCube){
		_blurCubeArray->use();
		_blurCubeArray->uniform("invHalfSize", 2.0f/float(texture->width));
		for(size_t lid = 0; lid < texture->depth; ++lid){
			const int fid = int(lid)%6;
			_intermediate->bind(Load::Operation::DONTCARE);
			_blurCubeArray->uniform("layer", int(lid)/6);
			_blurCubeArray->uniform("up", Library::boxUps[fid]);
			_blurCubeArray->uniform("right", Library::boxRights[fid]);
			_blurCubeArray->uniform("center", Library::boxCenters[fid]);
			_blurCubeArray->texture(texture, 0);
			ScreenQuad::draw();
			GPU::blit(*_intermediate->texture(0), *framebuffer.texture(0), 0, lid, Filter::NEAREST);
		}
	} else {
		Log::Error() << "Unsupported shape." << std::endl;
	}

}

// Handle screen resizing
void BoxBlur::resize(unsigned int width, unsigned int height) const {
	_intermediate->resize(width, height);
}

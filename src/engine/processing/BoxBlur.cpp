#include "processing/BoxBlur.hpp"
#include "graphics/GPU.hpp"
#include "resources/Library.hpp"
#include "resources/ResourcesManager.hpp"

BoxBlur::BoxBlur(bool approximate, const std::string & name) : _intermediate(name + " Box blur") {

	const std::string suffix = approximate ? "-approx" : "";
	_blur2D = Resources::manager().getProgram2D("box-blur-2d" + suffix);
	_blurArray = Resources::manager().getProgram2D("box-blur-2d-array" + suffix);
	_blurCube = Resources::manager().getProgram("box-blur-cube" + suffix, "box-blur-cube", "box-blur-cube" + suffix);
	_blurCubeArray = Resources::manager().getProgram("box-blur-cube-array" + suffix, "box-blur-cube","box-blur-cube-array" + suffix);

}

// Draw function
void BoxBlur::process(const Texture& src, Texture & dst) {
	GPUMarker marker("Box blur");

	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);
	
	// Detect changes of descriptor.
	if(!_intermediate.gpu || _intermediate.format != dst.format){
		_intermediate.setupAsDrawable(dst.format, dst.width, dst.height);
	}
	// Detect changes of size.
	if(_intermediate.width != dst.width || _intermediate.height != dst.height){
		_intermediate.resize(dst.width, dst.height);
	}

	GPU::setViewport(_intermediate);

	const TextureShape & tgtShape = dst.shape;
	if(tgtShape == TextureShape::D2){
		_blur2D->use();
		GPU::beginRender(Load::Operation::DONTCARE, &_intermediate);
		_blur2D->texture(src, 0);
		GPU::drawQuad();
		GPU::endRender();

		GPU::blit(_intermediate, dst, Filter::NEAREST);

	} else if(tgtShape == TextureShape::Array2D){
		_blurArray->use();
		for(size_t lid = 0; lid < dst.depth; ++lid){
			GPU::beginRender(Load::Operation::DONTCARE, &_intermediate);
			_blurArray->uniform("layer", int(lid));
			_blurArray->texture(src, 0);
			GPU::drawQuad();
			GPU::endRender();

			GPU::blit(_intermediate, dst, 0, lid, Filter::NEAREST);
		}
	} else if(tgtShape == TextureShape::Cube){
		_blurCube->use();
		_blurCube->uniform("invHalfSize", 2.0f/float(src.width));
		for(size_t fid = 0; fid < 6; ++fid){
			GPU::beginRender(Load::Operation::DONTCARE, &_intermediate);
			_blurCube->uniform("up", Library::boxUps[fid]);
			_blurCube->uniform("right", Library::boxRights[fid]);
			_blurCube->uniform("center", Library::boxCenters[fid]);
			_blurCube->texture(src, 0);
			GPU::drawQuad();
			GPU::endRender();

			GPU::blit(_intermediate, dst, 0, fid, Filter::NEAREST);
		}
	} else if(tgtShape == TextureShape::ArrayCube){
		_blurCubeArray->use();
		_blurCubeArray->uniform("invHalfSize", 2.0f/float(src.width));
		for(size_t lid = 0; lid < src.depth; ++lid){
			const int fid = int(lid)%6;
			GPU::beginRender(Load::Operation::DONTCARE, &_intermediate);
			_blurCubeArray->uniform("layer", int(lid)/6);
			_blurCubeArray->uniform("up", Library::boxUps[fid]);
			_blurCubeArray->uniform("right", Library::boxRights[fid]);
			_blurCubeArray->uniform("center", Library::boxCenters[fid]);
			_blurCubeArray->texture(src, 0);
			GPU::drawQuad();
			GPU::endRender();
			
			GPU::blit(_intermediate, dst, 0, lid, Filter::NEAREST);
		}
	} else {
		Log::Error() << "Unsupported shape." << std::endl;
	}

}

// Handle screen resizing
void BoxBlur::resize(uint width, uint height) {
	_intermediate.resize(width, height);
}

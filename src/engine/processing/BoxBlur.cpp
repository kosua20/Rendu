#include "processing/BoxBlur.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/Library.hpp"
#include "resources/ResourcesManager.hpp"

BoxBlur::BoxBlur(bool approximate) {

	const std::string suffix = approximate ? "-approx" : "";
	_blur2D = Resources::manager().getProgram2D("box-blur-2d" + suffix);
	_blurArray = Resources::manager().getProgram2D("box-blur-2d-array" + suffix);
	_blurCube = Resources::manager().getProgram("box-blur-cube" + suffix, "box-blur-cube", "box-blur-cube" + suffix);
	_blurCubeArray = Resources::manager().getProgram("box-blur-cube-array" + suffix, "box-blur-cube","box-blur-cube-array" + suffix);

	checkGLError();
}

// Draw function
void BoxBlur::process(const Texture * texture, Framebuffer & framebuffer) {

	// Detect changes of descriptor.
	if(!_intermediate || _intermediate->descriptor() != framebuffer.descriptor()){
		_intermediate.reset(new Framebuffer(framebuffer.width(), framebuffer.height(), framebuffer.descriptor(), false, "Box blur"));
	}
	// Detect changes of size.
	if(_intermediate->width() != framebuffer.width() || _intermediate->height() != framebuffer.height()){
		resize(framebuffer.width() , framebuffer.height());
	}

	_intermediate->setViewport();
	const TextureShape & tgtShape = framebuffer.shape();
	if(tgtShape == TextureShape::D2){
		_blur2D->use();
		_intermediate->bind();
		ScreenQuad::draw(texture);
		_intermediate->unbind();
		GLUtilities::blit(*_intermediate, framebuffer, Filter::NEAREST);

	} else if(tgtShape == TextureShape::Array2D){
		_blurArray->use();
		for(size_t lid = 0; lid < framebuffer.depth(); ++lid){
			_intermediate->bind();
			_blurArray->uniform("layer", int(lid));
			ScreenQuad::draw(texture);
			_intermediate->unbind();
			GLUtilities::blit(*_intermediate, framebuffer, 0, lid, Filter::NEAREST);
		}
	} else if(tgtShape == TextureShape::Cube){
		_blurCube->use();
		_blurCube->uniform("invHalfSize", 2.0f/float(texture->width));
		for(size_t fid = 0; fid < 6; ++fid){
			_intermediate->bind();
			_blurCube->uniform("up", Library::boxUps[fid]);
			_blurCube->uniform("right", Library::boxRights[fid]);
			_blurCube->uniform("center", Library::boxCenters[fid]);
			ScreenQuad::draw(texture);
			_intermediate->unbind();
			GLUtilities::blit(*_intermediate, framebuffer, 0, fid, Filter::NEAREST);
		}
	} else if(tgtShape == TextureShape::ArrayCube){
		_blurCubeArray->use();
		_blurCubeArray->uniform("invHalfSize", 2.0f/float(texture->width));
		for(size_t lid = 0; lid < texture->depth; ++lid){
			const int fid = int(lid)%6;
			_intermediate->bind();
			_blurCubeArray->uniform("layer", int(lid)/6);
			_blurCubeArray->uniform("up", Library::boxUps[fid]);
			_blurCubeArray->uniform("right", Library::boxRights[fid]);
			_blurCubeArray->uniform("center", Library::boxCenters[fid]);
			ScreenQuad::draw(texture);
			_intermediate->unbind();
			GLUtilities::blit(*_intermediate, framebuffer, 0, lid, Filter::NEAREST);
		}
	} else {
		Log::Error() << "Unsupported shape." << std::endl;
	}

}

// Handle screen resizing
void BoxBlur::resize(unsigned int width, unsigned int height) const {
	_intermediate->resize(width, height);
}

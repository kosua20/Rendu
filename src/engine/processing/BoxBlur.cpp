#include "processing/BoxBlur.hpp"
#include "graphics/GLUtilities.hpp"

BoxBlur::BoxBlur(bool approximate) {

	std::string blurName = "box-blur-";
	if(approximate){
		blurName.append("approx-");
	}
	_blur2D = Resources::manager().getProgram2D(blurName + "2d");
	_blurArray = Resources::manager().getProgram2D(blurName + "array-2d");

	checkGLError();
}

// Draw function
void BoxBlur::process(const Texture * texture, Framebuffer & framebuffer) {

	// Detect changes of descriptor.
	if(!_intermediate || _intermediate->descriptor() != framebuffer.descriptor()){
		if(_intermediate){
			_intermediate->clean();
		}
		_intermediate.reset(new Framebuffer(framebuffer.width(), framebuffer.height(), framebuffer.descriptor(), false));
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
	} else {
		Log::Error() << "Unsupported shape." << std::endl;
	}

}

// Clean function
void BoxBlur::clean() const {
	if(_intermediate){
		_intermediate->clean();
	}
}

// Handle screen resizing
void BoxBlur::resize(unsigned int width, unsigned int height) const {
	_intermediate->resize(width, height);
}

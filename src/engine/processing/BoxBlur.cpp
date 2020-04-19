#include "processing/BoxBlur.hpp"
#include "graphics/GLUtilities.hpp"

BoxBlur::BoxBlur(TextureShape shape, unsigned int width, unsigned int height, unsigned int depth, const Descriptor & descriptor, bool approximate) {

	if(shape != TextureShape::D2 && shape != TextureShape::Array2D){
		Log::Error() << "Unsupported texture shape for blurring.";
		return;
	}
	// Enforce linear filtering.
	const Descriptor linearDescriptor(descriptor.typedFormat(), Filter::LINEAR_NEAREST, descriptor.wrapping());


	std::string blur_type_name = "box-blur-";;
	if(approximate){
		blur_type_name.append("approx-");
	}
	blur_type_name.append(shape == TextureShape::Array2D ? "array-2d" : "2d");

	_blurProgram = Resources::manager().getProgram2D(blur_type_name);
	// Create one framebuffer.
	_finalFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(shape, width, height, depth, 1, {linearDescriptor}, false));
	// Final combining buffer.
	_finalTexture = _finalFramebuffer->textureId();

	checkGLError();
}

// Draw function
void BoxBlur::process(const Texture * textureId) const {
	_finalFramebuffer->setViewport();
	_blurProgram->use();
	for(size_t lid = 0; lid < _finalFramebuffer->depth(); ++lid){
		_finalFramebuffer->bind(lid);
		GLUtilities::clearColor(glm::vec4(0.0f));
		_blurProgram->uniform("layer", int(lid));
		ScreenQuad::draw(textureId);
	}
	_finalFramebuffer->unbind();
}

// Clean function
void BoxBlur::clean() const {
	_finalFramebuffer->clean();
}

// Handle screen resizing
void BoxBlur::resize(unsigned int width, unsigned int height) const {
	_finalFramebuffer->resize(width, height);
}

void BoxBlur::clear() const {
	_finalFramebuffer->setViewport();
	for(size_t lid = 0; lid < _finalFramebuffer->depth(); ++lid){
		_finalFramebuffer->bind(lid);
		GLUtilities::clearColor(glm::vec4(1.0f));
	}
	_finalFramebuffer->unbind();
}

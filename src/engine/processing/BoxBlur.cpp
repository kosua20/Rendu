#include "processing/BoxBlur.hpp"
#include "graphics/GLUtilities.hpp"

BoxBlur::BoxBlur(unsigned int width, unsigned int height, bool approximate, const Descriptor & descriptor) {

	// Enforce linear filtering.
	const Descriptor linearDescriptor(descriptor.typedFormat(), Filter::LINEAR_NEAREST, descriptor.wrapping());
	const unsigned int channels = linearDescriptor.getChannelsCount();

	std::string blur_type_name = "box-blur-" + (approximate ? std::string("approx-") : "");
	blur_type_name.append(std::to_string(channels));

	_blurProgram = Resources::manager().getProgram2D(blur_type_name);
	// Create one framebuffer.
	_finalFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, linearDescriptor, false));
	// Final combining buffer.
	_finalTexture = _finalFramebuffer->textureId();

	checkGLError();
}

// Draw function
void BoxBlur::process(const Texture * textureId) const {
	_finalFramebuffer->bind();
	_finalFramebuffer->setViewport();
	GLUtilities::clearColor(glm::vec4(0.0f));
	_blurProgram->use();
	ScreenQuad::draw(textureId);
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
	_finalFramebuffer->bind();
	_finalFramebuffer->setViewport();
	GLUtilities::clearColor(glm::vec4(1.0f));
	_finalFramebuffer->unbind();
}

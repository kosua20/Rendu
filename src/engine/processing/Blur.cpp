#include "processing/Blur.hpp"

Blur::Blur() {
	_passthroughProgram = Resources::manager().getProgram("passthrough");
	_finalTexture		= nullptr;
}

void Blur::draw() const {
	_passthroughProgram->use();
	ScreenQuad::draw(_finalTexture);
}

const Texture * Blur::texture() const {
	return _finalTexture;
}

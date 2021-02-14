#include "graphics/ScreenQuad.hpp"
#include "graphics/GPU.hpp"

void ScreenQuad::draw() {
	GPU::drawQuad();
}

void ScreenQuad::draw(const Texture & texture) {
	GPU::bindTexture(&texture, 0);
	draw();
}

void ScreenQuad::draw(const Texture * texture) {
	GPU::bindTexture(texture, 0);
	draw();
}

void ScreenQuad::draw(const std::vector<const Texture *> & textures) {
	for(size_t i = 0; i < textures.size(); ++i) {
		GPU::bindTexture(textures[i], i);
	}
	draw();
}

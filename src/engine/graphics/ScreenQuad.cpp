#include "graphics/ScreenQuad.hpp"
#include "graphics/GLUtilities.hpp"

void ScreenQuad::draw() {
	GLUtilities::drawQuad();
}

void ScreenQuad::draw(const Texture & texture) {
	GLUtilities::bindTexture(&texture, 0);
	draw();
}

void ScreenQuad::draw(const Texture * texture) {
	GLUtilities::bindTexture(texture, 0);
	draw();
}

void ScreenQuad::draw(const std::vector<const Texture *> & textures) {
	for(size_t i = 0; i < textures.size(); ++i) {
		GLUtilities::bindTexture(textures[i], i);
	}
	draw();
}

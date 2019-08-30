#include "graphics/ScreenQuad.hpp"
#include "graphics/GLUtilities.hpp"

GLuint ScreenQuad::_vao = 0;
bool ScreenQuad::_init  = false;

void ScreenQuad::draw() {
	if(!_init) {
		// Generate an empty VAO (imposed by the OpenGL spec).
		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);
		glBindVertexArray(0);
		_init = true;
	}
	// Draw with an empty VAO (mandatory)
	glBindVertexArray(_vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
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

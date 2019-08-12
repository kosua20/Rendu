#include "graphics/ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"


GLuint ScreenQuad::_vao = 0;
bool ScreenQuad::_init = false;



void ScreenQuad::draw() {
	if(!_init){
		// Generate an empty VAO (imposed by the OpenGL spec).
		glGenVertexArrays (1, &_vao);
		glBindVertexArray(_vao);
		glBindVertexArray(0);
		_init = true;
	}
	// Draw with an empty VAO (mandatory)
	glBindVertexArray(_vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void ScreenQuad::draw(const GLuint textureId) {
	// Active screen texture.
	glActiveTexture(GL_TEXTURE0 );
	glBindTexture(GL_TEXTURE_2D, textureId);
	draw();
}

void ScreenQuad::draw(const std::vector<GLuint> & textures) {
	// Active screen textures.
	for(GLuint i = 0; i < textures.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textures[i]);
	}
	draw();
}




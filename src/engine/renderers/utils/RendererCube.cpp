#include "RendererCube.hpp"
#include "input/Input.hpp"
#include "graphics/GLUtilities.hpp"



RendererCube::RendererCube(RenderingConfig & config, const std::string & cubemapName, const std::string & shaderName, const unsigned int width, const unsigned int height, const GLenum preciseFormat) : Renderer(config) {
	
	_resultFramebuffer = std::make_shared<Framebuffer>(width, height, preciseFormat, false);
	
	_program = Resources::manager().getProgram(shaderName, "skybox_basic", shaderName);
	_mesh = Resources::manager().getMesh("skybox");
	_texture = Resources::manager().getCubemap(cubemapName, {GL_RGB32F, GL_LINEAR, GL_CLAMP_TO_EDGE});
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	checkGLError();
	
}


void RendererCube::draw() {
	drawCube(_resultFramebuffer->width(), _resultFramebuffer->height(), "cubemap-output-default");
}

void RendererCube::drawCube(const unsigned int localWidth, const unsigned int localHeight, const std::string & localOutputPath) {
	glDisable(GL_DEPTH_TEST);
	
	_resultFramebuffer->bind();

	glViewport(0,0,localWidth,localHeight);
	
	const glm::mat4 projection = glm::perspective(float(M_PI/2.0), (float)_resultFramebuffer->width()/(float)_resultFramebuffer->height(), 0.1f, 200.0f);
	const glm::vec3 ups[6] = { glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0),glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0) };
	const glm::vec3 centers[6] = { glm::vec3(1.0,0.0,0.0), glm::vec3(-1.0,0.0,0.0), glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0), glm::vec3(0.0,1.0,0.0), glm::vec3(0.0,-1.0,0.0) };
	const std::string suffixes[6] = { "px", "nx", "pz", "nz", "py", "ny"};

	// Loop over the faces. Instead we could use a geometry shader and multiple output layers, one for each face with the corresponding view transformation.
	for(size_t i = 0; i < 6; ++i){
		
		const glm::mat4 view = glm::lookAt(glm::vec3(0.0f,0.0f,0.0f), centers[i], ups[i]);
		
		glClearColor(0.0f,0.0f,i,0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		const glm::mat4 MVP = projection * view;
		
		// Select the program (and shaders).
		glUseProgram(_program->id());
		glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(_texture.cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, _texture.id);
		glBindVertexArray(_mesh.vId);
		glDrawElements(GL_TRIANGLES, _mesh.count, GL_UNSIGNED_INT, (void*)0);
		glBindVertexArray(0);
		
		glFlush();
		glFinish();
		
		const std::string outputPathComplete = localOutputPath + "-" + suffixes[i];
		GLUtilities::saveFramebuffer(_resultFramebuffer, localWidth, localHeight, outputPathComplete, false);
		
	}
	
	_resultFramebuffer->unbind();
	
	glEnable(GL_DEPTH_TEST);
	
}

void RendererCube::update(){
	Renderer::update();
	// Update nothing.
}

void RendererCube::physics(double fullTime, double frameTime){
	// Simulate nothing.
}


void RendererCube::clean() {
	Renderer::clean();
	// Clean objects.
	_mesh.clean();
	glDeleteTextures(1, &(_texture.id));
	_resultFramebuffer->clean();
	
}


void RendererCube::resize(unsigned int width, unsigned int height){
	// Do nothing.
}




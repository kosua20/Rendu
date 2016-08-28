#include <stdio.h>
#include <iostream>
#include <vector>
// glm additional header to generate transformation matrices directly.
#include <glm/gtc/matrix_transform.hpp>
#include <cstring> // For memcopy depending on the platform.

#include "helpers/ProgramUtilities.h"
#include "Renderer.h"

Renderer::Renderer(){}

Renderer::~Renderer(){}

void Renderer::init(int width, int height){

	// Initialize the timer and pingpong.
	_timer = glfwGetTime();
	_pingpong = 0;
	// Setup projection matrix.
	_camera.screen(width, height);
	
	// Setup the framebuffer.
	_lightFramebuffer = Framebuffer(1024, 1024);
	_lightFramebuffer.setup(GL_RG,GL_FLOAT,GL_LINEAR,GL_CLAMP_TO_BORDER);
	_blurFramebuffer = Framebuffer(1024, 1024);
	_blurFramebuffer.setup(GL_RG,GL_FLOAT,GL_LINEAR,GL_CLAMP_TO_BORDER);
	
	_sceneFramebuffer = Framebuffer(_camera._renderSize[0],_camera._renderSize[1]);
	_sceneFramebuffer.setup(GL_RGBA,GL_UNSIGNED_BYTE,GL_LINEAR,GL_CLAMP_TO_EDGE);
	_fxaaFramebuffer = Framebuffer(_camera._renderSize[0],_camera._renderSize[1]);
	_fxaaFramebuffer.setup(GL_RGBA,GL_UNSIGNED_BYTE,GL_LINEAR,GL_CLAMP_TO_EDGE);
	
	// Query the renderer identifier, and the supported OpenGL version.
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "Renderer: " << renderer << std::endl;
	std::cout << "OpenGL version supported: " << version << std::endl;
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	checkGLError();
	
	
	// Setup light
	_light = Light(glm::vec4(0.0f),glm::vec4(0.3f, 0.3f, 0.3f, 0.0f), glm::vec4(0.8f, 0.8f,0.8f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), 25.0f, glm::ortho(-0.75f,0.75f,-0.75f,0.75f,2.0f,6.0f));
	// position will be updated at each frame
	//glm::perspective(45.0f, 1.0f, 1.0f, 5.f); depending on the type of light, one might prefer to use one or the other matrix.
	
	// Generate the buffer.
	glGenBuffers(1, &_ubo);
	// Bind the buffer.
	glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
	
	// We need to know the alignment size if we want to store two uniform blocks in the same uniform buffer.
	GLint uboAlignSize = 0;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboAlignSize);
	// Compute the padding for the second block, it needs to be a multiple of uboAlignSize (typically uboAlignSize will be 256.)
	GLuint lightSize = 4*sizeof(glm::vec4) + 1*sizeof(float);
	_padding = ((lightSize/uboAlignSize)+1)*uboAlignSize;
	
	// Allocate enough memory to hold two copies of the Light struct.
	glBufferData(GL_UNIFORM_BUFFER, _padding + lightSize, NULL, GL_DYNAMIC_DRAW);
	
	// Bind the range allocated to the first version of the light.
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, _ubo, 0, lightSize);
	// Submit the data.
	glBufferSubData(GL_UNIFORM_BUFFER, 0, lightSize, _light.getStruct());
	
	// Bind the range allocated to the second version of the light.
	glBindBufferRange(GL_UNIFORM_BUFFER, 1, _ubo, _padding, lightSize);
	// Submit the data.
	glBufferSubData(GL_UNIFORM_BUFFER, _padding, lightSize, _light.getStruct());
	
	glBindBuffer(GL_UNIFORM_BUFFER,0);
	
	// Initialize objects.
	_suzanne.init(_blurFramebuffer.textureId());
	_dragon.init(_blurFramebuffer.textureId());
	_plane.init(_blurFramebuffer.textureId());
	_skybox.init();
	_blurScreen.init(_lightFramebuffer.textureId(), "ressources/shaders/boxblur");
	_fxaaScreen.init(_sceneFramebuffer.textureId(), "ressources/shaders/fxaa");
	_finalScreen.init(_fxaaFramebuffer.textureId(), "ressources/shaders/final_screenquad");
	checkGLError();
	
	
	
}


void Renderer::draw(){
	
	// Compute the time elapsed since last frame
	float elapsed = glfwGetTime() - _timer;
	_timer = glfwGetTime();
	
	// Physics simulation
	physics(elapsed);

	// Update the light position (in view space).
	// Bind the buffer.
	glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
	// Obtain a handle to the underlying memory.
	// We force the GPU to consider the memory region as unsynchronized:
	// even if it is used, it will be overwritten. As we alternate between
	// two sub-buffers indices ("ping-ponging"), we won't risk writing over
	// a currently used value.
	GLvoid * ptr = glMapBufferRange(GL_UNIFORM_BUFFER,_pingpong*_padding,sizeof(glm::vec4),GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	// Copy the light position.
	std::memcpy(ptr, _light.getStruct(), sizeof(glm::vec4));
	// Unmap, unbind.
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	
	// --- Light pass -------
	
	// Draw the scene inside the framebuffer.
	_lightFramebuffer.bind();
	glViewport(0, 0, _lightFramebuffer._width, _lightFramebuffer._height);
	
	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Draw objects.
	_suzanne.drawDepth(elapsed, _light._mvp);
	_dragon.drawDepth(elapsed, _light._mvp);
	_plane.drawDepth(elapsed, _light._mvp);
	
	// Unbind the shadow map framebuffer.
	_lightFramebuffer.unbind();
	// ----------------------
	
	// --- Blur pass --------
	glDisable(GL_DEPTH_TEST);
	// Bind the post-processing framebuffer.
	_blurFramebuffer.bind();
	// Set screen viewport.
	glViewport(0,0,_blurFramebuffer._width, _blurFramebuffer._height);
	
	// Draw the fullscreen quad
	_blurScreen.draw( 1.0f / _camera._renderSize);
	
	_blurFramebuffer.unbind();
	glEnable(GL_DEPTH_TEST);
	// ----------------------
	
	// --- Scene pass -------
	// Bind the full scene framebuffer.
	_sceneFramebuffer.bind();
	// Set screen viewport
	glViewport(0,0,_sceneFramebuffer._width,_sceneFramebuffer._height);
	
	// Clear the depth buffer (we know we will draw everywhere, no need to clear color.
	glClear(GL_DEPTH_BUFFER_BIT);
	
	// Draw objects
	_suzanne.draw(elapsed, _camera._view, _camera._projection, _pingpong);
	_dragon.draw(elapsed, _camera._view, _camera._projection, _pingpong);
	_plane.draw(elapsed, _camera._view, _camera._projection, _pingpong);
	_skybox.draw(elapsed, _camera._view, _camera._projection);
	
	// Unbind the full scene framebuffer.
	_sceneFramebuffer.unbind();
	// ----------------------
	
	glDisable(GL_DEPTH_TEST);
	// --- FXAA pass -------
	// Bind the post-processing framebuffer.
	_fxaaFramebuffer.bind();
	// Set screen viewport.
	glViewport(0,0,_fxaaFramebuffer._width, _fxaaFramebuffer._height);
	
	// Draw the fullscreen quad
	_fxaaScreen.draw( 1.0f / _camera._renderSize);
	
	_fxaaFramebuffer.unbind();
	// ----------------------
	
	
	// --- Final pass -------
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	glEnable(GL_FRAMEBUFFER_SRGB);
	
	// Set screen viewport.
	glViewport(0,0,_camera._screenSize[0],_camera._screenSize[1]);
	
	// Draw the fullscreen quad
	_finalScreen.draw( 1.0f / _camera._screenSize);
	
	glDisable(GL_FRAMEBUFFER_SRGB);
	// ----------------------
	glEnable(GL_DEPTH_TEST);
	
	// Update timer
	_timer = glfwGetTime();
	// Update pingpong
	_pingpong = (_pingpong + 1)%2;
}

void Renderer::physics(float elapsedTime){
	_camera.update(elapsedTime);
	_light.update(_timer, _camera._view);
}


void Renderer::clean(){
	// Clean objects.
	_suzanne.clean();
	_dragon.clean();
	_plane.clean();
	_skybox.clean();
	_blurScreen.clean();
	_fxaaScreen.clean();
	_finalScreen.clean();
	_lightFramebuffer.clean();
	_blurFramebuffer.clean();
	_sceneFramebuffer.clean();
	_fxaaFramebuffer.clean();
}


void Renderer::resize(int width, int height){
	//Update the size of the viewport.
	glViewport(0, 0, width, height);
	// Update the projection matrix.
	_camera.screen(width, height);
	// Resize the framebuffer.
	_sceneFramebuffer.resize(_camera._renderSize);
	_fxaaFramebuffer.resize(_camera._renderSize);
}

void Renderer::keyPressed(int key, int action){
	if(action == GLFW_PRESS){
		_camera.key(key, true);
	} else if(action == GLFW_RELEASE) {
		_camera.key(key, false);
	}
}

void Renderer::buttonPressed(int button, int action, double x, double y){
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			_camera.mouse(MouseMode::Start,x, y);
		} else if (action == GLFW_RELEASE) {
			_camera.mouse(MouseMode::End, 0.0, 0.0);
		}
	} else {
		std::cout << "Button: " << button << ", action: " << action << std::endl;
	}
}

void Renderer::mousePosition(int x, int y, bool leftPress, bool rightPress){
	if (leftPress){
		_camera.mouse(MouseMode::Move, float(x), float(y));
    }
}




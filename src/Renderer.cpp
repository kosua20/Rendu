#include <stdio.h>
#include <iostream>
#include <vector>
#include <lodepng/lodepng.h>
// glm additional header to generate transformation matrices directly.
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/ProgramUtilities.h"
#include "helpers/MeshUtilities.h"

#include "Renderer.h"

Renderer::Renderer(){}

Renderer::~Renderer(){}

void Renderer::init(int width, int height){
	_width = width;
	_height = height;

	updateProjectionMatrix();

	// initialize the timer
	_timer = glfwGetTime();


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
	
	// Load the shaders
	_programId = createGLProgram("ressources/shaders/prog2.vert","ressources/shaders/prog2.frag");

	// Load geometry.
	mesh_t mesh;
	loadObj("ressources/suzanne.obj",mesh,Indexed);
	centerAndUnitMesh(mesh);

	_count = mesh.indices.size();

	// Create an array buffer to host the geometry data.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.positions.size() * 3, &(mesh.positions[0]), GL_STATIC_DRAW);

	GLuint vbo_nor = 0;
	glGenBuffers(1, &vbo_nor);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.normals.size() * 3, &(mesh.normals[0]), GL_STATIC_DRAW);

	// Generate a vertex array (useful when we add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// The second attribute will be the normals.
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// We load the indices data
	glGenBuffers(1, &_ebo);
 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
 	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.indices.size(), &(mesh.indices[0]), GL_STATIC_DRAW);

	glBindVertexArray(0);

	// Setup light
	_light.position = glm::vec4(0.0f); // position will be updated at each frame
	_light.shininess = 200.0f;
	_light.Ia = glm::vec4(0.1f,0.1f,0.1f,0.0f);
	_light.Id = glm::vec4(0.8f,0.1f,0.9f,0.0f);
	_light.Is = glm::vec4(0.4f, 0.4f, 0.4f,0.0f);

	// Get a binding point for the Uniform buffer.
	GLuint lightUniformId = glGetUniformBlockIndex(_programId, "Light");  
	glUniformBlockBinding(_programId, lightUniformId, 0);
	// Generate the buffer.
	glGenBuffers(1, &_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
	// Allocate enough memory to hold the Light struvt
	glBufferData(GL_UNIFORM_BUFFER, 4*sizeof(glm::vec4) + sizeof(float), NULL, GL_DYNAMIC_DRAW);
	// Bind the allocated range.
  	glBindBufferRange(GL_UNIFORM_BUFFER, 0, _ubo, 0, 4*sizeof(glm::vec4) + sizeof(float));
  	// And bind the buffer.
  	glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
  	// Submit the data.
	glBufferSubData(GL_UNIFORM_BUFFER, 0, 4*sizeof(glm::vec4) + sizeof(float), &_light);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);  
	checkGLError();

}


void Renderer::draw(){

	// Compute the time elapsed since last frame
	float elapsed = glfwGetTime() - _timer;
	_timer = glfwGetTime();

	// Physics simulation
	physics(elapsed);

	// Scale the model by 0.5.
	glm::mat4 model = glm::scale(glm::mat4(1.0f),glm::vec3(0.5f));

	// Combine the three matrices.
	glm::mat4 MV = _camera._view * model;
	glm::mat4 MVP = _projection * MV;

	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Select the program (and shaders).
	glUseProgram(_programId);

	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
	// Upload the MV matrix.
	GLuint mvID  = glGetUniformLocation(_programId, "mv");
	glUniformMatrix4fv(mvID, 1, GL_FALSE, &MV[0][0]);
	// Upload the normal matrix.
	GLuint normalMatrixID  = glGetUniformLocation(_programId, "normalMatrix");
	glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);

	// Update the light position (in view space).
	// Bind the buffer.
	glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
	// Obtain a handle to the underlying memory.
	GLvoid * ptr = glMapBuffer(GL_UNIFORM_BUFFER,GL_WRITE_ONLY);
	// Copy the light position.
	memcpy(ptr, &(_light.position[0]), sizeof(glm::vec4));
	// Unmap, unbind.
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);

	// Update timer
	_timer = glfwGetTime();
}

void Renderer::physics(float elapsedTime){
	_camera.update(elapsedTime);
	// Compute the light position in view space
	_light.position = _camera._view * glm::vec4(4.0f,4.0f,1.0f,1.0f);
}


void Renderer::clean(){
	glDeleteVertexArrays(1, &_vao);
	glDeleteTextures(1, &_tex);
	glDeleteProgram(_programId);
}


void Renderer::resize(int width, int height){
	_width = width;
	_height = height;
	//Update the size of the viewport.
	glViewport(0, 0, width, height);
	updateProjectionMatrix();
}

void Renderer::keyPressed(int key, int action){
	if(action == GLFW_PRESS){
		if (key == GLFW_KEY_W || key == GLFW_KEY_A || key == GLFW_KEY_S || key == GLFW_KEY_D || key == GLFW_KEY_Q || key == GLFW_KEY_E){
			_camera.registerMove(key, true);

		} else if(key == GLFW_KEY_R) {
			_camera.reset();

		} else {
			std::cout << "Key: " << key << " (" << char(key) << ")." << std::endl;
		}
	} else if(action == GLFW_RELEASE) {
		if (key == GLFW_KEY_W || key == GLFW_KEY_A || key == GLFW_KEY_S || key == GLFW_KEY_D || key == GLFW_KEY_Q || key == GLFW_KEY_E){
			_camera.registerMove(key, false);
		}
	}
}

void Renderer::buttonPressed(int button, int action, double x, double y){
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			// We normalize the x and y values to the [-1, 1] range.
			float xPosition =  fmax(fmin(1.0f,2.0f * (float)x / _width - 1.0),-1.0f);
			float yPosition =  fmax(fmin(1.0f,2.0f * (float)y / _height - 1.0),-1.0f);
			_camera.startLeftMouse(xPosition, yPosition);
			return;
		} else if (action == GLFW_RELEASE) {
			_camera.endLeftMouse();
		}
	}
	std::cout << "Button: " << button << ", action: " << action << std::endl;            
}

void Renderer::mousePosition(int x, int y, bool leftPress, bool rightPress){
	if (leftPress){
		// We normalize the x and y values to the [-1, 1] range.
        float xPosition =  fmax(fmin(1.0f,2.0f * (float)x / _width - 1.0),-1.0f);
		float yPosition =  fmax(fmin(1.0f,2.0f * (float)y / _height - 1.0),-1.0f);
		_camera.leftMouseTo(xPosition, yPosition); 
    }
}

void Renderer::updateProjectionMatrix(){
	// Perspective projection.
	_projection = glm::perspective(45.0f, float(_width) / float(_height), 0.1f, 100.f);
}




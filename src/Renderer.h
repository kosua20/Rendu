#ifndef Renderer_h
#define Renderer_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Camera.h"

class Renderer {

public:

	Renderer();

	~Renderer();

	/// Init function
	void init(int width, int height);

	/// Draw function
	void draw();

	void physics(float elapsedTime);

	/// Clean function
	void clean();

	/// Handle screen resizing
	void resize(int width, int height);

	/// Handle keyboard inputs
	void keyPressed(int key, int action);

	/// Handle mouse inputs
	void buttonPressed(int button, int action, double x, double y);

	void mousePosition(int x, int y, bool leftPress, bool rightPress);

	/// Update projection matrix
	void updateProjectionMatrix();

private:

	int _width;
	int _height;

	float _timer;

	GLuint _programId;
	GLuint _vao;
	GLuint _tex;

	glm::mat4 _projection;

	Camera _camera;

};

#endif

#ifndef Renderer_h
#define Renderer_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Camera.h"

struct Light
{
	glm::vec4 position;
	glm::vec4 Ia;
	glm::vec4 Id;
	glm::vec4 Is;
	float shininess;
};

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
	int _count;

	float _timer;

	GLuint _programId;
	GLuint _vao;
	GLuint _ebo;
	GLuint _tex;
	GLuint _ubo;

	glm::mat4 _projection;

	Camera _camera;

	Light _light;

};

#endif

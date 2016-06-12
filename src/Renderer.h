#ifndef Renderer_h
#define Renderer_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Framebuffer.h"
#include "camera/Camera.h"
#include "Suzanne.h"
#include "Skybox.h"
#include "Dragon.h"
#include "ScreenQuad.h"

struct Light
{
	glm::vec4 position;
	glm::vec4 Ia;
	glm::vec4 Id;
	glm::vec4 Is;
	float shininess;
};

struct Material
{
	glm::vec4 Ka;
	glm::vec4 Kd;
	glm::vec4 Ks;
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


private:
	
	float _timer;

	GLuint _ubo;
	
	Camera _camera;

	Light _light;
	Material _material;
	
	Suzanne _suzanne;
	Dragon _dragon;
	Skybox _skybox;
	ScreenQuad _screen;
	
	Framebuffer _framebuffer;

};

#endif

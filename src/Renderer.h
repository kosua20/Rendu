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
#include "Plane.h"
#include "ScreenQuad.h"
#include "Light.h"



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

	
	
	Suzanne _suzanne;
	Dragon _dragon;
	Skybox _skybox;
	Plane _plane;
	Framebuffer _lightFramebuffer;
	Framebuffer _blurFramebuffer;
	Framebuffer _sceneFramebuffer;
	Framebuffer _fxaaFramebuffer;
	ScreenQuad _blurScreen;
	ScreenQuad _fxaaScreen;
	ScreenQuad _finalScreen;
	size_t _pingpong;
	GLuint _padding;


	Light _light;
	
};

#endif

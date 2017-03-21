#ifndef Renderer_h
#define Renderer_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>

#include "Framebuffer.h"
#include "Gbuffer.h"
#include "camera/Camera.h"
#include "Object.h"
#include "Skybox.h"
#include "ScreenQuad.h"
#include "GbufferQuad.h"
#include "Light.h"



struct Material
{
	glm::vec4 Ka;
	glm::vec4 Kd;
	glm::vec4 Ks;
};


class Renderer {

public:


	~Renderer();

	/// Init function
	Renderer(int width, int height);

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

	Object _suzanne;
	Object _dragon;
	Skybox _skybox;
	Object _plane;
	
	std::shared_ptr<Framebuffer> _lightFramebuffer;
	std::shared_ptr<Framebuffer> _blurFramebuffer;
	std::shared_ptr<Gbuffer> _gbuffer;
	std::shared_ptr<Framebuffer> _sceneFramebuffer;
	std::shared_ptr<Framebuffer> _fxaaFramebuffer;
	
	GbufferQuad _gbufferScreen;
	ScreenQuad _blurScreen;
	ScreenQuad _fxaaScreen;
	ScreenQuad _finalScreen;
	
	GLuint _pingpong;
	GLuint _padding;

	Light _light;
	
};

#endif

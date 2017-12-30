#ifndef Renderer_h
#define Renderer_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "helpers/GenerationUtilities.hpp"
#include "Config.hpp"
#include "scenes/Scene.hpp"
#include "Framebuffer.hpp"
#include "Gbuffer.hpp"
#include "Blur.hpp"
#include "AmbientQuad.hpp"
#include "input/Camera.hpp"
#include "ScreenQuad.hpp"

class Renderer {

public:

	~Renderer();

	/// Init function
	Renderer(Config & config, std::shared_ptr<Scene> & scene);

	/// Draw function
	void draw();

	void update(double fullTime, double frameTime);

	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(int width, int height);

	
private:
	
	Config & _config;
	
	Camera _camera;

	std::shared_ptr<Gbuffer> _gbuffer;
	std::shared_ptr<Blur> _blurBuffer;
	std::shared_ptr<Framebuffer> _ssaoFramebuffer;
	std::shared_ptr<Framebuffer> _ssaoBlurFramebuffer;
	std::shared_ptr<Framebuffer> _sceneFramebuffer;
	std::shared_ptr<Framebuffer> _bloomFramebuffer;
	std::shared_ptr<Framebuffer> _toneMappingFramebuffer;
	std::shared_ptr<Framebuffer> _fxaaFramebuffer;
	
	AmbientQuad _ambientScreen;
	ScreenQuad _ssaoBlurScreen;
	ScreenQuad _bloomScreen;
	ScreenQuad _toneMappingScreen;
	ScreenQuad _fxaaScreen;
	ScreenQuad _finalScreen;

	std::shared_ptr<Scene> _scene;
	glm::vec2 _renderResolution;
	
};

#endif

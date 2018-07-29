#ifndef DeferredRenderer_h
#define DeferredRenderer_h
#include "../../Framebuffer.hpp"
#include "../../input/ControllableCamera.hpp"
#include "../../ScreenQuad.hpp"

#include "../../GaussianBlur.hpp"
#include "../../BoxBlur.hpp"

#include "../Renderer.hpp"

#include "Gbuffer.hpp"
#include "AmbientQuad.hpp"

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>



class DeferredRenderer : public Renderer {

public:

	~DeferredRenderer();

	/// Init function
	DeferredRenderer(Config & config);

	void setScene(std::shared_ptr<Scene> & scene);
	
	/// Draw function
	void draw();
	
	void update();
	
	void physics(double fullTime, double frameTime);

	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(unsigned int width, unsigned int height);
	
	
private:
	
	ControllableCamera _userCamera;

	std::shared_ptr<Gbuffer> _gbuffer;
	std::shared_ptr<GaussianBlur> _blurBuffer;
	std::shared_ptr<BoxBlur> _blurSSAOBuffer;
	
	std::shared_ptr<Framebuffer> _ssaoFramebuffer;
	std::shared_ptr<Framebuffer> _sceneFramebuffer;
	std::shared_ptr<Framebuffer> _bloomFramebuffer;
	std::shared_ptr<Framebuffer> _toneMappingFramebuffer;
	std::shared_ptr<Framebuffer> _fxaaFramebuffer;
	
	AmbientQuad _ambientScreen;
	ScreenQuad _bloomScreen;
	ScreenQuad _toneMappingScreen;
	ScreenQuad _fxaaScreen;
	ScreenQuad _finalScreen;
	
	std::shared_ptr<Scene> _scene;
	
};

#endif

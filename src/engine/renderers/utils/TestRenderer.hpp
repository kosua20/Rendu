#ifndef TestRenderer_h
#define TestRenderer_h
#include "../../Framebuffer.hpp"
#include "../../input/Camera.hpp"
#include "../../ScreenQuad.hpp"

#include "../../processing/GaussianBlur.hpp"
#include "../../processing/BoxBlur.hpp"

#include "../Renderer.hpp"

#include "../deferred/Gbuffer.hpp"
#include "../deferred/AmbientQuad.hpp"

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>



class TestRenderer : public Renderer {

public:

	~TestRenderer();

	/// Init function
	TestRenderer(Config & config);

	/// Draw function
	void draw();
	
	void update();
	
	void physics(double fullTime, double frameTime);

	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(unsigned int width, int unsigned height);
	
	
private:
	
	Camera _camera;

	std::shared_ptr<Framebuffer> _framebuffer;
	
	
	ScreenQuad _screenQuad;
	
};

#endif

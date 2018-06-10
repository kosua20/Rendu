#ifndef Renderer2D_h
#define Renderer2D_h
#include "../../Framebuffer.hpp"
#include "../../ScreenQuad.hpp"


#include "../Renderer.hpp"

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>



class Renderer2D : public Renderer {

public:

	~Renderer2D();

	/// Init function
	Renderer2D(Config & config, const std::string & shaderName, const int width, const int height, const GLenum format, const GLenum type, const GLenum preciseFormat);

	/// Draw function
	void draw();
	
	void update();
	
	void physics(double fullTime, double frameTime);
	
	/// Save result to disk.
	void save(const std::string & outputPath);

	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(int width, int height);
	
	
private:
	
	std::shared_ptr<Framebuffer> _resultFramebuffer;
	ScreenQuad _resultScreen;
	
};

#endif

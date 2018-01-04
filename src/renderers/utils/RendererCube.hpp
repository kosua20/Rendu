#ifndef RendererCube_h
#define RendererCube_h
#include "../../Framebuffer.hpp"
#include "../../Object.hpp"


#include "../Renderer.hpp"

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>



class RendererCube : public Renderer {

public:

	~RendererCube();

	/// Init function
	RendererCube(Config & config, const std::string & cubemapName, const std::string & shaderName, const int width, const int height, const GLenum format, const GLenum type, const GLenum preciseFormat, const std::string & outputPath);

	/// Draw function
	void draw();
	
	void update();
	
	void physics(double fullTime, double frameTime);

	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(int width, int height);
	
	
private:
	
	std::shared_ptr<Framebuffer> _resultFramebuffer;
	std::shared_ptr<ProgramInfos> _program;
	Object _cubemap;
	std::string _outputPath;
	
};

#endif

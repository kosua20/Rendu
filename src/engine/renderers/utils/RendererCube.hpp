#ifndef RendererCube_h
#define RendererCube_h

#include "../../Common.hpp"
#include "../../Framebuffer.hpp"
#include "../../Object.hpp"
#include "../Renderer.hpp"



class RendererCube : public Renderer {

public:

	~RendererCube();

	/// Init function
	RendererCube(Config & config, const std::string & cubemapName, const std::string & shaderName, const unsigned int width, const unsigned int height, const GLenum format, const GLenum type, const GLenum preciseFormat);

	/// Draw function
	void draw();
	
	void drawCube(const unsigned int localWidth, const unsigned int localHeight, const std::string & localOutputPath);
	
	void update();
	
	void physics(double fullTime, double frameTime);

	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(unsigned int width, unsigned  int height);
	
	
private:
	
	
	
	std::shared_ptr<Framebuffer> _resultFramebuffer;
	std::shared_ptr<ProgramInfos> _program;
	Object _cubemap;
	std::string _outputPath;
	
};

#endif

#ifndef Renderer2D_h
#define Renderer2D_h

#include "../../Common.hpp"
#include "../../Framebuffer.hpp"
#include "../../ScreenQuad.hpp"
#include "../Renderer.hpp"



class Renderer2D : public Renderer {

public:

	~Renderer2D();

	/// Init function
	Renderer2D(Config & config, const std::string & shaderName, const unsigned int width, const unsigned int height, const GLenum format, const GLenum type, const GLenum preciseFormat);

	/// Draw function
	void draw();
	
	void update();
	
	void physics(double fullTime, double frameTime);
	
	/// Save result to disk.
	void save(const std::string & outputPath);

	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(unsigned int width, unsigned int height);
	
	
private:
	
	std::shared_ptr<Framebuffer> _resultFramebuffer;
	std::shared_ptr<ProgramInfos> _resultProgram;
	
};

#endif

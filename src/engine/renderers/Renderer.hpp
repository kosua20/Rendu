#ifndef Renderer_h
#define Renderer_h
#include "../Config.hpp"
#include "../Scene.hpp"



class Renderer {
	
public:
	
	~Renderer();
	
	/// Init function
	Renderer(Config & config);
	
	/// Draw function
	virtual void draw() = 0;
	
	virtual void update();
	
	virtual void physics(double fullTime, double frameTime) = 0;
	
	/// Clean function
	virtual void clean() const;
	
	/// Handle screen resizing
	virtual void resize(unsigned int width, unsigned int height) = 0;
	
	
protected:
	
	void updateResolution(unsigned int width, unsigned int height);
	
	Config & _config;
	glm::vec2 _renderResolution;
	
private:
	
	void defaultGLSetup();
};

#endif


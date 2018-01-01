#ifndef Renderer_h
#define Renderer_h
#include "../Config.hpp"
#include "../scenes/Scene.hpp"

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>



class Renderer {
	
public:
	
	~Renderer();
	
	Renderer(Config & config);
	
	/// Init function
	Renderer(Config & config, std::shared_ptr<Scene> & scene);
	
	/// Draw function
	virtual void draw() = 0;
	
	virtual void update();
	
	virtual void physics(double fullTime, double frameTime) = 0;
	
	/// Clean function
	virtual void clean() const;
	
	/// Handle screen resizing
	virtual void resize(int width, int height) = 0;
	
	
protected:
	
	void updateResolution(int width, int height);
	
	Config & _config;
	std::shared_ptr<Scene> _scene;
	glm::vec2 _renderResolution;
	
private:
	
	void defaultGLSetup();
};

#endif


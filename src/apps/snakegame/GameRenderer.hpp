
#ifndef GameRenderer_h
#define GameRenderer_h
#include "renderers/Renderer.hpp"
#include "input/Camera.hpp"
#include "Player.hpp"

class GameRenderer: public Renderer {
public:
	
	GameRenderer(RenderingConfig & config);
	
	void draw();
	
	void update();
	
	void physics(double fullTime, double frameTime);
	
	void resize(unsigned int width, unsigned int height);
	
	void clean() const;
	
private:
	std::shared_ptr<Framebuffer> _sceneFramebuffer;
	std::shared_ptr<Framebuffer> _fxaaFramebuffer;
	
	
	std::shared_ptr<ProgramInfos> _fxaaProgram; ///< FXAA program
	std::shared_ptr<ProgramInfos> _finalProgram; ///< Final output program
	
	Camera _playerCamera;
	Player _player;
	double _startTime;
};

#endif

#pragma once

#include "renderers/Renderer.hpp"
#include "input/Camera.hpp"
#include "Player.hpp"

class GameRenderer: public Renderer {
public:
	
	GameRenderer(RenderingConfig & config);
	
	void draw(const Player & player);
	
	void draw(){};
	
	void update();
	
	void physics(double fullTime, double frameTime){};
	
	void resize(unsigned int width, unsigned int height);
	
	void clean() const;
	
private:
	std::unique_ptr<Framebuffer> _sceneFramebuffer;
	std::unique_ptr<Framebuffer> _fxaaFramebuffer;
	
	
	std::shared_ptr<ProgramInfos> _fxaaProgram;
	std::shared_ptr<ProgramInfos> _finalProgram;
	std::shared_ptr<ProgramInfos> _coloredProgram;
	
	MeshInfos _head;
	MeshInfos _bodyElement;
	
	Camera _playerCamera;
	
};



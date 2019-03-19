#pragma once

#include "renderers/Renderer.hpp"
#include "input/Camera.hpp"
#include "Player.hpp"
#include "processing/SSAO.hpp"

class GameRenderer: public Renderer {
public:
	
	GameRenderer(RenderingConfig & config);
	
	void draw(const Player & player);
	
	void draw(){};
	
	void update();
	
	void physics(double fullTime, double frameTime){};
	
	void resize(unsigned int width, unsigned int height);
	
	void clean() const;
	
	GLuint finalImage() const ;
	
	glm::vec2 renderingResolution() const;
	
private:
	std::unique_ptr<Framebuffer> _sceneFramebuffer;
	std::unique_ptr<Framebuffer> _lightingFramebuffer;
	std::unique_ptr<Framebuffer> _fxaaFramebuffer;
	std::unique_ptr<SSAO> _ssaoPass;
	
	
	std::shared_ptr<ProgramInfos> _fxaaProgram;
	std::shared_ptr<ProgramInfos> _finalProgram;
	std::shared_ptr<ProgramInfos> _coloredProgram;
	std::shared_ptr<ProgramInfos> _compositingProgram;
	
	MeshInfos _head;
	MeshInfos _bodyElement;
	
	Camera _playerCamera;
	TextureInfos _cubemap;
};



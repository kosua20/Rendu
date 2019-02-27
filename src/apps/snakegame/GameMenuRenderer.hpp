#pragma once

#include "renderers/Renderer.hpp"
#include "GameMenu.hpp"

class GameMenuRenderer: public Renderer {
public:
	
	GameMenuRenderer(RenderingConfig & config);
	
	void draw(const GameMenu & menu);
	
	void draw(){};
	
	void update();
	
	void physics(double fullTime, double frameTime){};
	
	void resize(unsigned int width, unsigned int height);
	
	void clean() const;
	
	glm::vec2 getButtonSize();
	
private:
	
	std::shared_ptr<ProgramInfos> _backgroundProgram;
	std::shared_ptr<ProgramInfos> _buttonProgram;
	std::shared_ptr<ProgramInfos> _imageProgram;
	MeshInfos _button, _buttonIn, _quad;
	
};



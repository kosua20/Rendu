#pragma once
#include "Config.hpp"
#include "GameMenu.hpp"
#include "GameMenuRenderer.hpp"
#include "GameRenderer.hpp"

class Game {
public:
	
	Game(RenderingConfig & config);
	
	void draw();
	
	bool update();
	
	void physics(double fullTime, double frameTime);
	
	void resize(unsigned int width, unsigned int height);
	
	void clean() const;
	
private:
	RenderingConfig & _config;
	GameRenderer _inGameRenderer;
	GameMenuRenderer _menuRenderer;
	
	Player _player;
	double _playTime = 0.0;
	
	enum class Status {
		START, INGAME, PAUSE, DEAD, OPTIONS
	};
	
	Status _status = Status::START;
	
	std::map<Status, GameMenu> _menus;
	
};

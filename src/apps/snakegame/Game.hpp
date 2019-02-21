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
	
	void handleButton(int tag);
	
	RenderingConfig & _config;
	GameRenderer _inGameRenderer;
	GameMenuRenderer _menuRenderer;
	
	std::unique_ptr<Player> _player;
	double _playTime = 0.0;
	
	enum class Status {
		MAINMENU, INGAME, PAUSED, DEAD, OPTIONS
	};
	
	enum Action : int {
		NEWGAME, OPTIONS, QUIT, PAUSE, RESUME, BACKTOMENU, OPTION_FULLSCREEN
	};
	
	Status _status = Status::MAINMENU;
	
	std::map<Status, GameMenu> _menus;
	
};

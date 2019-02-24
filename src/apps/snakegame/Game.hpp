#pragma once
#include "Config.hpp"
#include "GameMenu.hpp"
#include "GameMenuRenderer.hpp"
#include "GameRenderer.hpp"
#include "helpers/InterfaceUtilities.hpp"

class Game {
public:
	
	Game(RenderingConfig & config);
	
	void draw();
	
	Interface::Action update();
	
	void physics(double fullTime, double frameTime);
	
	void resize(unsigned int width, unsigned int height);
	
	void clean() const;
	
	
private:
	
	enum class Status {
		MAINMENU, INGAME, PAUSED, DEAD, OPTIONS
	};
	
	enum ButtonAction : int {
		NEWGAME, OPTIONS, QUIT, PAUSE, RESUME, BACKTOMENU, OPTION_FULLSCREEN
	};
	
	Interface::Action handleButton(const ButtonAction tag);
	
	RenderingConfig & _config;
	GameRenderer _inGameRenderer;
	GameMenuRenderer _menuRenderer;
	
	std::unique_ptr<Player> _player;
	double _playTime = 0.0;
	
	Status _status = Status::MAINMENU;
	
	std::map<Status, GameMenu> _menus;
	
};

#pragma once
#include "GameRenderer.hpp"
#include "Config.hpp"

class Game {
public:
	
	Game(RenderingConfig & config);
	
	void draw();
	
	bool update();
	
	void physics(double fullTime, double frameTime);
	
	void resize(unsigned int width, unsigned int height);
	
	void clean() const;
	
private:
	GameRenderer _inGameRenderer;
	RenderingConfig & _config; ///< The current configuration.
	Player _player;
	double _playTime = 0.0;
	
	enum class Status {
		START, INGAME, PAUSE, DEAD, OPTIONS, TUTORIAL
	};
	
	Status _status = Status::START;
};

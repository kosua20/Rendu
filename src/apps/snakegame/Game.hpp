#pragma once

#include "GameMenu.hpp"
#include "GameMenuRenderer.hpp"
#include "GameRenderer.hpp"
#include "system/Window.hpp"
#include "system/Config.hpp"
#include "processing/GaussianBlur.hpp"
#include <map>

// *** Later improvements: ***
// \todo Add match-3 mechanics.
// \todo High score list or best score display.


/** \brief Game common configuration.
	\ingroup SnakeGame
 */
class GameConfig : public RenderingConfig {
public:

	/** Setup game configuration.
	 \param argv the arguments
	 */
	GameConfig(const std::vector<std::string> & argv);

	/** Save the config to disk at a predetermined path.
	 */
	void save();

	bool lowRes = false; ///< Perform internal rendering at a lower resolution.

};

/**
 \brief Handles communication between the different game components (renderers, player, menus) and the player actions.
 \ingroup SnakeGame
 */
class Game {
public:
	/** Constructor
	 \param config the shared game configuration
	 */
	explicit Game(GameConfig & config);

	/** Draw the game.
	 \param window the destination window
	 */
	void draw(Window & window);

	/** Perform once-per-frame update (button clicks, key presses).
	 \return If any, the action the windowing system should perform.
	 */
	Window::Action update();

	/** Update the physic state of the player snake and items
	 \param frameTime delta time since last physics update
	 */
	void physics(double frameTime);

	/** Resize renderers based on new window size.
	 \param width new width
	 \param height new height
	 */
	void resize(unsigned int width, unsigned int height);

private:
	/**
	 \brief Game state: either a specific menu or in-game.
	 */
	enum class Status {
		MAINMENU,
		INGAME,
		PAUSED,
		DEAD,
		OPTIONS
	};

	/**
	 \brief Action that can be performed by pressing a button or a key.
	 */
	enum ButtonAction : int {
		NEWGAME,
		OPTIONS,
		QUIT,
		PAUSE,
		RESUME,
		BACKTOMENU,
		OPTION_FULLSCREEN,
		OPTION_VSYNC,
		OPTION_HALFRES
	};

	/** For a given button action tag, perform the corresponding internal operations and indicates if any low-level action should be performed by the windowing system/hardware.
	 \param tag the action to perform
	 \return the potential low-level action to perform (fullscreen, vsync,...)
	 */
	Window::Action handleButton(ButtonAction tag);

	GameConfig & _config;		 ///< Reference to the shared game configuration.
	std::unique_ptr<Player> _player; ///< The player state.

	GameRenderer _inGameRenderer;		   ///< In-game renderer.
	GameMenuRenderer _menuRenderer;		   ///< Menus renderer.
	std::unique_ptr<GaussianBlur> _bgBlur; ///< Blurring pass for the paused/dead menus background.
	std::unique_ptr<Framebuffer> _gameFramebuffer; ///< Game scene framebuffer.
	std::unique_ptr<Framebuffer> _bgBlurBuffer; ///< Game scene framebuffer.
	Program * _finalProgram;		   ///< Final upscaling program.

	Status _status = Status::MAINMENU; ///< Current game sattus (specific menu or in-game)
	std::map<Status, GameMenu> _menus; ///< Menus for each game status.

	double _playTime   = 0.0;   ///< Current playtime.
	bool _overrideTime = false; ///< Debug pause.
};

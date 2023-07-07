#include "Game.hpp"
#include "input/Input.hpp"
#include "graphics/GPU.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

GameConfig::GameConfig(const std::vector<std::string> & argv) :
	RenderingConfig(argv) {
	for(const auto & arg : arguments()) {
		const std::string key = arg.key;
		if(key == "low-res") {
			lowRes = true;
		}
	}

	registerSection("Extra game options");
	registerArgument("low-res", "", "Render at lower resolution in-game.");
}

void GameConfig::save(){
	const std::string path = "./config.ini";
	Resources::saveStringToExternalFile(path, "# SnakeGame Config v1.0\n"
			+ std::string(fullscreen ? "fullscreen\n" : "")
			+ std::string(!vsync ? "no-vsync\n" : "")
			+ std::string(lowRes ? "low-res\n" : ""));
}

Game::Game(GameConfig & config) :
	_config(config), _inGameRenderer(_config.screenResolution), _gameResult("Game scene"), _bgBlurTexture("Background blur")  {

	_bgBlur = std::unique_ptr<GaussianBlur>(new GaussianBlur(3, 1, "Background"));
	const uint renderWidth = uint(_config.screenResolution[0]);
	const uint renderHeight = uint(_config.screenResolution[1]);
	const Layout tgtFormat = _inGameRenderer.outputColorFormat();
	_bgBlurTexture.setupAsDrawable(tgtFormat, renderWidth, renderHeight);
	_gameResult.setupAsDrawable(tgtFormat, renderWidth, renderHeight);
	_finalProgram = Resources::manager().getProgram2D("sharpening");

	// Create menus.
	const glm::vec2 meshSize = _menuRenderer.getButtonSize();
	const float displayScale = 0.3f;
	const Font * font		 = Resources::manager().getFont("digits");

	const Layout format = Layout::SRGB8_ALPHA8;
	const Texture * backgroundTexture = Resources::manager().getTexture("menubg", format, Storage::GPU);

	_menus[Status::MAINMENU].backgroundImage = backgroundTexture;
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, -0.10f), meshSize, displayScale, NEWGAME,
		Resources::manager().getTexture("button-newgame", format, Storage::GPU));
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, 0.25f), meshSize, displayScale, OPTIONS,
		Resources::manager().getTexture("button-options", format, Storage::GPU));
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, 0.60f), meshSize, displayScale, QUIT,
		Resources::manager().getTexture("button-quit", format, Storage::GPU));
	_menus[Status::MAINMENU].images.emplace_back(glm::vec2(0.0f, -0.47f), 0.5f,
		Resources::manager().getTexture("title", format, Storage::GPU));

	_menus[Status::PAUSED].backgroundImage = &_bgBlurTexture;
	_menus[Status::PAUSED].buttons.emplace_back(glm::vec2(0.0f, -0.10f), meshSize, displayScale, RESUME,
		Resources::manager().getTexture("button-resume", format, Storage::GPU));
	_menus[Status::PAUSED].buttons.emplace_back(glm::vec2(0.0f, 0.25f), meshSize, displayScale, BACKTOMENU,
		Resources::manager().getTexture("button-menu", format, Storage::GPU));
	_menus[Status::PAUSED].images.emplace_back(glm::vec2(0.0f, -0.47f), 0.5f,
		Resources::manager().getTexture("title-pause", format, Storage::GPU));

	_menus[Status::OPTIONS].backgroundImage = backgroundTexture;

	_menus[Status::OPTIONS].toggles.emplace_back(glm::vec2(0.0f, -0.20f), meshSize, displayScale, OPTION_FULLSCREEN,
		Resources::manager().getTexture("button-fullscreen", format, Storage::GPU));
	_menus[Status::OPTIONS].toggles.back().state = config.fullscreen ? MenuButton::State::ON : MenuButton::State::OFF;
	_menus[Status::OPTIONS].toggles.emplace_back(glm::vec2(0.0f, 0.10f), meshSize, displayScale, OPTION_VSYNC,
		Resources::manager().getTexture("button-vsync", format, Storage::GPU));
	_menus[Status::OPTIONS].toggles.back().state = config.vsync ? MenuButton::State::ON : MenuButton::State::OFF;
	_menus[Status::OPTIONS].toggles.emplace_back(glm::vec2(0.0f, 0.40f), meshSize, displayScale, OPTION_HALFRES,
			Resources::manager().getTexture("button-halfres", format, Storage::GPU));
		_menus[Status::OPTIONS].toggles.back().state = config.lowRes ? MenuButton::State::ON : MenuButton::State::OFF;

		_menus[Status::OPTIONS].buttons.emplace_back(glm::vec2(0.0f, 0.80f), meshSize, displayScale, BACKTOMENU,
		Resources::manager().getTexture("button-back", format, Storage::GPU));
	_menus[Status::OPTIONS].images.emplace_back(glm::vec2(0.0f, -0.55f), 0.5f,
		Resources::manager().getTexture("title-options", format, Storage::GPU));

	_menus[Status::DEAD].backgroundImage = &_bgBlurTexture;
	_menus[Status::DEAD].buttons.emplace_back(glm::vec2(0.0f, 0.20f), meshSize, displayScale, NEWGAME,
		Resources::manager().getTexture("button-newgame"));
	_menus[Status::DEAD].buttons.emplace_back(glm::vec2(0.0f, 0.55f), meshSize, displayScale, BACKTOMENU,
		Resources::manager().getTexture("button-menu"));
	_menus[Status::DEAD].images.emplace_back(glm::vec2(0.0f, -0.47f), 0.5f,
		Resources::manager().getTexture("title-dead", format, Storage::GPU));
	_menus[Status::DEAD].labels.emplace_back(glm::vec2(0.0f, -0.27f), 0.25f, font, Font::Alignment::CENTER);

	_menus[Status::INGAME].labels.emplace_back(glm::vec2(0.0f, -0.90f), 0.2f, font, Font::Alignment::CENTER);

	// Initialize each menu buttons sizes.
	const float initialRatio = float(_config.initialWidth) / float(_config.initialHeight);
	for(auto & menu : _menus) {
		GameMenu & currentMenu = menu.second;
		currentMenu.update(_config.screenResolution, initialRatio);
	}

	resize(uint(_config.screenResolution[0]), uint(_config.screenResolution[1]));
}

void Game::draw(Window& window) {
	// If ingame, render the game.
	if(_status == Status::INGAME) {
		// Before drawing, prepare the model matrices.
		_player->updateModels();
		_inGameRenderer.drawPlayer(*_player, _gameResult);

		GPU::beginRender(window);
		window.setViewport();
		_finalProgram->use();
		_finalProgram->texture(_gameResult, 0);
		GPU::drawQuad();
		GPU::endRender();
		
	}

	// Make sure we are rendering directly in the window.
	GPU::beginRender(window, 1.0f, Load::Operation::DONTCARE, Load::Operation::LOAD);
	const float renderRatio = float(_gameResult.height) / float(_gameResult.width);
	_menuRenderer.drawMenu( _menus[_status], _config.screenResolution, renderRatio);
	GPU::endRender();

}

Window::Action Game::update() {

	// Check if we need to resize.
	if(Input::manager().resized()) {
		resize(uint(Input::manager().size()[0]), uint(Input::manager().size()[1]));
	}

	// Decide which action should (maybe) be performed.
	Window::Action finalAction = Window::Action::None;

	// Handle quitting.
	if(Input::manager().triggered(Input::Key::Escape)) {
		if(_status == Status::MAINMENU) {
			// Special case.
			finalAction = handleButton(ButtonAction::QUIT);
		} else if(_status == Status::INGAME) {
			finalAction = handleButton(ButtonAction::PAUSE);
		} else if(_status == Status::PAUSED) {
			finalAction = handleButton(ButtonAction::RESUME);
		} else if(_status == Status::OPTIONS || _status == Status::DEAD) {
			// If paused, dead or in options menu, go back to main menu.
			finalAction = handleButton(ButtonAction::BACKTOMENU);
		}
	}

	// Handle in-game updates and transition to death menu.
	if(_status == Status::INGAME) {
		_player->update();

		if(!_player->alive()) {
			_status = Status::DEAD;
			// Make sure the blur effect buffer is the right size.
			_bgBlurTexture.resize(_gameResult.width, _gameResult.height);
			_bgBlur->process(_gameResult, _bgBlurTexture);
			_menus[Status::DEAD].labels[0].update(std::to_string(_player->score()));

			// Save the final score.
			const std::string scores = Resources::loadStringFromExternalFile("./scores.sav");
			Resources::saveStringToExternalFile("./scores.sav", std::to_string(_player->score()) + "\n" + scores);
		}
	} else {
		// We are in a menu.
		GameMenu & currentMenu = _menus[_status];
		// Check if any button is hovered or pressed.
		const glm::vec2 mousePos = (Input::manager().mouse() * 2.0f - 1.0f);
		for(MenuButton & button : currentMenu.buttons) {
			button.state = MenuButton::State::OFF;
			// Check if mouse inside.
			if(button.contains(mousePos)) {
				button.state = Input::manager().pressed(Input::Mouse::Left) ? MenuButton::State::ON : MenuButton::State::HOVER;
				// If the mouse was released, trigger the action.
				if(Input::manager().released(Input::Mouse::Left)) {
					// Do the action.
					const Window::Action result = handleButton(ButtonAction(button.tag));
					if(finalAction == Window::Action::None) {
						finalAction = result;
					}
				}
			}
		}
		// Check if any checkbox was checked.
		for(MenuButton & toggle : currentMenu.toggles) {
			// Check if mouse inside, and if the click was validated through release.
			if(toggle.contains(mousePos) && Input::manager().released(Input::Mouse::Left)) {
				// Do the action.
				const Window::Action result = handleButton(ButtonAction(toggle.tag));
				if(finalAction == Window::Action::None) {
					finalAction = result;
				}
				// Update the display state.
				toggle.state = toggle.state == MenuButton::State::ON ? MenuButton::State::OFF : MenuButton::State::ON;
			}
		}
	}

	return finalAction;
}

Window::Action Game::handleButton(ButtonAction tag) {
	switch(tag) {
		case NEWGAME:
			_player = std::unique_ptr<Player>(new Player());
			_menus[Status::INGAME].labels[0].update("0");
			_status = Status::INGAME;
			break;
		case BACKTOMENU:
			// Delete the player if it exist.
			if(_player) {
				_player = std::unique_ptr<Player>(nullptr);
			}
			_status = Status::MAINMENU;
			break;
		case OPTIONS:
			_status = Status::OPTIONS;
			break;
		case PAUSE: {
			_bgBlurTexture.resize(_gameResult.width, _gameResult.height);
			_bgBlur->process(_gameResult, _bgBlurTexture);
			_status = Status::PAUSED;
			break;
		}
		case RESUME:
			_status = Status::INGAME;
			break;
		case QUIT:
			return Window::Action::Quit;
		case OPTION_FULLSCREEN:
			return Window::Action::Fullscreen;
		case OPTION_VSYNC:
			return Window::Action::Vsync;
		case OPTION_HALFRES:
			_config.lowRes = !_config.lowRes;
			resize(uint(_config.screenResolution[0]), uint(_config.screenResolution[1]));
		default:
			break;
	}

	return Window::Action::None;
}

void Game::physics(double frameTime) {

	if(_status == Status::INGAME && !_overrideTime) {
		_playTime			= _playTime + frameTime;
		const bool hasEaten = _player->physics(_playTime, frameTime);
		// Update the ingame score label.
		if(hasEaten) {
			_menus[Status::INGAME].labels[0].update(std::to_string(_player->score()));
		}
	}

	// No physics in menus.
}

void Game::resize(unsigned int width, unsigned int height) {
	const float scaling = _config.lowRes ? 0.75f : 1.0f;
	_config.internalVerticalResolution = int(float(height) / Input::manager().density() * scaling);
	_config.screenResolution = {float(width), float(height)};

	const glm::vec2 renderRes = _config.renderingResolution();
	const uint w = uint(renderRes[0]);
	const uint h = uint(renderRes[1]);
	_gameResult.resize(w,h);
	_inGameRenderer.resize(w, h);
	// Update each menu buttons sizes.
	const float initialRatio = float(_config.initialWidth) / float(_config.initialHeight);
	for(auto & menu : _menus) {
		GameMenu & currentMenu = menu.second;
		currentMenu.update(_config.screenResolution, initialRatio);
	}
}


#include "Game.hpp"
#include "input/Input.hpp"
#include "graphics/GLUtilities.hpp"
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
	_config(config), _inGameRenderer(_config.screenResolution) {

	_bgBlur = std::unique_ptr<GaussianBlur>(new GaussianBlur(_config.initialWidth, _config.initialHeight, 3, Layout::RGB8));
	_finalProgram = Resources::manager().getProgram2D("sharpening");
	_gameFramebuffer = _inGameRenderer.createOutput(uint(_config.screenResolution[0]), uint(_config.screenResolution[1]));

	// Create menus.
	const glm::vec2 meshSize = _menuRenderer.getButtonSize();
	const float displayScale = 0.3f;
	const Font * font		 = Resources::manager().getFont("digits");

	const Descriptor commonDesc		  = {Layout::SRGB8_ALPHA8, Filter::LINEAR_LINEAR, Wrap::CLAMP};
	const Texture * backgroundTexture = Resources::manager().getTexture("menubg", commonDesc, Storage::GPU);

	_menus[Status::MAINMENU].backgroundImage = backgroundTexture;
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, 0.10f), meshSize, displayScale, NEWGAME,
		Resources::manager().getTexture("button-newgame", commonDesc, Storage::GPU));
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, -0.25f), meshSize, displayScale, OPTIONS,
		Resources::manager().getTexture("button-options", commonDesc, Storage::GPU));
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, -0.60f), meshSize, displayScale, QUIT,
		Resources::manager().getTexture("button-quit", commonDesc, Storage::GPU));
	_menus[Status::MAINMENU].images.emplace_back(glm::vec2(0.0f, 0.47f), 0.5f,
		Resources::manager().getTexture("title", commonDesc, Storage::GPU));

	_menus[Status::PAUSED].backgroundImage = _bgBlur->texture();
	_menus[Status::PAUSED].buttons.emplace_back(glm::vec2(0.0f, 0.10f), meshSize, displayScale, RESUME,
		Resources::manager().getTexture("button-resume", commonDesc, Storage::GPU));
	_menus[Status::PAUSED].buttons.emplace_back(glm::vec2(0.0f, -0.25f), meshSize, displayScale, BACKTOMENU,
		Resources::manager().getTexture("button-menu", commonDesc, Storage::GPU));
	_menus[Status::PAUSED].images.emplace_back(glm::vec2(0.0f, 0.47f), 0.5f,
		Resources::manager().getTexture("title-pause", commonDesc, Storage::GPU));

	_menus[Status::OPTIONS].backgroundImage = backgroundTexture;

	_menus[Status::OPTIONS].toggles.emplace_back(glm::vec2(0.0f, 0.20f), meshSize, displayScale, OPTION_FULLSCREEN,
		Resources::manager().getTexture("button-fullscreen", commonDesc, Storage::GPU));
	_menus[Status::OPTIONS].toggles.back().state = config.fullscreen ? MenuButton::State::ON : MenuButton::State::OFF;
	_menus[Status::OPTIONS].toggles.emplace_back(glm::vec2(0.0f, -0.10f), meshSize, displayScale, OPTION_VSYNC,
		Resources::manager().getTexture("button-vsync", commonDesc, Storage::GPU));
	_menus[Status::OPTIONS].toggles.back().state = config.vsync ? MenuButton::State::ON : MenuButton::State::OFF;
	_menus[Status::OPTIONS].toggles.emplace_back(glm::vec2(0.0f, -0.40f), meshSize, displayScale, OPTION_HALFRES,
			Resources::manager().getTexture("button-halfres", commonDesc, Storage::GPU));
		_menus[Status::OPTIONS].toggles.back().state = config.lowRes ? MenuButton::State::ON : MenuButton::State::OFF;

		_menus[Status::OPTIONS].buttons.emplace_back(glm::vec2(0.0f, -0.80f), meshSize, displayScale, BACKTOMENU,
		Resources::manager().getTexture("button-back", commonDesc, Storage::GPU));
	_menus[Status::OPTIONS].images.emplace_back(glm::vec2(0.0f, 0.55f), 0.5f,
		Resources::manager().getTexture("title-options", commonDesc, Storage::GPU));

	_menus[Status::DEAD].backgroundImage = _bgBlur->texture();
	_menus[Status::DEAD].buttons.emplace_back(glm::vec2(0.0f, -0.20f), meshSize, displayScale, NEWGAME,
		Resources::manager().getTexture("button-newgame"));
	_menus[Status::DEAD].buttons.emplace_back(glm::vec2(0.0f, -0.55f), meshSize, displayScale, BACKTOMENU,
		Resources::manager().getTexture("button-menu"));
	_menus[Status::DEAD].images.emplace_back(glm::vec2(0.0f, 0.47f), 0.5f,
		Resources::manager().getTexture("title-dead", commonDesc, Storage::GPU));
	_menus[Status::DEAD].labels.emplace_back(glm::vec2(0.0f, 0.05f), 0.25f, font, Font::Alignment::CENTER);

	_menus[Status::INGAME].labels.emplace_back(glm::vec2(0.0f, 0.70f), 0.2f, font, Font::Alignment::CENTER);

	// Initialize each menu buttons sizes.
	const float initialRatio = float(_config.initialWidth) / float(_config.initialHeight);
	for(auto & menu : _menus) {
		GameMenu & currentMenu = menu.second;
		currentMenu.update(_config.screenResolution, initialRatio);
	}

	resize(uint(_config.screenResolution[0]), uint(_config.screenResolution[1]));
}

void Game::draw() {
	// If ingame, render the game.
	if(_status == Status::INGAME) {
		// Before drawing, prepare the model matrices.
		_player->updateModels();
		_inGameRenderer.drawPlayer(*_player, *_gameFramebuffer);
		
		GLUtilities::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
		Framebuffer::backbuffer()->bind(Framebuffer::Mode::SRGB);
		_finalProgram->use();
		ScreenQuad::draw(_gameFramebuffer->texture());
		Framebuffer::backbuffer()->unbind();
		checkGLError();
		
	}

	const float renderRatio = float(_gameFramebuffer->height()) / float(_gameFramebuffer->width());
	_menuRenderer.drawMenu(_menus[_status], _config.screenResolution, renderRatio);

	checkGLError();
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
			_bgBlur->resize(_gameFramebuffer->width(), _gameFramebuffer->height());
			_bgBlur->process(_gameFramebuffer->texture());
			_menus[Status::DEAD].labels[0].update(std::to_string(_player->score()));

			// Save the final score.
			const std::string scores = Resources::loadStringFromExternalFile("./scores.sav");
			Resources::saveStringToExternalFile("./scores.sav", std::to_string(_player->score()) + "\n" + scores);
		}
	} else {
		// We are in a menu.
		GameMenu & currentMenu = _menus[_status];
		// Check if any button is hovered or pressed.
		const glm::vec2 mousePos = (Input::manager().mouse() * 2.0f - 1.0f) * glm::vec2(1.0f, -1.0f);
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
			_bgBlur->resize(_gameFramebuffer->width(), _gameFramebuffer->height());
			_bgBlur->process(_gameFramebuffer->texture());
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
	_gameFramebuffer->resize(w,h);
	_inGameRenderer.resize(w, h);
	// Update each menu buttons sizes.
	const float initialRatio = float(_config.initialWidth) / float(_config.initialHeight);
	for(auto & menu : _menus) {
		GameMenu & currentMenu = menu.second;
		currentMenu.update(_config.screenResolution, initialRatio);
	}
}

void Game::clean() {
	_inGameRenderer.clean();
	_bgBlur->clean();
	_gameFramebuffer->clean();
}


#include "Game.hpp"
#include "Common.hpp"
#include "input/Input.hpp"

Game::Game(RenderingConfig & config) : _config(config), _inGameRenderer(config), _menuRenderer(config) {
	
	_bgBlur = std::unique_ptr<GaussianBlur>(new GaussianBlur(_config.initialWidth, _config.initialHeight, 3, GL_RGB8));
	
	// Create menus.
	
	const glm::vec2 meshSize = _menuRenderer.getButtonSize();
	const float displayScale = 0.3f;
	const FontInfos * font = Resources::manager().getFont("digits");
	
	const Descriptor commonDesc = {GL_SRGB8_ALPHA8, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE};
	const GLuint backgroundTexture = Resources::manager().getTexture("menubg", commonDesc)->id;
	
	_menus[Status::MAINMENU].backgroundImage = backgroundTexture;
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f,  0.10f), meshSize, displayScale, NEWGAME,
												  *Resources::manager().getTexture("button-newgame", commonDesc));
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, -0.25f), meshSize, displayScale, OPTIONS,
												  *Resources::manager().getTexture("button-options", commonDesc));
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, -0.60f), meshSize, displayScale, QUIT,
												  *Resources::manager().getTexture("button-quit", commonDesc));
	_menus[Status::MAINMENU].images.emplace_back(glm::vec2(0.0f, 0.47f), 0.5f,
												 *Resources::manager().getTexture("title", commonDesc));
	
	_menus[Status::PAUSED].backgroundImage = _bgBlur->textureId();
	_menus[Status::PAUSED].buttons.emplace_back(glm::vec2(0.0f,  0.10f), meshSize, displayScale, RESUME,
												*Resources::manager().getTexture("button-resume", commonDesc));
	_menus[Status::PAUSED].buttons.emplace_back(glm::vec2(0.0f, -0.25f), meshSize, displayScale, BACKTOMENU,
												*Resources::manager().getTexture("button-menu", commonDesc));
	_menus[Status::PAUSED].images.emplace_back(glm::vec2(0.0f, 0.47f), 0.5f,
											   *Resources::manager().getTexture("title-pause", commonDesc));
	
	_menus[Status::OPTIONS].backgroundImage = backgroundTexture;
	_menus[Status::OPTIONS].toggles.emplace_back(glm::vec2(0.0f,  0.10f), meshSize, displayScale, OPTION_FULLSCREEN,
												 *Resources::manager().getTexture("button-fullscreen", commonDesc));
	_menus[Status::OPTIONS].toggles.back().state = config.fullscreen ? MenuButton::ON : MenuButton::OFF;
	_menus[Status::OPTIONS].toggles.emplace_back(glm::vec2(0.0f,  -0.25f), meshSize, displayScale, OPTION_VSYNC,
												 *Resources::manager().getTexture("button-vsync", commonDesc));
	_menus[Status::OPTIONS].toggles.back().state = config.vsync ? MenuButton::ON : MenuButton::OFF;
	_menus[Status::OPTIONS].buttons.emplace_back(glm::vec2(0.0f, -0.60f), meshSize, displayScale, BACKTOMENU,
												 *Resources::manager().getTexture("button-back", commonDesc));
	_menus[Status::OPTIONS].images.emplace_back(glm::vec2(0.0f, 0.47f), 0.5f,
												*Resources::manager().getTexture("title-options", commonDesc));
	
	_menus[Status::DEAD].backgroundImage = _bgBlur->textureId();
	_menus[Status::DEAD].buttons.emplace_back(glm::vec2(0.0f, -0.20f), meshSize, displayScale, NEWGAME,
											  *Resources::manager().getTexture("button-newgame"));
	_menus[Status::DEAD].buttons.emplace_back(glm::vec2(0.0f, -0.55f), meshSize, displayScale, BACKTOMENU,
											  *Resources::manager().getTexture("button-menu"));
	_menus[Status::DEAD].images.emplace_back(glm::vec2(0.0f, 0.47f), 0.5f,
											 *Resources::manager().getTexture("title-dead", commonDesc));
	_menus[Status::DEAD].labels.emplace_back(glm::vec2(0.0f, 0.05f), 0.25f, font, Font::CENTER);
	
	_menus[Status::INGAME].labels.emplace_back(glm::vec2(0.0f, 0.70f), 0.2f, font, Font::CENTER);
	
	// Initialize each menu buttons sizes.
	const float initialRatio = _config.initialWidth / float(_config.initialHeight);
	for(auto & menu : _menus){
		GameMenu & currentMenu = menu.second;
		currentMenu.update(_config.screenResolution, initialRatio);
	}
}

void Game::draw(){
	// If ingame, render the game.
	if(_status == Status::INGAME){
		// Before drawing, prepare the model matrices.
		_player->updateModels();
		_inGameRenderer.draw(*_player);
	}
	
	_menuRenderer.draw(_menus[_status]);
	
	checkGLError();
}

Interface::Action Game::update(){
	
	// Check if we need to resize.
	if(Input::manager().resized()){
		resize((unsigned int)Input::manager().size()[0], (unsigned int)Input::manager().size()[1]);
	}
	
	// Debug: Reload resources.
	/*{
		if(Input::manager().triggered(Input::KeyP)){
			Resources::manager().reload();
		}
		if(ImGui::Begin("Infos")){
			ImGui::Text("%.1f ms, %.1f fps", ImGui::GetIO().DeltaTime*1000.0f, ImGui::GetIO().Framerate);
			ImGui::Separator();
			ImGui::Checkbox("Force pause", &_overrideTime);
		}
		ImGui::End();
	}*/
	
	// Decide which action should (maybe) be performed.
	Interface::Action finalAction = Interface::Action::None;
	
	// Handle quitting.
	if(Input::manager().triggered(Input::KeyEscape)){
		if(_status == Status::MAINMENU){
			// Special case.
			finalAction = handleButton(ButtonAction::QUIT);
		} else if(_status == Status::INGAME){
			finalAction = handleButton(ButtonAction::PAUSE);
		} else if(_status == Status::PAUSED){
			finalAction = handleButton(ButtonAction::RESUME);
		} else if(_status == Status::OPTIONS || _status == Status::DEAD){
			// If paused, dead or in options menu, go back to main menu.
			finalAction = handleButton(ButtonAction::BACKTOMENU);
		}
	}
	
	// Handle in-game updates and transition to death menu.
	if(_status == Status::INGAME){
		_inGameRenderer.update();
		_player->update();
		
		if(!_player->alive()){
			_status = Status::DEAD;
			// Make sure the blur effect buffer is the right size.
			const glm::vec2 gameRes = _inGameRenderer.renderingResolution();
			_bgBlur->resize(unsigned int(gameRes[0]), unsigned int(gameRes[1]));
			_bgBlur->process(_inGameRenderer.finalImage());
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
		for( MenuButton & button : currentMenu.buttons){
			button.state = MenuButton::OFF;
			// Check if mouse inside.
			if(button.contains(mousePos)){
				button.state = Input::manager().pressed(Input::MouseLeft) ? MenuButton::ON : MenuButton::HOVER;
				// If the mouse was released, trigger the action.
				if(Input::manager().released(Input::MouseLeft)){
					// Do the action.
					const Interface::Action result = handleButton(ButtonAction(button.tag));
					if(finalAction == Interface::Action::None){
						finalAction = result;
					}
				}
			}
		}
		// Check if any checkbox was checked.
		for( MenuButton & toggle : currentMenu.toggles){
			// Check if mouse inside, and if the click was validated through release.
			if(toggle.contains(mousePos) && Input::manager().released(Input::MouseLeft)){
				// Do the action.
				const Interface::Action result = handleButton(ButtonAction(toggle.tag));
				if(finalAction == Interface::Action::None){
					finalAction = result;
				}
				// Update the display state.
				toggle.state = (toggle.state == MenuButton::ON ? MenuButton::OFF : MenuButton::ON);
			}
		}
	}

	return finalAction;
	
}

Interface::Action Game::handleButton(const ButtonAction tag){
	switch (tag) {
		case NEWGAME:
			_player = std::unique_ptr<Player>(new Player());
			_menus[Status::INGAME].labels[0].update("0");
			_status = Status::INGAME;
			break;
		case BACKTOMENU:
			// Delete the player if it exist.
			if(_player){
				_player = std::unique_ptr<Player>(nullptr);
			}
			_status = Status::MAINMENU;
			break;
		case OPTIONS:
			_status = Status::OPTIONS;
			break;
		case PAUSE:
		{
			const glm::vec2 gameRes = _inGameRenderer.renderingResolution();
			_bgBlur->resize(unsigned int(gameRes[0]), unsigned int(gameRes[1]));
			_bgBlur->process(_inGameRenderer.finalImage());
			_status = Status::PAUSED;
			break;
		}
		case RESUME:
			_status = Status::INGAME;
			break;
		case QUIT:
			return Interface::Action::Quit;
			break;
		case OPTION_FULLSCREEN:
			return Interface::Action::Fullscreen;
			break;
		case OPTION_VSYNC:
			return Interface::Action::Vsync;
			break;
		default:
			break;
	}
	
	return Interface::Action::None;
}

void Game::physics(double frameTime){
	
	if(_status == Status::INGAME && !_overrideTime){
		_playTime = _playTime + frameTime;
		const bool hasEaten = _player->physics(_playTime, frameTime);
		// Update the ingame score label.
		if(hasEaten){
			_menus[Status::INGAME].labels[0].update(std::to_string(_player->score()));
		}
	}
	
	// No physics in menus.
}

void Game::resize(unsigned int width, unsigned int height){
	_config.internalVerticalResolution = int(height / Input::manager().density());
	_inGameRenderer.resize(width, height);
	_menuRenderer.resize(width, height);
	// Update each menu buttons sizes.
	const float initialRatio = _config.initialWidth / float(_config.initialHeight);
	for(auto & menu : _menus){
		GameMenu & currentMenu = menu.second;
		currentMenu.update(_config.screenResolution, initialRatio);
	}
}

void Game::clean() const {
	_inGameRenderer.clean();
	_menuRenderer.clean();
	_bgBlur->clean();
}

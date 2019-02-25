
#include "Game.hpp"
#include "Common.hpp"
#include "input/Input.hpp"
#include "helpers/InterfaceUtilities.hpp"

Game::Game(RenderingConfig & config) : _config(config), _inGameRenderer(config), _menuRenderer(config) {
	// Create menus.
	
	_menus[Status::MAINMENU].backgroundColor = glm::vec3(1.0f, 1.0f, 0.0f);
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f,  0.10f), glm::vec2(0.28f, 0.14f), NEWGAME);
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, -0.25f), glm::vec2(0.28f, 0.14f), OPTIONS);
	_menus[Status::MAINMENU].buttons.emplace_back(glm::vec2(0.0f, -0.60f), glm::vec2(0.28f, 0.14f), QUIT);
	
	_menus[Status::PAUSED].backgroundColor = glm::vec3(1.0f, 0.0f, 1.0f);
	_menus[Status::PAUSED].buttons.emplace_back(glm::vec2(0.0f,  0.10f), glm::vec2(0.28f, 0.14f), RESUME);
	_menus[Status::PAUSED].buttons.emplace_back(glm::vec2(0.0f, -0.25f), glm::vec2(0.28f, 0.14f), BACKTOMENU);
	
	_menus[Status::OPTIONS].backgroundColor = glm::vec3(0.0f, 1.0f, 1.0f);
	_menus[Status::OPTIONS].buttons.emplace_back(glm::vec2(0.0f,  0.10f), glm::vec2(0.28f, 0.14f), OPTION_FULLSCREEN);
	_menus[Status::OPTIONS].buttons.emplace_back(glm::vec2(0.0f, -0.25f), glm::vec2(0.28f, 0.14f), BACKTOMENU);
	
	_menus[Status::DEAD].backgroundColor = glm::vec3(1.0f, 1.0f, 0.0f);
	_menus[Status::DEAD].buttons.emplace_back(glm::vec2(0.0f,  0.10f), glm::vec2(0.28f, 0.14f), NEWGAME);
	_menus[Status::DEAD].buttons.emplace_back(glm::vec2(0.0f, -0.25f), glm::vec2(0.28f, 0.14f), BACKTOMENU);
	
	// Initialize each menu buttons sizes.
	const float initialRatio = _config.initialWidth / float(_config.initialHeight);
	for(auto & menu : _menus){
		GameMenu & currentMenu = menu.second;
		currentMenu.update(_config.screenResolution, initialRatio);
	}
}

void Game::draw(){
	// before drawing, prepare the model matrices.
	if(_status == Status::INGAME){
		_player->updateModels();
		_inGameRenderer.draw(*_player);
	} else {
		_menuRenderer.draw(_menus[_status]);
	}
	
	// \todo Draw everything to a final framebuffer and then handle writing to the window from here.
	// Especially if we need to preserve aspect ratio in fullscreen mode.
	checkGLError();
}

Interface::Action Game::update(){
	
	if(Input::manager().resized()){
		resize((unsigned int)Input::manager().size()[0], (unsigned int)Input::manager().size()[1]);
	}
	
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
	
	// Debug: Reload resources.
	{
		if(Input::manager().triggered(Input::KeyP)){
			Resources::manager().reload();
		}
		if(ImGui::Begin("Infos")){
			ImGui::Text("%.1f ms, %.1f fps", ImGui::GetIO().DeltaTime*1000.0f, ImGui::GetIO().Framerate);
		}
		ImGui::End();
	}
	
	if(_status == Status::INGAME){
		_inGameRenderer.update();
		_player->update();
		if(!_player->alive()){
			_status = Status::DEAD;
			Log::Info() << "Final score: " << _player->score() << "!" << std::endl;
		}
	} else {
		// We are in a menu.
		GameMenu & currentMenu = _menus[_status];
		// Check if any button is hovered or pressed.
		const glm::vec2 mousePos = (Input::manager().mouse() * 2.0f - 1.0f) * glm::vec2(1.0f, -1.0f);
		for( MenuButton & button : currentMenu.buttons){
			button.state = MenuButton::OFF;
			// Check if mouse inside.
			if(glm::all(glm::greaterThanEqual(mousePos, button.pos - button.scale))
			   && glm::all(glm::lessThanEqual(mousePos, button.pos + button.scale))){
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
	}

	return finalAction;
	
}

Interface::Action Game::handleButton(const ButtonAction tag){
	switch (tag) {
		case NEWGAME:
			_player = std::unique_ptr<Player>(new Player());
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
			_status = Status::PAUSED;
			break;
		case RESUME:
			_status = Status::INGAME;
			break;
		case QUIT:
			return Interface::Action::Quit;
			break;
		case OPTION_FULLSCREEN:
			return Interface::Action::Fullscreen;
			break;
		default:
			break;
	}
	
	return Interface::Action::None;
}

void Game::physics(double fullTime, double frameTime){
	
	if(_status == Status::INGAME){
		_playTime = _playTime + frameTime;
		_player->physics(_playTime, frameTime);
	}
	
	// No physics in menus.
}

void Game::resize(unsigned int width, unsigned int height){
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
}

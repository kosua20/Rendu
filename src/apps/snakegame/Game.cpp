
#include "Game.hpp"
#include "Common.hpp"
#include "input/Input.hpp"
#include "helpers/InterfaceUtilities.hpp"

Game::Game(RenderingConfig & config) : _config(config), _inGameRenderer(config), _menuRenderer(config) {
	// Create menus.
	_menus[Status::START] = GameMenu();
	_menus[Status::START].backgroundColor = glm::vec3(0.0f, 1.0f, 0.0f);
	
	_menus[Status::PAUSE] = GameMenu();
	_menus[Status::PAUSE].backgroundColor = glm::vec3(0.0f, 0.0f, 1.0f);
	
	_menus[Status::OPTIONS] = GameMenu();
	_menus[Status::OPTIONS].backgroundColor = glm::vec3(0.0f, 1.0f, 1.0f);
	
	_menus[Status::DEAD] = GameMenu();
	_menus[Status::DEAD].backgroundColor = glm::vec3(1.0f, 0.0f, 0.0f);
}

void Game::draw(){
	// before drawing, prepare the model matrices.
	if(_status == Status::INGAME){
		_player.updateModels();
		_inGameRenderer.draw(_player);
	} else {
		_menuRenderer.draw(_menus[_status]);
	}
	
	checkGLError();
}

bool Game::update(){
	
	if(Input::manager().resized()){
		resize((unsigned int)Input::manager().size()[0], (unsigned int)Input::manager().size()[1]);
	}
	
	// Handle quitting.
	if(Input::manager().pressed(Input::KeyEscape)){
		return true;
	}
	// Reload resources.
	if(Input::manager().triggered(Input::KeyP)){
		Resources::manager().reload();
	}
	
	
	if(Input::manager().triggered(Input::KeySpace)){
		if(_status == Status::INGAME){
			_status = Status::PAUSE;
		} else {
			_status = Status::INGAME;
		}
	}
	
	if(ImGui::Begin("Infos")){
		ImGui::Text("%.1f ms, %.1f fps", ImGui::GetIO().DeltaTime*1000.0f, ImGui::GetIO().Framerate);
	}
	ImGui::End();
	
	if(_status == Status::INGAME){
		_inGameRenderer.update();
		_player.update();
		if(!_player.alive()){
			_status = Status::DEAD;
		}
	} else {
		const GameMenu & currentMenu = _menus[_status];
		// Do something with it.
	}

	return false;
	
}

void Game::physics(double fullTime, double frameTime){
	
	if(_status == Status::INGAME){
		// \todo Improve to avoid accumulating errors.
		_playTime = _playTime + frameTime;
		_player.physics(_playTime, frameTime);
	}
	
	// No physics in menus.
}

void Game::resize(unsigned int width, unsigned int height){
	_inGameRenderer.resize(width, height);
	_menuRenderer.resize(width, height);
}

void Game::clean() const {
	_inGameRenderer.clean();
	_menuRenderer.clean();
}

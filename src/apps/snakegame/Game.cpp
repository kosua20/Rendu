
#include "Game.hpp"
#include "Common.hpp"
#include "input/Input.hpp"
#include "helpers/InterfaceUtilities.hpp"

Game::Game(RenderingConfig & config) : _inGameRenderer(config), _config(config) {
	
}

void Game::draw(){
	// before drawing, prepare the model matrices.
	if(_status == Status::INGAME){
		_player.updateModels();
		_inGameRenderer.draw(_player);
	} else if(_status == Status::DEAD){
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0.0f, 0.0f, _config.screenResolution[0], _config.screenResolution[1]);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
	} else if(_status == Status::START){
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0.0f, 0.0f, _config.screenResolution[0], _config.screenResolution[1]);
		glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0.0f, 0.0f, _config.screenResolution[0], _config.screenResolution[1]);
		glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	
	
	checkGLError();
}

bool Game::update(){
	
	if(Input::manager().resized()){
		_inGameRenderer.resize((unsigned int)Input::manager().size()[0], (unsigned int)Input::manager().size()[1]);
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
	}

	
	
	return false;
	
}

void Game::physics(double fullTime, double frameTime){
	
	if(_status == Status::INGAME){
		// \todo Improve to avoid accumulating errors.
		_playTime = _playTime + frameTime;
		_player.physics(_playTime, frameTime);
	}
}

void Game::resize(unsigned int width, unsigned int height){
	_inGameRenderer.resize(width, height);
}

void Game::clean() const {
	_inGameRenderer.clean();
}

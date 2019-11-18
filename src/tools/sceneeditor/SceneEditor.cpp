#include "SceneEditor.hpp"
#include "graphics/GLUtilities.hpp"

SceneEditor::SceneEditor(RenderingConfig & config) : CameraApp(config)
{
	_cameraFOV = _userCamera.fov() * 180.0f / glm::pi<float>();
	// Query existing scenes.
	std::map<std::string, std::string> sceneInfos;
	Resources::manager().getFiles("scene", sceneInfos);
	_sceneNames.emplace_back("New scene");
	for(const auto & info : sceneInfos) {
	 _sceneNames.push_back(info.first);
	}
	_scenes.push_back(nullptr);
	for(size_t i = 1; i < _sceneNames.size(); ++i) {
		const auto & sceneName = _sceneNames[i];
		_scenes.emplace_back(new Scene(sceneName));
	}
	
	// Set the empty scene by default.
	setScene(_scenes[0]);
}

void SceneEditor::setScene(const std::shared_ptr<Scene> & scene) {
	if(!scene) {
		return;
	}
	
	scene->init(Storage::GPU);
	
	// Camera setup.
	_userCamera.apply(scene->viewpoint());
	_userCamera.ratio(_config.screenResolution[0] / _config.screenResolution[1]);
	const BoundingBox & bbox = scene->boundingBox();
	const float range		 = glm::length(bbox.getSize());
	_userCamera.frustum(0.01f * range, 5.0f * range);
	_userCamera.speed() = 0.2f * range;
	_cameraFOV			= _userCamera.fov() * 180.0f / glm::pi<float>();
	

}

void SceneEditor::draw() {

	// If no scene, just clear.
	if(!_scenes[_currentScene]) {
		GLUtilities::clearColorAndDepth({0.2f, 0.2f, 0.2f, 1.0f}, 1.0f);
		return;
	}
}
void SceneEditor::update() {
	CameraApp::update();
	
	// Handle scene switching.
	if(ImGui::Begin("Scene")) {
		ImGui::Text("%.1f ms, %.1f fps", ImGui::GetIO().DeltaTime * 1000.0f, ImGui::GetIO().Framerate);
		
		const std::string & currentName = _sceneNames[_currentScene];
		if(ImGui::BeginCombo("Scene", currentName.c_str(), ImGuiComboFlags_None)) {
			for(size_t i = 0; i < _sceneNames.size(); ++i) {
				if(ImGui::Selectable(_sceneNames[i].c_str(), i == _currentScene)) {
					_currentScene = i;
					setScene(_scenes[_currentScene]);
				}
				if(_currentScene == i) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Separator();
	}
	ImGui::End();
	
	// If no scene, no need to udpate the camera or the scene-specific UI.
	if(!_scenes[_currentScene]) {
		return;
	}
	if(ImGui::Begin("Camera")) {
		// Camera settings.
		ImGui::PushItemWidth(100);
		ImGui::Combo("Camera mode", reinterpret_cast<int *>(&_userCamera.mode()), "FPS\0Turntable\0Joystick\0\0", 3);
		ImGui::InputFloat("Camera speed", &_userCamera.speed(), 0.1f, 1.0f);
		if(ImGui::InputFloat("Camera FOV", &_cameraFOV, 1.0f, 10.0f)) {
			_userCamera.fov(_cameraFOV * glm::pi<float>() / 180.0f);
		}
		ImGui::PopItemWidth();

		// Copy/paste camera to clipboard.
		if(ImGui::Button("Copy camera")) {
			const std::string camDesc = _userCamera.encode();
			ImGui::SetClipboardText(camDesc.c_str());
		}
		ImGui::SameLine();
		if(ImGui::Button("Paste camera")) {
			const std::string camDesc(ImGui::GetClipboardText());
			const auto cameraCode = Codable::parse(camDesc);
			if(!cameraCode.empty()) {
				_userCamera.decode(cameraCode[0]);
				_cameraFOV = _userCamera.fov() * 180.0f / glm::pi<float>();
			}
		}
		// Reset to the scene reference viewpoint.
		if(ImGui::Button("Reset")) {
			_userCamera.apply(_scenes[_currentScene]->viewpoint());
			_userCamera.ratio(_config.screenResolution[0] / _config.screenResolution[1]);
			_cameraFOV = _userCamera.fov() * 180.0f / glm::pi<float>();
		}
	}
	ImGui::End();
}

void SceneEditor::physics(double fullTime, double frameTime) {
	if(_scenes[_currentScene] && !_paused) {
		_scenes[_currentScene]->update(fullTime, frameTime);
	}
}

void SceneEditor::clean() {
}

void SceneEditor::resize() {
}

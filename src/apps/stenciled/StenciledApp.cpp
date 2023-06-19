#include "StenciledApp.hpp"
#include "graphics/GPU.hpp"
#include "input/Input.hpp"

StenciledApp::StenciledApp(RenderingConfig & config) :
	CameraApp(config) {

	const glm::vec2 renderRes = _config.renderingResolution();
	_renderer.reset(new StenciledRenderer(renderRes));
	_finalRender.reset(new Framebuffer(uint(renderRes[0]), uint(renderRes[1]), Layout::RGBA8, "Final render"));

	// Load all existing scenes, with associated names.
	std::vector<Resources::FileInfos> sceneInfos;
	Resources::manager().getFiles("scene", sceneInfos);
	_sceneNames.emplace_back("None");
	for(const auto & info : sceneInfos) {
		_sceneNames.push_back(info.name);
	}
	_scenes.push_back(nullptr);
	for(size_t i = 1; i < _sceneNames.size(); ++i) {
		const auto & sceneName = _sceneNames[i];
		_scenes.emplace_back(new Scene(sceneName));
	}
	// Load the first scene by default.
	setScene(_scenes[_currentScene]);
}

void StenciledApp::setScene(const std::shared_ptr<Scene> & scene) {
	if(!scene) {
		freezeCamera(true);
		return;
	}
	freezeCamera(false);

	if(!scene->init(Storage::GPU)){
		_currentScene = 0;
		setScene(_scenes[_currentScene]);
		return;
	}

	_userCamera.apply(scene->viewpoint());
	_userCamera.ratio(_config.screenResolution[0] / _config.screenResolution[1]);
	const BoundingBox & bbox = scene->boundingBox();
	const float range		 = glm::length(bbox.getSize());
	_userCamera.frustum(0.01f * range, 5.0f * range);
	_userCamera.speed() = 0.2f * range;

	// Set the scene for the renderer.
	_renderer->setScene(scene);

}

void StenciledApp::draw() {
	if(!_scenes[_currentScene]) {
		Swapchain::backbuffer()->bind(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, Load::Operation::DONTCARE);
		return;
	}
	_renderer->draw(_userCamera, *_finalRender);
	
	GPU::blit(*_finalRender, *Swapchain::backbuffer(), Filter::LINEAR);

}

void StenciledApp::update() {
	CameraApp::update();

	// First part of the ImGui window is always displayed.
	if(ImGui::Begin("Renderer")) {
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
		if(ImGui::CollapsingHeader("Camera")){
			_userCamera.interface();
		}
	}
	ImGui::End();

	// If no scene, no need to udpate the camera or the scene-specific UI.
	if(!_scenes[_currentScene]) {
		return;
	}

}

void StenciledApp::physics(double fullTime, double frameTime) {
	if(_scenes[_currentScene]) {
		_scenes[_currentScene]->update(fullTime, frameTime);
	}
}

void StenciledApp::resize() {
	// Same aspect ratio as the display resolution
	const glm::vec2 renderRes = _config.renderingResolution();
	const uint rw = uint(renderRes[0]);
	const uint rh = uint(renderRes[1]);
	_renderer->resize(rw, rh);
	_finalRender->resize(rw, rh);
}

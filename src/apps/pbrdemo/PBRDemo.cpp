#include "PBRDemo.hpp"
#include "graphics/GLUtilities.hpp"
#include "input/Input.hpp"

PBRDemo::PBRDemo(RenderingConfig & config) : CameraApp(config) {
	
	const glm::vec2 renderRes = _config.renderingResolution();
	_renderer.reset(new DeferredRenderer(renderRes));
	_postprocess.reset(new PostProcessStack(renderRes));
	_finalProgram = Resources::manager().getProgram2D("final_screenquad");
	
	// Setup camera parameters.
	_cameraFOV = _userCamera.fov() * 180.0f / glm::pi<float>();
	_cplanes   = _userCamera.clippingPlanes();
	
	// Load all existing scenes, with associated names.
	std::map<std::string, std::string> sceneInfos;
	Resources::manager().getFiles("scene", sceneInfos);
	_sceneNames.emplace_back("None");
	for(const auto & info : sceneInfos) {
		_sceneNames.push_back(info.first);
	}
	_scenes.push_back(nullptr);
	for(size_t i = 1; i < _sceneNames.size(); ++i) {
		const auto & sceneName = _sceneNames[i];
		_scenes.emplace_back(new Scene(sceneName));
	}
	// Load the first scene by default.
	setScene(_scenes[_currentScene]);
}

void PBRDemo::setScene(const std::shared_ptr<Scene> & scene) {
	if(!scene){
		freezeCamera(true);
		return;
	}
	freezeCamera(false);
	
	scene->init(Storage::GPU);

	_userCamera.apply(scene->viewpoint());
	_userCamera.ratio(_config.screenResolution[0] / _config.screenResolution[1]);
	const BoundingBox & bbox = scene->boundingBox();
	const float range		 = glm::length(bbox.getSize());
	_userCamera.frustum(0.01f * range, 5.0f * range);
	_userCamera.speed() = 0.2f * range;
	_cplanes			= _userCamera.clippingPlanes();
	_cameraFOV			= _userCamera.fov() * 180.0f / glm::pi<float>();
	_renderer->setScene(scene);
}

void PBRDemo::draw() {

	if(!_scenes[_currentScene]) {
		GLUtilities::clearColorAndDepth({0.2f, 0.2f, 0.2f, 1.0f}, 1.0f);
		return;
	}
	_renderer->draw(_userCamera);
	_postprocess->process(_renderer->result());
	
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	Framebuffer::backbuffer()->bind(Framebuffer::Mode::SRGB);
	GLUtilities::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
	_finalProgram->use();
	ScreenQuad::draw(_postprocess->result());
	Framebuffer::backbuffer()->unbind();
	
}

void PBRDemo::update() {
	CameraApp::update();
	
	// First part of the ImGui window is always displayed.
	if(ImGui::Begin("Renderer")) {
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
	}
	ImGui::End();
	
	// If no scene, no need to udpate the camera or the scene-specific UI.
	if(!_scenes[_currentScene]) {
		return;
	}
	
	// Reload the scene metadata.
	if(Input::manager().triggered(Input::Key::P)) {
		_scenes[_currentScene].reset(new Scene(_sceneNames[_currentScene]));
		setScene(_scenes[_currentScene]);
	}
	
	// Reopen the Imgui window.
	if(ImGui::Begin("Renderer")) {
		ImGui::Separator();
		
		ImGui::PushItemWidth(110);
		ImGui::Combo("Camera mode", reinterpret_cast<int *>(&_userCamera.mode()), "FPS\0Turntable\0Joystick\0\0", 3);
		ImGui::InputFloat("Camera speed", &_userCamera.speed(), 0.1f, 1.0f);
		if(ImGui::InputFloat("Camera FOV", &_cameraFOV, 1.0f, 10.0f)) {
			_userCamera.fov(_cameraFOV * glm::pi<float>() / 180.0f);
		}
		ImGui::PopItemWidth();

		if(ImGui::DragFloat2("Planes", static_cast<float *>(&_cplanes[0]))) {
			_userCamera.frustum(_cplanes[0], _cplanes[1]);
		}

		if(ImGui::Button("Copy camera", ImVec2(104, 0))) {
			const std::string camDesc = Codable::encode({_userCamera.encode()});
			ImGui::SetClipboardText(camDesc.c_str());
		}
		ImGui::SameLine();
		if(ImGui::Button("Paste camera", ImVec2(104, 0))) {
			const std::string camDesc(ImGui::GetClipboardText());
			const auto cameraCode = Codable::decode(camDesc);
			if(!cameraCode.empty()) {
				_userCamera.decode(cameraCode[0]);
				_cameraFOV = _userCamera.fov() * 180.0f / glm::pi<float>();
				_cplanes   = _userCamera.clippingPlanes();
			}
		}

		ImGui::Separator();
		ImGui::PushItemWidth(110);
		if(ImGui::InputInt("Vertical res.", &_config.internalVerticalResolution, 50, 200)) {
			_config.internalVerticalResolution = std::max(8, _config.internalVerticalResolution);
			resize();
		}

		ImGui::Checkbox("Bloom  ", &_postprocess->settings().bloom);
		if(_postprocess->settings().bloom) {
			ImGui::SameLine(120);
			ImGui::SliderFloat("Th.##Bloom", &_postprocess->settings().bloomTh, 0.5f, 2.0f);

			ImGui::PushItemWidth(80);
			if(ImGui::InputInt("Rad.##Bloom", &_postprocess->settings().bloomRadius, 1, 10)) {
				_postprocess->updateBlurPass();
			}
			ImGui::PopItemWidth();
			ImGui::SameLine(120);
			ImGui::SliderFloat("Mix##Bloom", &_postprocess->settings().bloomMix, 0.0f, 1.5f);
		}
		
		ImGui::Checkbox("SSAO", &_renderer->applySSAO());
		if(_renderer->applySSAO()) {
			ImGui::SameLine(120);
			ImGui::InputFloat("Radius", &_renderer->radiusSSAO(), 0.5f);
		}

		ImGui::Checkbox("Tonemap ", &_postprocess->settings().tonemap);
		if(_postprocess->settings().tonemap) {
			ImGui::SameLine(120);
			ImGui::SliderFloat("Exposure", &_postprocess->settings().exposure, 0.1f, 10.0f);
		}
		ImGui::Checkbox("FXAA", &_postprocess->settings().fxaa);

		ImGui::Separator();
		ImGui::Checkbox("Debug", &_renderer->showLights());
		ImGui::SameLine();
		ImGui::Checkbox("Pause", &_paused);
		ImGui::SameLine();
		ImGui::Checkbox("Update shadows", &_renderer->updateShadows());
		ImGui::PopItemWidth();
		ImGui::ColorEdit3("Background", &(_scenes[_currentScene]->backgroundColor[0]), ImGuiColorEditFlags_Float);
	}
	ImGui::End();
}

void PBRDemo::physics(double fullTime, double frameTime) {
	if(_scenes[_currentScene] && !_paused) {
		_scenes[_currentScene]->update(fullTime, frameTime);
	}
}

void PBRDemo::clean() {
	// Clean objects.
	_renderer->clean();
	_postprocess->clean();
}

void PBRDemo::resize() {
	// Same aspect ratio as the display resolution
	const glm::vec2 renderRes = _config.renderingResolution();
	_renderer->resize(uint(renderRes[0]), uint(renderRes[1]));
	_postprocess->resize(uint(renderRes[0]), uint(renderRes[1]));
}

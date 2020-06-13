#include "PBRDemo.hpp"
#include "renderers/shadowmaps/VarianceShadowMapArray.hpp"
#include "graphics/GLUtilities.hpp"
#include "input/Input.hpp"

PBRDemo::PBRDemo(RenderingConfig & config) :
	CameraApp(config) {

	const glm::vec2 renderRes = _config.renderingResolution();
	_defRenderer.reset(new DeferredRenderer(renderRes, ShadowMode::VARIANCE, true));
	_forRenderer.reset(new ForwardRenderer(renderRes, ShadowMode::VARIANCE, true));
	_postprocess.reset(new PostProcessStack(renderRes));
	_debugRenderer.reset(new DebugRenderer());
	_finalRender.reset(new Framebuffer(uint(renderRes[0]), uint(renderRes[1]), {Layout::RGB16F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, true));

	_finalProgram = Resources::manager().getProgram2D("sharpening");
	
	_probesRenderer.reset(new DeferredRenderer(glm::vec2(256,256), ShadowMode::BASIC, false));

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
	if(!scene) {
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

	// Set the scene for the renderer.
	_defRenderer->setScene(scene);
	_forRenderer->setScene(scene);
	_probesRenderer->setScene(scene);
	_debugRenderer->setScene(scene);

	// Recreate the shadow maps.
	// Delete existing shadow maps.
	for(auto & map : _shadowMaps) {
		map.reset();
	}
	_shadowMaps.clear();
	// Allocate shadow maps.
	// For now all techniques require at most a VSM (depth,depth^2) map so we use this in all cases.
	std::vector<std::shared_ptr<Light>> lights2D;
	std::vector<std::shared_ptr<PointLight>> lightsCube;
	for(auto & light : scene->lights) {
		if(!light->castsShadow()) {
			continue;
		}
		if(auto pLight = std::dynamic_pointer_cast<PointLight>(light)) {
			lightsCube.push_back(pLight);
		} else {
			lights2D.push_back(light);
		}
	}
	if(!lights2D.empty()){
		_shadowMaps.emplace_back(new VarianceShadowMap2DArray(lights2D, glm::vec2(512)));
	}
	if(!lightsCube.empty()){
		_shadowMaps.emplace_back(new VarianceShadowMapCubeArray(lightsCube, 512));
	}

	// Recreate probes
	// Delete existing probes.
	for(auto & probe : _probes) {
		probe.reset();
	}
	_probes.clear();
	// Allocate probes.
	if(scene->environment.type() == LightProbe::Type::DYNAMIC){
		_probes.emplace_back(new Probe(scene->environment.position(), _probesRenderer, 256, 6, glm::vec2(0.01f, 1000.0f)));
		scene->environment.registerEnvironment(_probes[0]->texture(), _probes[0]->shCoeffs());
	}
	// Trigger one-shot data update.
	// Shadow pass.
	for(const auto & map : _shadowMaps) {
		map->draw(*_scenes[_currentScene]);
	}
	// Probes pass.
	for(int i = 0; i < 3; ++i) {
		for(auto & probe : _probes) {
			probe->draw();
			probe->convolveRadiance(1.2f, 1, 5);
			probe->prepareIrradiance();
			probe->estimateIrradiance(5.0f);
			GLUtilities::sync();
		}
	}
}

void PBRDemo::updateMaps(){
	// Light shadows pass.
	_shadowTime.begin();
	for(const auto & map : _shadowMaps) {
		map->draw(*_scenes[_currentScene]);
	}
	_shadowTime.end();

	// Probes pass.

	for(auto & probe : _probes) {
		if(_frameID % _frameCount == 0){
			_probesTime.begin();
			probe->draw();
			_probesTime.end();

		} else if(_frameID % _frameCount == 1){

			_copyTimeCPU.begin();
			_copyTime.begin();
			probe->estimateIrradiance(5.0f);
			_copyTime.end();
			_copyTimeCPU.end();
			
			_inteTime.begin();
			probe->convolveRadiance(1.2f, 1, 2);
			_inteTime.end();

			probe->prepareIrradiance();
		} else {
			_inteTime.begin();
			probe->convolveRadiance(1.2f, 3, 3);
			_inteTime.end();
		}
	}

}

void PBRDemo::draw() {

	_frameID = (_frameID + 1)%_frameCount;

	if(!_scenes[_currentScene]) {
		GLUtilities::clearColorAndDepth({0.2f, 0.2f, 0.2f, 1.0f}, 1.0f);
		return;
	}

	if(_scenes[_currentScene]->animated() && !_paused){
		updateMaps();
	}

	// Renderer and postproc passes.
	_rendererTime.begin();
	if(_mode == RendererMode::DEFERRED) {
		_defRenderer->draw(_userCamera, *_finalRender);
	} else if(_mode == RendererMode::FORWARD) {
		_forRenderer->draw(_userCamera, *_finalRender);
	}
	_rendererTime.end();

	const Framebuffer & depthSrc = _mode == RendererMode::FORWARD ? _forRenderer->depthFramebuffer() : _defRenderer->depthFramebuffer();

	_postprocessTime.begin();
	_postprocess->process(_finalRender->texture(), _userCamera.projection(), depthSrc.depthBuffer(), *_finalRender);
	_postprocessTime.end();

	if(_showDebug){
		GLUtilities::blitDepth(depthSrc, *_finalRender);
		_debugRenderer->draw(_userCamera, *_finalRender);
	}

	// We now render a full screen quad in the default framebuffer, using sRGB space.
	Framebuffer::backbuffer()->bind(Framebuffer::Mode::SRGB);
	GLUtilities::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
	_finalProgram->use();
	GLUtilities::setDepthState(false);
	GLUtilities::setCullState(true);
	ScreenQuad::draw(_finalRender->texture());
	Framebuffer::backbuffer()->unbind();
}

void PBRDemo::update() {
	CameraApp::update();

	// Performances window.
	if(ImGui::Begin("Performance")){
		ImGui::Text("%.1f ms, %.1f fps", frameTime() * 1000.0f, frameRate());
		ImGui::Text("Shadow maps update: %05.1fms", float(_shadowTime.value())/1000000.0f);
		ImGui::Text("Probes update: %05.1fms", float(_probesTime.value())/1000000.0f);
		ImGui::Text("Probes integration: %05.1fms", float(_inteTime.value())/1000000.0f);
		ImGui::Text("Probes copy: %05.1fms", float(_copyTime.value())/1000000.0f);
		ImGui::Text("Probes copy CPU: %05.1fms", float(_copyTimeCPU.value()) / 1000000.0f);
		ImGui::Text("Scene rendering: %05.1fms", float(_rendererTime.value())/1000000.0f);
		ImGui::Text("Post processing: %05.1fms", float(_postprocessTime.value())/1000000.0f);
	}
	ImGui::End();

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
	}
	ImGui::End();

	// If no scene, no need to udpate the camera or the scene-specific UI.
	if(!_scenes[_currentScene]) {
		return;
	}

	// Reload the scene metadata.
	if(Input::manager().triggered(Input::Key::LeftBracket)) {
		_scenes[_currentScene].reset(new Scene(_sceneNames[_currentScene]));
		setScene(_scenes[_currentScene]);
	}

	// Reopen the Imgui window.
	if(ImGui::Begin("Renderer")) {
		ImGui::PushItemWidth(110);
		ImGui::Combo("Renderer##picklist", reinterpret_cast<int *>(&_mode), "Deferred\0Forward\0\0");
		if(ImGui::InputInt("Vertical res.", &_config.internalVerticalResolution, 50, 200)) {
			_config.internalVerticalResolution = std::max(8, _config.internalVerticalResolution);
			resize();
		}

		ImGui::Checkbox("Show debug objects", &_showDebug);
		if(_showDebug){
			if(ImGui::CollapsingHeader("Debug##options")){
				_debugRenderer->interface();
			}
		}

		if(ImGui::CollapsingHeader("Renderer##options")){
			if(_mode == RendererMode::DEFERRED){
				_defRenderer->interface();
			} else {
				_forRenderer->interface();
			}
		}

		if(ImGui::CollapsingHeader("Postprocess")){
			_postprocess->interface();
		}

		if(ImGui::CollapsingHeader("Camera")){
			_userCamera.interface();
		}

		ImGui::Checkbox("Pause animation", &_paused);
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

void PBRDemo::resize() {
	// Same aspect ratio as the display resolution
	const glm::vec2 renderRes = _config.renderingResolution();
	const uint rw = uint(renderRes[0]);
	const uint rh = uint(renderRes[1]);
	_defRenderer->resize(rw, rh);
	_forRenderer->resize(rw, rh);
	_postprocess->resize(rw, rh);
	_finalRender->resize(renderRes);
}

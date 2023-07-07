#include "PBRDemo.hpp"
#include "renderers/shadowmaps/VarianceShadowMap.hpp"
#include "renderers/shadowmaps/BasicShadowMap.hpp"
#include "graphics/GPU.hpp"
#include "input/Input.hpp"
#include "system/Window.hpp"

PBRDemo::PBRDemo(RenderingConfig & config, Window & window) :
	CameraApp(config, window), _finalRender("Final render") {

	const glm::vec2 renderRes = _config.renderingResolution();
	_defRenderer.reset(new DeferredRenderer(renderRes, true, "Deferred"));
	_forRenderer.reset(new ForwardRenderer(renderRes, true, "Forward"));
	_postprocess.reset(new PostProcessStack(renderRes));
	_debugRenderer.reset(new DebugRenderer());
	_finalRender.setupAsDrawable(Layout::RGBA16F, int(renderRes[0]), uint(renderRes[1]));

	_finalProgram = Resources::manager().getProgram2D("sharpening");
	
	_probesRenderer.reset(new DeferredRenderer(glm::vec2(128,128), false, "Probes"));

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

void PBRDemo::setScene(const std::shared_ptr<Scene> & scene) {
	if(!scene) {
		freezeCamera(true);
		return;
	}
	// Reset frame counter.
	_frameID = 0;

	freezeCamera(false);

	if(!scene->init(Storage::GPU)){
		// If unable to load, fallback to the default scene.
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
	_defRenderer->setScene(scene);
	_forRenderer->setScene(scene);
	_probesRenderer->setScene(scene);
	_debugRenderer->setScene(scene);

	// Recreate the shadow maps.
	createShadowMaps(_shadowMode);

	// Recreate probes
	// Delete existing probes.
	for(auto & probe : _probes) {
		probe.reset();
	}
	_probes.clear();
	// Allocate probes.
	for(LightProbe& probe : scene->probes){
		if(probe.type() != LightProbe::Type::DYNAMIC){
			continue;
		}
		_probes.emplace_back(new Probe(probe, _probesRenderer, 128, 6, glm::vec2(0.01f, 1000.0f)));
	}

	// Trigger one-shot data update.
	// Update each probe fully 3 times to capture multi-bounce effects.
	for(uint i = 0; i < 3; ++i){
		for(auto & probe : _probes) {
			probe->update(probe->totalBudget());
			GPU::flush();
		}
	}
}

void PBRDemo::updateMaps(){
	// Light shadows pass.
	{
		GPUMarker marker("Shadow maps");
		_shadowTime.begin();
		for(const auto & map : _shadowMaps) {
			map->draw(*_scenes[_currentScene]);
		}
		_shadowTime.end();
	}


	// Probes pass.
	{
		GPUMarker marker("Probes");
		_probesTime.begin();
		for(auto & probe : _probes) {
			// For now, ensure each dynamic probe is entirely updated over frameCount frames.
			const uint budget = probe->totalBudget() / _frameCount;
			probe->update(budget);
		}
		_probesTime.end();
	}

}

void PBRDemo::draw() {

	++_frameID;

	if(!_scenes[_currentScene]) {
		GPU::beginRender(window(), 1.0f, Load::Operation::DONTCARE, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		GPU::endRender();
		return;
	}

	_totalTime.begin();
	
	if((_scenes[_currentScene]->animated() && !_paused)){
		updateMaps();
	}

	// Renderer and postproc passes.
	_rendererTime.begin();
	if(_mode == RendererMode::DEFERRED) {
		_defRenderer->draw(_userCamera, &_finalRender, nullptr);
	} else if(_mode == RendererMode::FORWARD) {
		_forRenderer->draw(_userCamera, &_finalRender, nullptr);
	}
	_rendererTime.end();

	Texture& depthSrc = _mode == RendererMode::FORWARD ? _forRenderer->sceneDepth() : _defRenderer->sceneDepth();

	_postprocessTime.begin();
	_postprocess->process(_finalRender, _userCamera.projection(), depthSrc, _finalRender);
	_postprocessTime.end();

	if(_showDebug){
		_debugRenderer->draw(_userCamera, &_finalRender, &depthSrc);
	}

	// We now render a full screen quad in the default backbuffer.

	GPU::setDepthState(false);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

	GPU::beginRender(window());
	
	GPU::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
	_finalProgram->use();
	_finalProgram->texture(_finalRender, 0);
	GPU::drawQuad();
	GPU::endRender();

	_totalTime.end();
}

void PBRDemo::update() {
	CameraApp::update();

	// Performances window.
	if(ImGui::Begin("Performance")){
		ImGui::Text("%.1f ms, %.1f fps", frameTime() * 1000.0f, frameRate());
		ImGui::Text("Total CPU time: %05.1fms", float(_totalTime.value())/1000000.0f);
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
		// Save the viewpoint.
		const auto cam = _userCamera.encode();
		setScene(_scenes[_currentScene]);
		// Restore the viewpoint.
		_userCamera.decode(cam);
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
			if(ImGui::Combo("Shadow technique", reinterpret_cast<int*>(&_shadowMode), "None\0Basic\0PCF\0Variance\0\0")){
				createShadowMaps(_shadowMode);
			}
			
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
	_finalRender.resize(rw, rh);
}

void PBRDemo::createShadowMaps(ShadowMode mode){
	// Delete existing shadow maps.
	for(auto & map : _shadowMaps) {
		map.reset();
	}

	_shadowMaps.clear();
	const std::shared_ptr<Scene> & scene = _scenes[_currentScene];
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

	if(mode == ShadowMode::VARIANCE){
		if(!lights2D.empty()){
			_shadowMaps.emplace_back(new VarianceShadowMap2DArray(lights2D, glm::vec2(512)));
		}
		if(!lightsCube.empty()){
			_shadowMaps.emplace_back(new VarianceShadowMapCubeArray(lightsCube, 512));
		}
	} else if(mode == ShadowMode::BASIC || mode == ShadowMode::PCF){
		if(!lights2D.empty()){
			_shadowMaps.emplace_back(new BasicShadowMap2DArray(lights2D, glm::vec2(512), mode));
		}
		if(!lightsCube.empty()){
			_shadowMaps.emplace_back(new BasicShadowMapCubeArray(lightsCube, 512, mode));
		}
	} else {
		if(!lights2D.empty()){
			_shadowMaps.emplace_back(new EmptyShadowMap2DArray(lights2D));
		}
		if(!lightsCube.empty()){
			_shadowMaps.emplace_back(new EmptyShadowMapCubeArray(lightsCube));
		}
	}

	// Shadow pass.
	for(const auto & map : _shadowMaps) {
		map->draw(*_scenes[_currentScene]);
	}
}

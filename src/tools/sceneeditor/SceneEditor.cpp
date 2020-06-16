#include "SceneEditor.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"

SceneEditor::SceneEditor(RenderingConfig & config) : CameraApp(config) {
	_passthrough = Resources::manager().getProgram2D("passthrough");

	_sceneFramebuffer = _renderer.createOutput(uint(config.renderingResolution()[0]), uint(config.renderingResolution()[1]), "Scene render");
		 
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
	
	_renderer.setScene(scene);

}

void SceneEditor::draw() {

	// If no scene, just clear.
	if(!_scenes[_currentScene]) {
		Framebuffer::backbuffer()->bind();
		GLUtilities::clearColorAndDepth({0.2f, 0.2f, 0.2f, 1.0f}, 1.0f);
		Framebuffer::backbuffer()->unbind();
		return;
	}

	_renderer.draw(_userCamera, *_sceneFramebuffer);

	// We now render a full screen quad in the default framebuffer, using sRGB space.
	Framebuffer::backbuffer()->bind(Framebuffer::Mode::SRGB);
	GLUtilities::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
	_passthrough->use();
	_passthrough->uniform("flip", 0);
	ScreenQuad::draw(_sceneFramebuffer->texture());
	Framebuffer::backbuffer()->unbind();
	checkGLError();
}
void SceneEditor::update() {
	CameraApp::update();
	
	// Handle scene switching.
	if(ImGui::Begin("Scene")) {
		ImGui::Text("%.1f ms, %.1f fps", frameTime() * 1000.0f, frameRate());
		
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
	// Reload the scene.
	const auto & scene = _scenes[_currentScene];
	
	if(ImGui::Begin("Scene")){
		if(ImGui::Button("Reload") && _scenes[_currentScene]) {
			_scenes[_currentScene].reset(new Scene(_sceneNames[_currentScene]));
			setScene(_scenes[_currentScene]);
		}
		ImGui::SameLine();
		if(ImGui::Button("Save") && _scenes[_currentScene]) {
			const auto tokens = _scenes[_currentScene]->encode();
			Log::Info() << Codable::encode(tokens) << std::endl;
		}
		ImGui::Separator();
		ImGui::Checkbox("Pause animations", &_paused);
		// Camera settings.
		_userCamera.interface();
		// Copy/paste camera to clipboard.
		if(ImGui::Button("Define camera")) {
			_scenes[_currentScene]->setViewpoint(_userCamera);
		}
		ImGui::SameLine();
		// Reset to the scene reference viewpoint.
		if(ImGui::Button("Reset")) {
			_userCamera.apply(_scenes[_currentScene]->viewpoint());
			_userCamera.ratio(_config.screenResolution[0] / _config.screenResolution[1]);
		}
		ImGui::Separator();
		ImGui::ColorEdit3("Background", &scene->backgroundColor[0]);
		
	}
	ImGui::End();
	
	if(ImGui::Begin("Inspector") && _selectedObject >= 0) {
		if(_selectedObject < int(scene->objects.size())){
			ImGui::Text("Object %d", _selectedObject);
			const auto & obj = scene->objects[_selectedObject];
			ImGui::Text("Geometry: %s", obj.mesh()->name().c_str());
			
			ImGui::Text("Textures:");
			for(const auto tex : obj.textures()){
				ImGui::Text("%s", tex->name().c_str());
			}
		} else {
			const int lid = _selectedObject - int(scene->objects.size());
			ImGui::Text("Light %d", lid);
			auto & light = scene->lights[lid];
			std::string type = "Unknown";
			if(std::dynamic_pointer_cast<PointLight>(light)){
				type = "Omni";
			} else if(std::dynamic_pointer_cast<DirectionalLight>(light)){
				type = "Directional";
			} else if(std::dynamic_pointer_cast<SpotLight>(light)){
				type = "Spot";
			}
			ImGui::Text("Type: %s", type.c_str());
			glm::vec3 col = light->intensity();
			if(ImGui::DragFloat3("Color", &col[0])){
				light->setIntensity(col);
			}
			
		}
	}
	ImGui::End();
	
	if(ImGui::Begin("Elements")) {
		
		if(ImGui::TreeNode("Objects")){
			int id = 0;
			for(const auto & obj : scene->objects){
				ImGuiTreeNodeFlags nodeFlags = (_selectedObject == id) ? ImGuiTreeNodeFlags_Selected : 0;
				nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				ImGui::TreeNodeEx((void*)(intptr_t)id, nodeFlags, "%s", obj.mesh()->name().c_str());
				if (ImGui::IsItemClicked()){
					_selectedObject = id;
				}
				++id;
			}
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Lights")){
			int id = 0;
			
			for(const auto & light : scene->lights){
				// tentative cast.
				std::string type = "Unknown";
				if(std::dynamic_pointer_cast<PointLight>(light)){
					type = "Omni";
				} else if(std::dynamic_pointer_cast<DirectionalLight>(light)){
					type = "Directional";
				} else if(std::dynamic_pointer_cast<SpotLight>(light)){
					type = "Spot";
				}
				ImGuiTreeNodeFlags nodeFlags = (_selectedObject - int(scene->objects.size()) == id)  ? ImGuiTreeNodeFlags_Selected : 0;
				nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				ImGui::TreeNodeEx((void*)(intptr_t)id, nodeFlags, "%s %d", type.c_str(), id);
				if (ImGui::IsItemClicked()){
					_selectedObject = id + int(scene->objects.size());
				}
				++id;
			}
			ImGui::TreePop();
		}
		
	}
	ImGui::End();
}

void SceneEditor::physics(double fullTime, double frameTime) {
	if(_scenes[_currentScene] && !_paused) {
		_scenes[_currentScene]->update(fullTime, frameTime);
	}
}

void SceneEditor::resize() {
	_sceneFramebuffer->resize(_config.renderingResolution());
}

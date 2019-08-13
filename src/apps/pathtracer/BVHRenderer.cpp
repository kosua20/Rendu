#include "BVHRenderer.hpp"
#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include "resources/Texture.hpp"


BVHRenderer::BVHRenderer(RenderingConfig & config) : Renderer(config) {	
	// Setup camera parameters.
	_userCamera.ratio(config.screenResolution[0]/config.screenResolution[1]);
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	
	// GL setup
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGB8, true));
	_objectProgram = Resources::manager().getProgram("object_basic_lit");
	_bvhProgram = Resources::manager().getProgram("object_basic_color");
	_passthrough = Resources::manager().getProgram2D("passthrough");
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	checkGLError();
	
	// Initial setup for rendering image.
	_renderTex.shape = TextureShape::D2;
	_renderTex.levels = 1;
	_renderTex.width = renderWidth;
	_renderTex.height = renderHeight;
	GLUtilities::setupTexture(_renderTex, {GL_SRGB8, GL_LINEAR, GL_CLAMP_TO_EDGE});
}

void BVHRenderer::setScene(std::shared_ptr<Scene> scene){
	_scene = scene;
	if(!scene){
		return;
	}
	// Camera setup.
	_userCamera.apply(_scene->viewpoint());
	_userCamera.ratio(_renderResolution[0]/_renderResolution[1]);
	const BoundingBox & bbox = _scene->boundingBox();
	const float range = glm::length(bbox.getSize());
	_userCamera.frustum(0.01f*range, 5.0f*range);
	_userCamera.speed() = 0.2f*range;
	_cameraFOV = _userCamera.fov() * 180.0f / float(M_PI);
	
	// Create the path tracer and raycaster.
	_pathTracer = PathTracer(_scene);
	_visuHelper = std::unique_ptr<RaycasterVisualisation>(new RaycasterVisualisation(_pathTracer.raycaster()));
	
	// Build the BVH mesh.
	_visuHelper->getAllLevels(_bvhLevels);
	for(Mesh & level : _bvhLevels){
		// Setup the OpenGL mesh, don't keep the CPU mesh.
		level.upload();
		level.clearGeometry();
	}
	_bvhRange = glm::vec2(0, 0);
	checkGLError();
}

void BVHRenderer::draw() {
	glClearColor(0.2f,0.2f,0.2f,1.0f);
	
	// If no scene, just clear.
	if(!_scene){
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}
	
	// Directly render the result texture without drawing the scene.
	if(_showRender){
		glEnable(GL_FRAMEBUFFER_SRGB);
		glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
		glUseProgram(_passthrough->id());
		glUniform1i(_passthrough->uniform("flip"), 1);
		ScreenQuad::draw(_renderTex.gpu->id);
		glDisable(GL_FRAMEBUFFER_SRGB);
		return;
	}
	
	// Draw the scene.
	glEnable(GL_DEPTH_TEST);
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	
	const glm::mat4 & view = _userCamera.view();
	const glm::mat4 & proj = _userCamera.projection();
	const glm::mat4 VP = proj * view;
	
	glUseProgram(_objectProgram->id());
	for(auto & object : _scene->objects){
		// Combine the three matrices.
		const glm::mat4 MVP = VP * object.model();
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(object.model())));
		glUniformMatrix4fv(_objectProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix3fv(_objectProgram->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
		GLUtilities::drawMesh(*object.mesh());
	}
	
	// Debug wireframe visualisation.
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(_bvhProgram->id());
	glUniformMatrix4fv(_bvhProgram->uniform("mvp"), 1, GL_FALSE, &VP[0][0]);
	// If there is a ray mesh, show it.
	if(_rayVis.gpu && _rayVis.gpu->count > 0){
		GLUtilities::drawMesh(_rayVis);
		if(_showBVH){
			for(int lid = _bvhRange.x; lid <= _bvhRange.y; ++lid){
				if(lid >= int(_rayLevels.size())){
					break;
				}
				GLUtilities::drawMesh(_rayLevels[lid]);
			}
		}
	} else if(_showBVH){
		for(int lid = _bvhRange.x; lid <= _bvhRange.y; ++lid){
			GLUtilities::drawMesh(_bvhLevels[lid]);
		}
	}
	
	glUseProgram(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	_sceneFramebuffer->unbind();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	glEnable(GL_FRAMEBUFFER_SRGB);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glUseProgram(_passthrough->id());
	glUniform1i(_passthrough->uniform("flip"), 0);
	ScreenQuad::draw(_sceneFramebuffer->textureId());
	glDisable(GL_FRAMEBUFFER_SRGB);
	checkGLError();
}


void BVHRenderer::update(){
	Renderer::update();
	// If no scene, no need to udpate the camera or the scene-specific UI.
	if(!_scene){
		return;
	}
	_userCamera.update();
	
	if(ImGui::Begin("Path tracer")){
		
		ImGui::Text("Rendering size: %d x %d", _renderTex.width, _renderTex.height);
		
		// Tracing options
		ImGui::PushItemWidth(100);
		if(ImGui::InputInt("Samples", &_samples, 1, 4)){
			_samples = std::max(1, _samples);
		}
		if(ImGui::InputInt("Depth", &_depth, 1, 2)){
			_depth = std::max(1, _depth);
		}
		if(ImGui::InputScalar("Output height", ImGuiDataType_U32, (void*)(&_renderTex.height))){
			_renderTex.height = std::max((unsigned int)1, _renderTex.height);
			_renderTex.width = (unsigned int)std::round(_renderResolution[0]/_renderResolution[1] * _renderTex.height);
		}
		ImGui::PopItemWidth();
		
		// Perform rendering.
		if(ImGui::Button("Render")){
			// Clean the texture.
			_renderTex.clearImages();
			_renderTex.gpu->clean();
			GLUtilities::setupTexture(_renderTex, {GL_SRGB8, GL_LINEAR, GL_CLAMP_TO_EDGE});
			// Render.
			_renderTex.images.emplace_back(_renderTex.width, _renderTex.height, 3);
			Image & render = _renderTex.images.back();
			_pathTracer.render(_userCamera, _samples, _depth, render);
			// Upload to the GPU.
			GLUtilities::uploadTexture(_renderTex);
			_showRender = true;
		}
		ImGui::SameLine();
		// Save the render to disk.
		const bool hasImage = !_renderTex.images.empty();
		if(hasImage && ImGui::Button("Save...")){
			std::string outPath;
			if(System::showPicker(System::Picker::Save, "", outPath) && !outPath.empty()){
				ImageUtilities::saveLDRImage(outPath, _renderTex.images[0], false);
			}
		}
		
		ImGui::Checkbox("Show rendered image", &_showRender);
		if(!_showRender){
			// Mesh and BVH display.
			ImGui::Separator();
			ImGui::Checkbox("Show BVH", &_showBVH);
			ImGui::SameLine();
			// Keep both ends of the range equal.
			if(ImGui::Checkbox("Lock", &_lockLevel)){
				_bvhRange[1] = _bvhRange[0];
			}
			// Display a subset of the BVH.
			const int maxLevel = int(_bvhLevels.size())-1;
			const bool mod1 = ImGui::SliderInt("Range min.", &_bvhRange[0], 0, maxLevel);
			const bool mod2 = ImGui::SliderInt("Range max.", &_bvhRange[1], 0, maxLevel);
			if(mod1 || mod2){
				// Enforce synchronisation.
				_bvhRange[1] = glm::clamp(_bvhRange[1], _bvhRange[0], maxLevel);
				_bvhRange[0] = glm::clamp(_bvhRange[0], 0, _bvhRange[1]);
				if(_lockLevel){
					_bvhRange[1] = _bvhRange[0];
				}
			}
		}
		
		if(Input::manager().released(Input::MouseLeft) && Input::manager().pressed(Input::KeySpace)){
			castRay(Input::manager().mouse());
		}
		
		if(ImGui::Button("Clear ray")){
			_rayVis.clean();
			for(auto & level : _rayLevels){
				level.clean();
			}
			_rayLevels.clear();
		}
		
		// Camera settings.
		if(ImGui::CollapsingHeader("Camera settings")){
			ImGui::PushItemWidth(100);
			ImGui::Combo("Camera mode", (int*)(&_userCamera.mode()), "FPS\0Turntable\0Joystick\0\0", 3);
			ImGui::InputFloat("Camera speed", &_userCamera.speed(), 0.1f, 1.0f);
			if(ImGui::InputFloat("Camera FOV", &_cameraFOV, 1.0f, 10.0f)){
				_userCamera.fov(_cameraFOV*float(M_PI)/180.0f);
			}
			ImGui::PopItemWidth();
			
			// Copy/paste camera to clipboard.
			if(ImGui::Button("Copy camera")){
				const std::string camDesc = _userCamera.encode();
				ImGui::SetClipboardText(camDesc.c_str());
			}
			ImGui::SameLine();
			if(ImGui::Button("Paste camera")){
				const std::string camDesc(ImGui::GetClipboardText());
				const auto cameraCode = Codable::parse(camDesc);
				if(!cameraCode.empty()){
					_userCamera.decode(cameraCode[0]);
					_cameraFOV = _userCamera.fov()*180.0f/float(M_PI);
				}
			}
			// Reset to the scene reference viewpoint.
			if(ImGui::Button("Reset")){
				_userCamera.apply(_scene->viewpoint());
				_userCamera.ratio(_renderResolution[0]/_renderResolution[1]);
				_cameraFOV = _userCamera.fov()*180.0f/float(M_PI);
			}
		}
	}
	ImGui::End();
}

void BVHRenderer::physics(double , double frameTime){
	_userCamera.physics(frameTime);
	// If there is any interaction, exit the 'show render' mode.
	if(Input::manager().interacted()){
		_showRender = false;
	}
}

void BVHRenderer::clean() {
	Renderer::clean();
	_sceneFramebuffer->clean();
	if(_scene){
		_scene->clean();
	}
	for(Mesh & level : _bvhLevels){
		level.clean();
	}
	for(Mesh & level : _rayLevels){
		level.clean();
	}
	_rayVis.clean();
	glDeleteTextures(1, &_renderTex.gpu->id);
}

void BVHRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
	// Resize the framebuffers.
	_sceneFramebuffer->resize(_renderResolution);
	// Udpate the image resolution, using the new aspect ratio.
	_renderTex.width = (unsigned int)std::round(_renderResolution[0]/_renderResolution[1] * _renderTex.height);
	checkGLError();
}

void BVHRenderer::castRay(const glm::vec2 & position){
	// Compute incremental pixel shifts.
	glm::vec3 corner, dx, dy;
	_userCamera.pixelShifts(corner, dx, dy);
	const glm::vec3 worldPos = corner + position.x * dx + position.y * dy;
	const glm::vec3 rayPos = _userCamera.position();
	const glm::vec3 rayDir = glm::normalize(worldPos - rayPos);
	
	// Intersect.
	const Raycaster::RayHit hit = _visuHelper->getRayLevels(rayPos, rayDir, _rayLevels);
	// Level meshes.
	for(Mesh & level : _rayLevels){
		// Setup the OpenGL mesh, don't keep the CPU mesh.
		level.upload();
		level.clearGeometry();
	}
	// Ray and intersection mesh.
	const float defaultLength = 3.0f * glm::length(_scene->boundingBox().getSize());
	
	_visuHelper->getRayMesh(rayPos, rayDir, hit, _rayVis, defaultLength);
	_rayVis.upload();
	_rayVis.clearGeometry();
}

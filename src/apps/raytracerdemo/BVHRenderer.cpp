#include "BVHRenderer.hpp"
#include "input/Input.hpp"
#include "helpers/System.hpp"


BVHRenderer::BVHRenderer(RenderingConfig & config) : Renderer(config) {	
	// Setup camera parameters.
	_userCamera.ratio(config.screenResolution[0]/config.screenResolution[1]);
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	
	// GL setup
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGB8, true));
	_objectProgram = Resources::manager().getProgram("object_basic_lit");
	_passthrough = Resources::manager().getProgram2D("passthrough");
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	checkGLError();
	
	// Initial setup for rendering image.
	_renderTex.array = false;
	_renderTex.cubemap = false;
	_renderTex.descriptor = {GL_SRGB8, GL_LINEAR, GL_CLAMP_TO_EDGE};
	_renderTex.mipmap = 1;
	_renderTex.width = renderWidth;
	_renderTex.height = renderHeight;
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
	_userCamera.speed() = 0.2f*range;
	_cameraFOV = _userCamera.fov() * 180.0f / float(M_PI);
	
	// Create the path tracer and raycaster.
	_pathTracer = PathTracer(_scene);
	
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
		ScreenQuad::draw(_renderTex.id);
		glDisable(GL_FRAMEBUFFER_SRGB);
		checkGLError();
		return;
	}
	
	// Draw the scene.
	glEnable(GL_DEPTH_TEST);
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	
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
	glUseProgram(0);
	_sceneFramebuffer->unbind();
	glDisable(GL_DEPTH_TEST);
	
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
			_renderTex.images.clear();
			_renderTex.images.emplace_back(_renderTex.width, _renderTex.height, 3);
			Image & render = _renderTex.images.back();
			_pathTracer.render(_userCamera, _samples, _depth, render);
			
			// Remove previous texture.
			glDeleteTextures(1, &_renderTex.id);
			// Upload to the GPU.
			_renderTex.id  = GLUtilities::createTexture(GL_TEXTURE_2D, _renderTex.descriptor, _renderTex.mipmap);
			GLUtilities::uploadTexture(GL_TEXTURE_2D, _renderTex.id, GL_SRGB8, 0, 0, render);
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

void BVHRenderer::clean() const {
	Renderer::clean();
	_sceneFramebuffer->clean();
	if(_scene){
		_scene->clean();
	}
}

void BVHRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
	// Resize the framebuffers.
	_sceneFramebuffer->resize(_renderResolution);
	// Udpate the image resolution, using the new aspect ratio.
	_renderTex.width = (unsigned int)std::round(_renderResolution[0]/_renderResolution[1] * _renderTex.height);
	checkGLError();
}

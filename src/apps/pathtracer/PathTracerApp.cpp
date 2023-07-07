#include "PathTracerApp.hpp"
#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"
#include "resources/Texture.hpp"
#include "system/Window.hpp"

PathTracerApp::PathTracerApp(RenderingConfig & config, Window & window, const std::shared_ptr<Scene> & scene) :
	CameraApp(config, window), _renderTex ("render"), _sceneColor("Visualisation color"), _sceneDepth("Visualisation depth") {

	_bvhRenderer.reset(new BVHRenderer());
	const glm::vec2 renderRes = _config.renderingResolution();
	const uint w = uint(renderRes[0]);
	const uint h = uint(renderRes[1]);
	_sceneColor.setupAsDrawable(_bvhRenderer->outputColorFormat(), w, h);
	_sceneDepth.setupAsDrawable(_bvhRenderer->outputDepthFormat(), w, h);
	_passthrough = Resources::manager().getProgram2D("tonemap");
	
	// Initial setup for rendering image.
	_renderTex.shape  = TextureShape::D2;
	_renderTex.levels = 1;
	_renderTex.depth  = 1;
	_renderTex.width  = int(renderRes[0]);
	_renderTex.height = int(renderRes[1]);
	_renderTex.format = Layout::RGBA8;
	GPU::setupTexture(_renderTex);
	
	_scene = scene;
	if(!scene) {
		return;
	}
	// Camera setup.
	_userCamera.apply(_scene->viewpoint());
	const BoundingBox & bbox = _scene->boundingBox();
	const float range		 = glm::length(bbox.getSize());
	_userCamera.frustum(0.01f * range, 5.0f * range);
	_userCamera.speed() = 0.2f * range;
	_userCamera.ratio(config.screenResolution[0] / config.screenResolution[1]);
	
	// Create the path tracer and raycaster.
	_pathTracer.reset(new PathTracer(_scene));
	// Setup the renderer data.
	_bvhRenderer->setScene(_scene, _pathTracer->raycaster());
	
}

void PathTracerApp::draw() {

	// If no scene, just clear.
	if(!_scene) {
		GPU::beginRender(window(), 1.0f, Load::Operation::DONTCARE, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		GPU::endRender();
		return;
	}
	
	// If we are rendering live, perform path tracing on the fly.
	if(_liveRender){
		_renderTex.clean();
		// Render.
		_renderTex.images.emplace_back(_renderTex.width, _renderTex.height, 4);
		Image & render = _renderTex.images.back();
		_pathTracer->render(_userCamera, _samples, _depth, render);
		// Upload to the GPU.
		_renderTex.upload(Layout::RGBA8, false);
		_showRender = true;
	}
	
	// Directly render the result texture without drawing the scene.
	if(_showRender) {
		GPU::setBlendState(false);
		GPU::setDepthState(false);
		GPU::setCullState(true, Faces::BACK);
		GPU::beginRender(window());
		window().setViewport();
		GPU::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
		_passthrough->use();
		_passthrough->uniform("apply", true);
		_passthrough->uniform("customExposure", _exposure);
		_passthrough->texture(_renderTex, 0);
		GPU::drawQuad();
		GPU::endRender();
		return;
	}
	
	// Draw the real time visualization.
	_bvhRenderer->draw(_userCamera, &_sceneColor, &_sceneDepth);
	// We now render a full screen quad in the default framebuffer.
	GPU::setBlendState(false);
	GPU::setDepthState(false);
	GPU::setCullState(true, Faces::BACK);
	GPU::beginRender(window());
	window().setViewport();
	_passthrough->use();
	_passthrough->uniform("apply", false);
	_passthrough->texture(_sceneColor, 0);
	GPU::drawQuad();
	GPU::endRender();
}

void PathTracerApp::update() {
	CameraApp::update();
	
	// If no scene, no need to udpate the camera or the scene-specific UI.
	if(!_scene) {
		return;
	}

	if(ImGui::Begin("Path tracer")) {

		ImGui::Text("Rendering size: %d x %d", _renderTex.width, _renderTex.height);

		// Tracing options
		ImGui::PushItemWidth(100);
		if(ImGui::InputInt("Samples", &_samples, 1, 4)) {
			_samples = std::max(1, _samples);
		}
		if(ImGui::InputInt("Depth", &_depth, 1, 2)) {
			_depth = std::max(1, _depth);
		}
		if(ImGui::InputScalar("Output height", ImGuiDataType_U32, static_cast<void *>(&_renderTex.height))) {
			_renderTex.height = std::max(uint(1), _renderTex.height);
			_renderTex.width  = uint(std::round(_config.screenResolution[0] / _config.screenResolution[1] * float(_renderTex.height)));
		}
		ImGui::PopItemWidth();

		// Perform rendering.
		if(ImGui::Button("Render")) {
			_renderTex.clean();
			// Render.
			_renderTex.images.emplace_back(_renderTex.width, _renderTex.height, 4);
			Image & render = _renderTex.images.back();
			_pathTracer->render(_userCamera, _samples, _depth, render);
			// Upload to the GPU.
			_renderTex.upload(Layout::RGBA8, false);
			_showRender = true;
		}
		ImGui::SameLine();
		// Save the render to disk.
		const bool hasImage = !_renderTex.images.empty();
		if(hasImage && ImGui::Button("Save...")) {
			std::string outPath;
			if(System::showPicker(System::Picker::Save, "", outPath) && !outPath.empty()) {
				// Tonemap the image if needed.
				if(!Image::isFloat(outPath)){
					Image& renderImg = _renderTex.images[0];
					const float exposure = _exposure;
					System::forParallel(0, renderImg.height, [&renderImg, &exposure](size_t y){
						for(uint x = 0; x < renderImg.width; ++x){
							const glm::vec3 & color = renderImg.rgb(int(x), int(y));
							renderImg.rgb(int(x), int(y)) = glm::vec3(1.0f) - glm::exp(-exposure * color);
						}
					});
				}
				_renderTex.images[0].save(outPath, Image::Save::SRGB_LDR | Image::Save::IGNORE_ALPHA);
			}
		}
		
		ImGui::Checkbox("Show render", &_showRender); ImGui::SameLine();
		ImGui::Checkbox("Live render", &_liveRender);
		if(_showRender){
			ImGui::SliderFloat("Exposure", &_exposure, 0.1f, 10.0f);
		}
		
		if(!_showRender) {
			// Mesh and BVH display.
			ImGui::Separator();
			ImGui::Checkbox("Show BVH", &_bvhRenderer->showBVH());
			ImGui::SameLine();
			
			int & minRange = _bvhRenderer->range()[0];
			int & maxRange = _bvhRenderer->range()[1];
			// Keep both ends of the range equal.
			if(ImGui::Checkbox("Lock", &_lockLevel)) {
				maxRange = minRange;
			}
			// Display a subset of the BVH.
			const int maxLevel = _bvhRenderer->maxLevel();
			const bool mod1	= ImGui::SliderInt("Range min.", &minRange, 0, maxLevel);
			const bool mod2	= ImGui::SliderInt("Range max.", &maxRange, 0, maxLevel);
			if(mod1 || mod2) {
				// Enforce synchronisation.
				maxRange = glm::clamp(maxRange, minRange, maxLevel);
				minRange = glm::clamp(minRange, 0, maxRange);
				if(_lockLevel) {
					maxRange = minRange;
				}
			}
		}

		if(Input::manager().released(Input::Mouse::Left) && Input::manager().pressed(Input::Key::Space)) {
			const glm::vec2 position = Input::manager().mouse();
			// Compute incremental pixel shifts.
			glm::vec3 corner, dx, dy;
			_userCamera.pixelShifts(corner, dx, dy);
			const glm::vec3 worldPos = corner + position.x * dx + position.y * dy;
			const glm::vec3 rayPos   = _userCamera.position();
			const glm::vec3 rayDir   = glm::normalize(worldPos - rayPos);
			_bvhRenderer->castRay(rayPos, rayDir);
		}

		if(ImGui::Button("Clear ray")) {
			_bvhRenderer->clearRay();
		}

		// Camera settings.
		if(ImGui::CollapsingHeader("Camera settings")) {
			_userCamera.interface();
			// Reset to the scene reference viewpoint.
			if(ImGui::Button("Reset")) {
				_userCamera.apply(_scene->viewpoint());
				_userCamera.ratio(_config.screenResolution[0] / _config.screenResolution[1]);
			}
		}
	}
	ImGui::End();
	
}

void PathTracerApp::physics(double, double) {
	// If there is any interaction, exit the 'show render' mode except if we are live rendering.
	if(Input::manager().interacted() && !_liveRender) {
		_showRender = false;
	}
}

void PathTracerApp::resize() {
	// Same aspect ratio as the display resolution
	const glm::vec2 renderRes = _config.renderingResolution();
	_sceneColor.resize(renderRes);
	_sceneDepth.resize(renderRes);
	// Udpate the image resolution, using the new aspect ratio.
	_renderTex.width = uint(std::round(_config.screenResolution[0] / _config.screenResolution[1] * float(_renderTex.height)));
}

#include "FilteringApp.hpp"

#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"

FilteringApp::FilteringApp(RenderingConfig & config) :
	CameraApp(config) {
	
	const glm::vec2 renderResolution = _config.renderingResolution();
	// Setup camera parameters.
	_userCamera.projection(config.screenResolution[0] / config.screenResolution[1], 1.3f, 0.01f, 200.0f);
	_userCamera.pose(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	const int renderWidth  = int(renderResolution[0]);
	const int renderHeight = int(renderResolution[1]);

	_passthrough = Resources::manager().getProgram2D("passthrough");
	_sceneShader = Resources::manager().getProgram("object", "object_basic", "object_basic_random");
	_mesh		 = Resources::manager().getMesh("light_sphere", Storage::GPU);

	_sceneBuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, {Layout::RGB8, Filter::NEAREST_NEAREST, Wrap::CLAMP}, true));

	// Create the Poisson filling and Laplacian integration pyramids, with a lowered internal resolution to speed things up.
	_pyramidFiller	 = std::unique_ptr<PoissonFiller>(new PoissonFiller(renderWidth, renderHeight, _fillDownscale));
	_pyramidIntegrator = std::unique_ptr<LaplacianIntegrator>(new LaplacianIntegrator(renderWidth, renderHeight, _intDownscale));
	_gaussianBlur	  = std::unique_ptr<GaussianBlur>(new GaussianBlur(_blurLevel, 1));
	_boxBlur		   = std::unique_ptr<BoxBlur>(new BoxBlur(false));
	_floodFill		   = std::unique_ptr<FloodFiller>(new FloodFiller(renderWidth, renderHeight));

	_painter = std::unique_ptr<PaintingTool>(new PaintingTool(renderWidth, renderHeight));

	// GL options
	GLUtilities::setDepthState(true);
	GLUtilities::setCullState(true);
	checkGLError();
}

void FilteringApp::draw() {

	const Texture * srcTexID = _sceneBuffer->texture();
	// Render the scene.
	if(_viewMode == View::SCENE) {
		GLUtilities::setDepthState(true);
		_sceneBuffer->bind();
		_sceneBuffer->setViewport();
		GLUtilities::clearColorAndDepth({0.0f, 0.0f, 0.0f, 1.0f}, 1.0f);
		const glm::mat4 MVP = _userCamera.projection() * _userCamera.view();
		_sceneShader->use();
		_sceneShader->uniform("mvp", MVP);
		GLUtilities::drawMesh(*_mesh);
		_sceneBuffer->unbind();

	} else if(_viewMode == View::IMAGE) {
		GLUtilities::setDepthState(false);
		_sceneBuffer->bind();
		_sceneBuffer->setViewport();
		_passthrough->use();
		if(_image.width > 0) {
			ScreenQuad::draw(_image);
		} else {
			GLUtilities::clearColor({0.0f, 0.0f, 0.0f, 1.0f});
		}
		_sceneBuffer->unbind();

	} else {
		GLUtilities::setDepthState(false);
		_painter->draw();
		// If we are in INPUT mode, we want to display the frame with the brush outline visible.
		// On the other hand, if we apply any processing, hide the brush and use the canvas frame.
		srcTexID = _mode == Processing::INPUT ? _painter->visuId() : _painter->texture();
	}

	GLUtilities::setDepthState(false);

	const Texture * finalTexID = srcTexID;

	switch(_mode) {
		case Processing::FILL:
			_pyramidFiller->process(srcTexID);
			finalTexID = _showProcInput ? _pyramidFiller->preprocId() : _pyramidFiller->texture();
			break;
		case Processing::INTEGRATE:
			_pyramidIntegrator->process(srcTexID);
			finalTexID = _showProcInput ? _pyramidIntegrator->preprocId() : _pyramidIntegrator->texture();
			break;
		case Processing::GAUSSBLUR:
			_gaussianBlur->process(srcTexID, *_sceneBuffer);
			finalTexID = _sceneBuffer->texture();
			break;
		case Processing::BOXBLUR:
			_boxBlur->process(srcTexID, *_sceneBuffer);
			finalTexID = _sceneBuffer->texture();
			break;
		case Processing::FLOODFILL:
			_floodFill->process(srcTexID, _showProcInput ? FloodFiller::Output::DISTANCE : FloodFiller::Output::COLOR);
			finalTexID = _floodFill->texture();
			break;
		default:
			// Show the input.
			break;
	}

	// Render the output on screen.
	Framebuffer::backbuffer()->bind();
	const glm::ivec2 screenSize = Input::manager().size();
	GLUtilities::setViewport(0, 0, screenSize[0], screenSize[1]);
	_passthrough->use();
	ScreenQuad::draw(finalTexID);
	Framebuffer::backbuffer()->unbind();
}

void FilteringApp::update() {
	CameraApp::update();

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
	if(ImGui::Begin("Filtering")) {
		// Infos.
		ImGui::Text("%.1f ms, %.1f fps", frameTime() * 1000.0f, frameRate());
		ImGui::Text("Input resolution: %ix%i", _sceneBuffer->width(), _sceneBuffer->height());
		ImGui::Separator();

		// View settings.
		ImGui::Text("View:");
		ImGui::SameLine();
		const bool t0 = ImGui::RadioButton("Scene", reinterpret_cast<int *>(&_viewMode), int(View::SCENE));
		ImGui::SameLine();
		const bool t1 = ImGui::RadioButton("Image", reinterpret_cast<int *>(&_viewMode), int(View::IMAGE));
		ImGui::SameLine();
		const bool t2 = ImGui::RadioButton("Paint", reinterpret_cast<int *>(&_viewMode), int(View::PAINT));
		if(t0 || t1 || t2){
			freezeCamera(_viewMode != View::SCENE);
		}
		// Image loading options for the image mode.
		if(_viewMode == View::IMAGE && ImGui::Button("Load image...")) {
			std::string newImagePath;
			const bool res = System::showPicker(System::Picker::Load, "./", newImagePath, "jpg,bmp,png,tga;exr");
			// If user picked a path, load the texture from disk.
			if(res && !newImagePath.empty()) {
				Log::Info() << "Loading " << newImagePath << "." << std::endl;

				_image.clean();
				_image.shape  = TextureShape::D2;
				_image.depth  = 1;
				_image.levels = 1;
				_image.images.emplace_back();
				Image & img   = _image.images.back();
				const int ret = img.load(newImagePath, 4, true, false);
				if(ret != 0) {
					Log::Error() << Log::Resources << "Unable to load the texture at path " << newImagePath << "." << std::endl;
				} else {
					_image.width  = img.width;
					_image.height = img.height;
					_image.upload({Layout::RGBA8, Filter::NEAREST_NEAREST, Wrap::CLAMP}, false);
					_image.clearImages();
					_config.screenResolution = {_image.width, _image.height};
					resize();
				}
			}
		}

		if(ImGui::InputInt("Vertical res.", &_config.internalVerticalResolution, 50, 200)) {
			resize();
		}

		// Filter mode.
		ImGui::Separator();
		showModeOptions();
	}
	ImGui::End();

	// Place the painter window below, if we are in painting mode.
	if(_viewMode == View::PAINT) {
		ImGui::SetNextWindowPos(ImVec2(10, 200), ImGuiCond_Once);
		_painter->update();
	}
	
}

void FilteringApp::showModeOptions() {
	ImGui::Combo("Mode", reinterpret_cast<int *>(&_mode), "Input\0Poisson fill\0Integrate\0Box blur\0Gaussian blur\0Flood fill\0\0");

	const glm::vec2 renderRes = _config.renderingResolution();
	const unsigned int width  = uint(renderRes[0]);
	const unsigned int height = uint(renderRes[1]);

	switch(_mode) {
		case Processing::GAUSSBLUR:
			if(ImGui::InputInt("Levels", &_blurLevel, 1, 2)) {
				_blurLevel = std::min(std::max(1, _blurLevel), 10);
				_gaussianBlur.reset(new GaussianBlur(_blurLevel, 1));
			}
			break;
		case Processing::FILL:
			ImGui::Checkbox("Show colored border", &_showProcInput);
			if(ImGui::InputInt("Pyramid downscale", &_fillDownscale, 1, 2)) {
				_fillDownscale = std::max(_fillDownscale, 1);
				_pyramidFiller = std::unique_ptr<PoissonFiller>(new PoissonFiller(width, height, _fillDownscale));
			}
			break;
		case Processing::INTEGRATE:
			ImGui::Checkbox("Show Laplacian", &_showProcInput);
			if(ImGui::InputInt("Pyramid downscale", &_intDownscale, 1, 2)) {
				_intDownscale = std::max(_intDownscale, 1);
				_pyramidIntegrator = std::unique_ptr<LaplacianIntegrator>(new LaplacianIntegrator(width, height, _intDownscale));
			}
			break;
		case Processing::FLOODFILL:
			ImGui::Checkbox("Show distance", &_showProcInput);
			break;
		default:
			break;
	}
}

void FilteringApp::physics(double, double) {
}

void FilteringApp::resize() {
	const glm::vec2 renderResolution = _config.renderingResolution();
	// Resize the framebuffers.
	_sceneBuffer->resize(renderResolution);
	const uint lwidth  = uint(renderResolution[0]);
	const uint lheight = uint(renderResolution[1]);
	_pyramidFiller->resize(lwidth, lheight);
	_pyramidIntegrator->resize(lwidth, lheight);
	_floodFill->resize(lwidth, lheight);
	_painter->resize(lwidth, lheight);

	checkGLError();
}

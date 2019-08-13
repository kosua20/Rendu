#include "FilteringRenderer.hpp"

#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"

FilteringRenderer::FilteringRenderer(RenderingConfig & config) : Renderer(config) {
	
	// Setup camera parameters.
	_userCamera.projection(config.screenResolution[0]/config.screenResolution[1], 1.3f, 0.01f, 200.0f);
	_userCamera.pose(glm::vec3(0.0,0.0,3.0), glm::vec3(0.0,0.0,0.0), glm::vec3(0.0,1.0,0.0));
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	
	_passthrough = Resources::manager().getProgram2D("passthrough");
	_sceneShader = Resources::manager().getProgram("object", "object_basic", "object_basic_random");
	_mesh = Resources::manager().getMesh("light_sphere", GPU);
	
	_sceneBuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, {GL_RGB8, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE}, true));
	
	// Create the Poisson filling and Laplacian integration pyramids, with a lowered internal resolution to speed things up.
	_pyramidFiller = std::unique_ptr<PoissonFiller>(new PoissonFiller(renderWidth, renderHeight, _fillDownscale));
	_pyramidIntegrator = std::unique_ptr<LaplacianIntegrator>( new LaplacianIntegrator(renderWidth, renderHeight, _intDownscale));
	_gaussianBlur = std::unique_ptr<GaussianBlur>(new GaussianBlur(renderWidth, renderHeight, _blurLevel, GL_RGB8));
	_boxBlur = std::unique_ptr<BoxBlur>(new BoxBlur(renderWidth, renderHeight, false, {GL_RGB8, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE}));
	_floodFill = std::unique_ptr<FloodFiller>(new FloodFiller(renderWidth, renderHeight));
	
	_painter = std::unique_ptr<PaintingTool>(new PaintingTool(renderWidth, renderHeight));
	
	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	checkGLError();
	
}

void FilteringRenderer::draw() {
	
	GLuint srcTexID = _sceneBuffer->textureId();
	// Render the scene.
	if(_viewMode == View::SCENE){
		glEnable(GL_DEPTH_TEST);
		_sceneBuffer->bind();
		_sceneBuffer->setViewport();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		const glm::mat4 MVP = _userCamera.projection() * _userCamera.view();
		glUseProgram(_sceneShader->id());
		glUniformMatrix4fv(_sceneShader->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		GLUtilities::drawMesh(*_mesh);
		glBindVertexArray(0);
		glUseProgram(0);
		_sceneBuffer->unbind();
	} else if(_viewMode == View::IMAGE){
		glDisable(GL_DEPTH_TEST);
		_sceneBuffer->bind();
		_sceneBuffer->setViewport();
		glUseProgram(_passthrough->id());
		if(_image.width > 0){
			ScreenQuad::draw(_image.gpu->id);
		} else {
			glClear(GL_COLOR_BUFFER_BIT);
		}
		glUseProgram(0);
		_sceneBuffer->unbind();
	} else {
		glDisable(GL_DEPTH_TEST);
		_painter->draw();
		// If we are in INPUT mode, we want to display the frame with the brush outline visible.
		// On the other hand, if we apply any processing, hide the brush and use the canvas frame.
		srcTexID = (_mode == Filter::INPUT) ? _painter->visuId() : _painter->textureId();
	}
	
	glDisable(GL_DEPTH_TEST);
	
	
	GLuint finalTexID = srcTexID;
	
	switch(_mode){
		case Filter::FILL:
			_pyramidFiller->process(srcTexID);
			finalTexID = _showProcInput ? _pyramidFiller->preprocId() : _pyramidFiller->textureId();
			break;
		case Filter::INTEGRATE:
			_pyramidIntegrator->process(srcTexID);
			finalTexID = _showProcInput ? _pyramidIntegrator->preprocId() :_pyramidIntegrator->textureId();
			break;
		case Filter::GAUSSBLUR:
			_gaussianBlur->process(srcTexID);
			finalTexID = _gaussianBlur->textureId();
			break;
		case Filter::BOXBLUR:
			_boxBlur->process(srcTexID);
			finalTexID = _boxBlur->textureId();
			break;
		case Filter::FLOODFILL:
			_floodFill->process(srcTexID, _showProcInput ? FloodFiller::DISTANCE : FloodFiller::COLOR);
			finalTexID = _floodFill->textureId();
		default:
			// Show the input.
			break;
	}
	
	// Render the output on screen.
	const glm::vec2 screenSize = Input::manager().size();
	glViewport(0, 0, GLsizei(screenSize[0]), GLsizei(screenSize[1]));
	glUseProgram(_passthrough->id());
	ScreenQuad::draw(finalTexID);
	glUseProgram(0);
}

void FilteringRenderer::update(){
	Renderer::update();
	_userCamera.update();
	
	ImGui::SetNextWindowPos(ImVec2(10,10), ImGuiCond_Once);
	if(ImGui::Begin("Filtering")){
		// Infos.
		ImGui::Text("%.2f ms, %.1f fps", ImGui::GetIO().DeltaTime * 1000.0f, 1.0f/ImGui::GetIO().DeltaTime);
		ImGui::Text("Input resolution: %ix%i", _sceneBuffer->width(), _sceneBuffer->height());
		ImGui::Separator();
		
		// View settings.
		ImGui::Text("View:"); ImGui::SameLine();
		ImGui::RadioButton("Scene", (int*)&_viewMode, int(View::SCENE));
		ImGui::SameLine();
		ImGui::RadioButton("Image", (int*)&_viewMode, int(View::IMAGE));
		ImGui::SameLine();
		ImGui::RadioButton("Paint", (int*)&_viewMode, int(View::PAINT));
		
		// Image loading options for the image mode.
		if(_viewMode == View::IMAGE){
			if(ImGui::Button("Load image...")){
				std::string newImagePath;
				const bool res = System::showPicker(System::Picker::Load, "./", newImagePath, "jpg,bmp,png,tga;exr");
				// If user picked a path, load the texture from disk.
				if(res && !newImagePath.empty()){
					Log::Info() << "Loading " << newImagePath << "." << std::endl;
					
					_image.clean();
					_image.shape = TextureShape::D2;
					_image.levels = 1;
					_image.images.emplace_back();
					Image & img = _image.images.back();
					const int ret = ImageUtilities::loadImage(newImagePath, 4, true, false, img);
					if (ret != 0) {
						Log::Error() << Log::Resources << "Unable to load the texture at path " << newImagePath << "." << std::endl;
					} else {
						_image.width = img.width;
						_image.height = img.height;
						_image.upload({ GL_RGBA8, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE }, false);
						_image.clearImages();
						resize(_image.width, _image.height);
					}
				}
			}
		}
		
		if(ImGui::InputInt("Vertical res.", &_config.internalVerticalResolution, 50, 200)){
			resize(int(_config.screenResolution[0]), int(_config.screenResolution[1]));
		}
		
		// Filter mode.
		ImGui::Separator();
		ImGui::Combo("Mode", (int*)&_mode, "Input\0Poisson fill\0Integrate\0Box blur\0Gaussian blur\0Flood fill\0\0");

		const unsigned int width  = (unsigned int)(_renderResolution[0]);
		const unsigned int height = (unsigned int)(_renderResolution[1]);

		// Mode specific option
		switch(_mode){
			case Filter::GAUSSBLUR:
				if(ImGui::InputInt("Levels", &_blurLevel, 1, 2)){
					_blurLevel = std::min(std::max(1, _blurLevel), 10);
					_gaussianBlur->clean();
					_gaussianBlur = std::unique_ptr<GaussianBlur>(new GaussianBlur(width, height, _blurLevel, GL_RGB8));
				}
				break;
			case Filter::FILL:
				ImGui::Checkbox("Show colored border", &_showProcInput);
				if(ImGui::InputInt("Pyramid downscale", &_fillDownscale, 1, 2)){
					_fillDownscale = std::max(_fillDownscale, 1);
					_pyramidFiller->clean();
					_pyramidFiller = std::unique_ptr<PoissonFiller>(new PoissonFiller(width, height, _fillDownscale));
				}
				break;
			case Filter::INTEGRATE:
				ImGui::Checkbox("Show Laplacian", &_showProcInput);
				if(ImGui::InputInt("Pyramid downscale", &_intDownscale, 1, 2)){
					_intDownscale = std::max(_intDownscale, 1);
					_pyramidIntegrator->clean();
					_pyramidIntegrator = std::unique_ptr<LaplacianIntegrator>(new LaplacianIntegrator(width, height, _intDownscale));
				}
				break;
			case Filter::FLOODFILL:
				ImGui::Checkbox("Show distance", &_showProcInput);
				break;
			default:
				break;
		}
		
	}
	ImGui::End();
	
	// Place the painter window below, if we are in painting mode.
	if(_viewMode == View::PAINT){
		ImGui::SetNextWindowPos(ImVec2(10,200), ImGuiCond_Once);
		_painter->update();
	}
}

void FilteringRenderer::physics(double, double frameTime){
	// Only update the user camera if we are in Scene mode, to avoid moving accidentally while in other modes.
	if(_viewMode == View::SCENE){
		_userCamera.physics(frameTime);
	}
}


void FilteringRenderer::clean() {
	Renderer::clean();
	// Clean objects.
	_sceneBuffer->clean();
	_pyramidFiller->clean();
	_pyramidIntegrator->clean();
	_gaussianBlur->clean();
	_boxBlur->clean();
	_floodFill->clean();
	_painter->clean();
}


void FilteringRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
	// Resize the framebuffers.
	_sceneBuffer->resize(_renderResolution);
	const unsigned int lwidth  = (unsigned int)(_renderResolution[0]);
	const unsigned int lheight = (unsigned int)(_renderResolution[1]);
	_pyramidFiller->resize(lwidth, lheight);
	_pyramidIntegrator->resize(lwidth, lheight);
	_gaussianBlur->resize(lwidth, lheight);
	_boxBlur->resize(lwidth, lheight);
	_floodFill->resize(lwidth, lheight);
	_painter->resize(lwidth, lheight);
	
	checkGLError();
}

#include "FilteringRenderer.hpp"

#include "input/Input.hpp"
#include "helpers/InterfaceUtilities.hpp"

FilteringRenderer::FilteringRenderer(RenderingConfig & config) : Renderer(config) {
	
	// Setup camera parameters.
	_userCamera.projection(config.screenResolution[0]/config.screenResolution[1], 1.3f, 0.01f, 200.0f);
	_userCamera.pose(glm::vec3(0.0,0.0,3.0), glm::vec3(0.0,0.0,0.0), glm::vec3(0.0,1.0,0.0));
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	
	_passthrough = Resources::manager().getProgram2D("passthrough");
	_sceneShader = Resources::manager().getProgram("object_basic");
	_mesh = Resources::manager().getMesh("light_sphere");
	
	_sceneBuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, {GL_RGB8, GL_NEAREST, GL_CLAMP_TO_EDGE}, true));
	
	// Create the Poisson filling and Laplacian integration pyramids, with a lowered internal resolution to speed things up.
	_pyramidFiller = std::unique_ptr<PoissonFiller>(new PoissonFiller(renderWidth, renderHeight, _fillDownscale));
	_pyramidIntegrator = std::unique_ptr<LaplacianIntegrator>( new LaplacianIntegrator(renderWidth, renderHeight, _intDownscale));
	_gaussianBlur = std::unique_ptr<GaussianBlur>(new GaussianBlur(renderWidth, renderHeight, _blurLevel, GL_RGB8));
	_boxBlur = std::unique_ptr<BoxBlur>(new BoxBlur(renderWidth, renderHeight, false, {GL_RGB8, GL_NEAREST, GL_CLAMP_TO_EDGE}));
	
	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	checkGLError();
	
}

void FilteringRenderer::draw() {
	
	// Render the scene.
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
	
	glDisable(GL_DEPTH_TEST);
	
	GLuint finalTexID = _sceneBuffer->textureId();
	
	switch(_mode){
		case FILL:
			_pyramidFiller->process(_sceneBuffer->textureId());
			finalTexID = _showProcInput ? _pyramidFiller->preprocId() : _pyramidFiller->textureId();
			break;
		case INTEGRATE:
			_pyramidIntegrator->process(_sceneBuffer->textureId());
			finalTexID = _showProcInput ? _pyramidIntegrator->preprocId() :_pyramidIntegrator->textureId();
			break;
		case GAUSSBLUR:
			_gaussianBlur->process(_sceneBuffer->textureId());
			finalTexID = _gaussianBlur->textureId();
			break;
		case BOXBLUR:
			_boxBlur->process(_sceneBuffer->textureId());
			finalTexID = _boxBlur->textureId();
			break;
		default:
			break;
	}
	
	// Render the output on screen.
	const glm::vec2 screenSize = Input::manager().size();
	glViewport(0.0f, 0.0f, screenSize[0], screenSize[1]);
	glUseProgram(_passthrough->id());
	ScreenQuad::draw(finalTexID);
	glUseProgram(0);
}

void FilteringRenderer::update(){
	Renderer::update();
	_userCamera.update();
	
	if(ImGui::Begin("Filtering")){
		ImGui::Text("%.2f ms, %.1f fps", ImGui::GetIO().DeltaTime * 1000.0f, 1.0f/ImGui::GetIO().DeltaTime);
		ImGui::Text("Scene res: %ix%i", _sceneBuffer->width(), _sceneBuffer->height());
		ImGui::Separator();
		if(ImGui::InputInt("Vertical res.", &_config.internalVerticalResolution, 50, 200)){
			resize(int(_config.screenResolution[0]), int(_config.screenResolution[1]));
		}
		ImGui::Combo("Mode", (int*)&_mode, "Input\0Fill\0Integrate\0Box blur\0Gaussian blur\0\0");
		ImGui::Separator();
		// Mode specific option
		switch(_mode){
			case GAUSSBLUR:
				if(ImGui::InputInt("Levels", &_blurLevel, 1, 2)){
					_blurLevel = std::min(std::max(1, _blurLevel), 10);
					_gaussianBlur->clean();
					_gaussianBlur = std::unique_ptr<GaussianBlur>(new GaussianBlur(_renderResolution[0], _renderResolution[1], _blurLevel, GL_RGB8));
				}
				break;
			case FILL:
				ImGui::Checkbox("Show colored border", &_showProcInput);
				if(ImGui::InputInt("Pyramid downscale", &_fillDownscale, 1, 2)){
					_fillDownscale = std::max(_fillDownscale, 1);
					_pyramidFiller->clean();
					_pyramidFiller = std::unique_ptr<PoissonFiller>(new PoissonFiller(_renderResolution[0], _renderResolution[1], _fillDownscale));
				}
				break;
			case INTEGRATE:
				ImGui::Checkbox("Show Laplacian", &_showProcInput);
				if(ImGui::InputInt("Pyramid downscale", &_intDownscale, 1, 2)){
					_intDownscale = std::max(_intDownscale, 1);
					_pyramidIntegrator->clean();
					_pyramidIntegrator = std::unique_ptr<LaplacianIntegrator>(new LaplacianIntegrator(_renderResolution[0], _renderResolution[1], _intDownscale));
				}
				break;
			default:
				break;
		}
		
	}
	ImGui::End();
}

void FilteringRenderer::physics(double fullTime, double frameTime){
	_userCamera.physics(frameTime);
}


void FilteringRenderer::clean() const {
	Renderer::clean();
	// Clean objects.
	_sceneBuffer->clean();
	_pyramidFiller->clean();
	_pyramidIntegrator->clean();
	_gaussianBlur->clean();
	_boxBlur->clean();
}


void FilteringRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
	// Resize the framebuffers.
	_sceneBuffer->resize(_renderResolution);
	_pyramidFiller->resize(_renderResolution[0], _renderResolution[1]);
	_pyramidIntegrator->resize(_renderResolution[0], _renderResolution[1]);
	_gaussianBlur->resize(_renderResolution[0], _renderResolution[1]);
	_boxBlur->resize(_renderResolution[0], _renderResolution[1]);
	checkGLError();
}

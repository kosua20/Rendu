#include "IslandApp.hpp"

IslandApp::IslandApp(RenderingConfig & config) : CameraApp(config) {
	_userCamera.projection(config.screenResolution[0] / config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	_userCamera.pose(glm::vec3(-2.234801,3.446842,-6.892219), glm::vec3(-1.869996,2.552125,-5.859552), glm::vec3(0.210734,0.774429,0.596532));
	
	// Framebuffer to store the rendered atmosphere result before tonemapping and upscaling to the window size.
	const glm::vec2 renderRes = _config.renderingResolution();
	_sceneBuffer.reset(new Framebuffer(uint(renderRes[0]), uint(renderRes[1]), {{Layout::RGB32F, Filter::LINEAR_NEAREST, Wrap::CLAMP}, {Layout::DEPTH_COMPONENT32F, Filter::NEAREST, Wrap::CLAMP}}, false));
	// Lookup table.
	_precomputedScattering = Resources::manager().getTexture("scattering-precomputed", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);
	// Atmosphere screen quad.
	_skyProgram = Resources::manager().getProgram("atmosphere_island", "background_infinity", "atmosphere_island");
	_groundProgram = Resources::manager().getProgram("ground_island");
	// Final tonemapping screen quad.
	_tonemap = Resources::manager().getProgram2D("tonemap");

	// Sun direction.
	_lightDirection = glm::normalize(glm::vec3(0.437f, 0.082f, -0.896f));
	_skyMesh = Resources::manager().getMesh("plane", Storage::GPU);

	// Ground.
	_terrain.reset(new Terrain(1024, 4567));

	GLUtilities::setDepthState(true);

	checkGLError();

}

void IslandApp::draw() {

	const glm::mat4 camToWorld = glm::inverse(_userCamera.view());
	const glm::mat4 clipToCam  = glm::inverse(_userCamera.projection());
	const glm::mat4 camToWorldNoT = glm::mat4(glm::mat3(camToWorld));
	const glm::mat4 clipToWorld   = camToWorldNoT * clipToCam;
	const glm::mat4 mvp = _userCamera.projection() * _userCamera.view();

	const glm::vec3 camDir = _userCamera.direction();
	const glm::vec3 frontPos = _userCamera.position() + camDir;
	// Clamp based on the terrain heightmap dimensions in world space.
	const float extent = 0.5f * std::abs(float(_terrain->map().width) * _terrain->texelSize() - 0.5f*_terrain->meshSize());
	const glm::vec3 frontPosClamped = glm::clamp(frontPos, -extent, extent);


	// Draw the atmosphere.
	_sceneBuffer->bind();
	_sceneBuffer->setViewport();
	GLUtilities::setDepthState(true);
	GLUtilities::clearColorAndDepth({0.0f,0.0f,0.0f,1.0f}, 1.0f);
	_prims.begin();

	// Render the ground.
	if(_showTerrain){
	_groundProgram->use();
	_groundProgram->uniform("mvp", mvp);
	_groundProgram->uniform("shift", frontPosClamped);
	_groundProgram->uniform("lightDirection", _lightDirection);

	_groundProgram->uniform("camPos", _userCamera.position());
	_groundProgram->uniform("debugCol", false);

	_groundProgram->uniform("texelSize", _terrain->texelSize());
	_groundProgram->uniform("invMapSize", 1.0f/float(_terrain->map().width));
	_groundProgram->uniform("invGridSize", 1.0f/float(_terrain->gridSize()));

	GLUtilities::bindTexture(_terrain->map(), 0);
	GLUtilities::drawMesh(_terrain->mesh());

	if(_showWire){
		GLUtilities::setPolygonState(PolygonMode::LINE, Faces::ALL);
		GLUtilities::setDepthState(true, DepthEquation::LEQUAL);
		_groundProgram->uniform("debugCol", true);
		GLUtilities::drawMesh(_terrain->mesh());
		GLUtilities::setPolygonState(PolygonMode::FILL, Faces::ALL);
		GLUtilities::setDepthState(true, DepthEquation::LESS);
	}
	}
	_prims.end();

	GLUtilities::setPolygonState(PolygonMode::FILL, Faces::ALL);

	// Render the sky.
	GLUtilities::setDepthState(true, DepthEquation::LEQUAL, false);
	GLUtilities::setBlendState(false);
	_skyProgram->use();

	_skyProgram->uniform("clipToWorld", clipToWorld);
	_skyProgram->uniform("viewPos", _userCamera.position());
	_skyProgram->uniform("lightDirection", _lightDirection);
	GLUtilities::bindTexture(_precomputedScattering, 0);
	GLUtilities::drawMesh(*_skyMesh);
	_sceneBuffer->unbind();

	GLUtilities::setDepthState(true, DepthEquation::LESS, true);
	GLUtilities::setDepthState(false);
	// Tonemapping and final screen.
	GLUtilities::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
	Framebuffer::backbuffer()->bind(Framebuffer::Mode::SRGB);
	_tonemap->use();
	ScreenQuad::draw(_sceneBuffer->texture());
	Framebuffer::backbuffer()->unbind();
}

void IslandApp::update() {
	CameraApp::update();

	_prims.value();
	if(ImGui::Begin("Island")){
		ImGui::Text("%.1f ms, %.1f fps", frameTime() * 1000.0f, frameRate());
		ImGui::Text("Ground: %llu primitives.", _prims.value());
		if(ImGui::DragFloat3("Light dir", &_lightDirection[0], 0.05f, -1.0f, 1.0f)) {
			_lightDirection = glm::normalize(_lightDirection);
		}

		ImGui::Checkbox("Terrain##showcheck", &_showTerrain);
		ImGui::SameLine();
		ImGui::Checkbox("Show wire", &_showWire);
		if(ImGui::CollapsingHeader("Terrain")){
			_terrain->interface();
		}
		if(ImGui::CollapsingHeader("Camera")){
			_userCamera.interface();
		}
	}
	ImGui::End();
}

void IslandApp::resize() {
	_sceneBuffer->resize(_config.renderingResolution());
}

void IslandApp::clean() {
	_sceneBuffer->clean();
	_terrain->clean();
}

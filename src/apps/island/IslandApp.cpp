#include "IslandApp.hpp"

#include "resources/Library.hpp"

IslandApp::IslandApp(RenderingConfig & config) : CameraApp(config), _waves(8, BufferType::UNIFORM, DataUse::STATIC)
{
	_userCamera.projection(config.screenResolution[0] / config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	_userCamera.pose(glm::vec3(-2.234801,3.446842,-6.892219), glm::vec3(-1.869996,2.552125,-5.859552), glm::vec3(0.210734,0.774429,0.596532));
	
	// Framebuffer to store the rendered atmosphere result before tonemapping and upscaling to the window size.
	const glm::vec2 renderRes = _config.renderingResolution();
	const std::vector<Descriptor> descriptors = {{Layout::RGB32F, Filter::LINEAR_NEAREST, Wrap::CLAMP}, {Layout::RGB32F, Filter::LINEAR_NEAREST, Wrap::CLAMP}};
	_sceneBuffer.reset(new Framebuffer(uint(renderRes[0]), uint(renderRes[1]), descriptors, true));
	_waterEffects.reset(new Framebuffer(uint(renderRes[0])/2, uint(renderRes[1])/2, descriptors, false));
	_waterEffectsBlur.reset(new Framebuffer(uint(renderRes[0])/2, uint(renderRes[1])/2, {descriptors[0]}, false));
	// Lookup table.
	_precomputedScattering = Resources::manager().getTexture("scattering-precomputed", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);
	// Atmosphere screen quad.
	_skyProgram = Resources::manager().getProgram("atmosphere_island", "background_infinity", "atmosphere_island");
	_groundProgram = Resources::manager().getProgram("ground_island");
	_oceanProgram = Resources::manager().getProgram("ocean_island", "ocean_island", "ocean_island", "", "ocean_island", "ocean_island");
	_farOceanProgram = Resources::manager().getProgram("far_ocean_island", "far_ocean_island", "ocean_island");
	_waterCopy = Resources::manager().getProgram2D("water_copy");

	// Final tonemapping screen quad.
	_tonemap = Resources::manager().getProgram2D("tonemap");

	// Sun direction.
	_lightDirection = glm::normalize(glm::vec3(0.437f, 0.482f, -0.896f));
	_skyMesh = Resources::manager().getMesh("plane", Storage::GPU);

	// Ground.
	_terrain.reset(new Terrain(1024, 4567));
	_materials = Resources::manager().getTexture("island_material_diff", {Layout::SRGB8, Filter::LINEAR_LINEAR, Wrap::REPEAT}, Storage::GPU);
	_materialNormals = Resources::manager().getTexture("island_material_nor", {Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::REPEAT}, Storage::GPU);

	// Fbm noise used to disturb smooth transitions.
	_transitionNoise.width = _transitionNoise.height = 512;
	_transitionNoise.depth = _transitionNoise.levels = 1,
	_transitionNoise.shape = TextureShape::D2;
	_transitionNoise.images.emplace_back(_transitionNoise.width, _transitionNoise.height, 1);
	PerlinNoise generator;
	generator.generateLayers(_transitionNoise.images[0], 4, 0.5f, 2.0f, 0.01f);
	_transitionNoise.upload({Layout::R32F, Filter::LINEAR, Wrap::REPEAT}, false);

	// Ocean.
	_oceanMesh = Library::generateGrid(64, 1.0f);
	_oceanMesh.upload();
	_farOceanMesh = Library::generateCylinder(64, 128.0f, 256.0f);
	_farOceanMesh.upload();

	GLUtilities::setDepthState(true);

	checkGLError();
	// Tesselation options.
	const float pSize = 128.0f;
	_maxLevelX = std::log2(pSize);
	_maxLevelY = pSize;
	_distanceScale = 1.0f / (float(_sceneBuffer->width()) / 1920.0f) * 4.0f;

	generateWaves();

}

void IslandApp::generateWaves(){
	// Compute Gerstner waves parameters with some variance.
	// Generate a set of low and high frequency waves.
	const float baseALow = 0.025f;
	const float baseAHigh = 0.02f;
	const float angleVar = 0.5f;
	const float basewLow = 2.5f;
	const float basewHigh = 10.0f;

	for(int i = 0; i < 3; ++i){
		auto & wv = _waves[i];
		wv.AQwp[0] = baseALow + Random::Float(-0.01f, 0.01f);
		wv.AQwp[1] = Random::Float(0.1f, 0.5f);
		wv.AQwp[2] = basewLow + Random::Float(-1.5f, 1.5f);
		wv.AQwp[3] = Random::Float(0.2f, 1.5f);
		// Angle.
		wv.DiAngleActive[2] = (2.0f/3.0f) * (i + Random::Float(-angleVar, angleVar)) * glm::pi<float>();
		wv.DiAngleActive[0] = std::cos(wv.DiAngleActive[2]);
		wv.DiAngleActive[1] = std::sin(wv.DiAngleActive[2]);
		// Ensure Q normalization.
		wv.AQwp[1] /= (wv.AQwp[0] * wv.AQwp[2] * 8.0f);
	}
	for(int i = 3; i < 8; ++i){
		auto & wv = _waves[i];
		wv.AQwp[0] = baseAHigh + Random::Float(-0.01f, 0.01f);
		wv.AQwp[1] = Random::Float(0.6f, 1.0f);
		wv.AQwp[2] = basewHigh + Random::Float(-3.0f, 8.0f);
		wv.AQwp[3] = Random::Float(1.0f, 3.0f);
		// Angle.
		wv.DiAngleActive[2] = ((2.0f/5.0f) * (i + Random::Float(-angleVar, angleVar)) - 1.0f) * glm::pi<float>();
		wv.DiAngleActive[0] = std::cos(wv.DiAngleActive[2]);
		wv.DiAngleActive[1] = std::sin(wv.DiAngleActive[2]);
		// Ensure Q normalization.
		wv.AQwp[1] /= (wv.AQwp[0] * wv.AQwp[2] * 8.0f);
	}
	_waves.upload();
}

void IslandApp::draw() {

	const glm::mat4 camToWorld = glm::inverse(_userCamera.view());
	const glm::mat4 clipToCam  = glm::inverse(_userCamera.projection());
	const glm::mat4 camToWorldNoT = glm::mat4(glm::mat3(camToWorld));
	const glm::mat4 clipToWorld   = camToWorldNoT * clipToCam;
	const glm::mat4 mvp = _userCamera.projection() * _userCamera.view();
	const glm::vec3 camDir = _userCamera.direction();
	const glm::vec3 & camPos = _userCamera.position();

	_sceneBuffer->bind();
	_sceneBuffer->setViewport();
	_sceneBuffer->clear(glm::vec4(10000.0f), 1.0f);
	GLUtilities::setDepthState(true);
	GLUtilities::setBlendState(false);
	GLUtilities::clearColor({0.0f,0.0f,0.0f,1.0f});
	_prims.begin();

	// Render the ground.
	if(_showTerrain){

		const glm::vec3 frontPos = camPos + camDir;
		// Clamp based on the terrain heightmap dimensions in world space.
		const float extent = 0.5f * std::abs(float(_terrain->map().width) * _terrain->texelSize() - 0.5f*_terrain->meshSize());
		const glm::vec3 frontPosClamped = glm::clamp(frontPos, -extent, extent);

		_groundProgram->use();
		_groundProgram->uniform("mvp", mvp);
		_groundProgram->uniform("shift", frontPosClamped);
		_groundProgram->uniform("lightDirection", _lightDirection);
		_groundProgram->uniform("camDir", camDir);
		_groundProgram->uniform("camPos", camPos);
		_groundProgram->uniform("debugCol", false);
		_groundProgram->uniform("texelSize", _terrain->texelSize());
		_groundProgram->uniform("invMapSize", 1.0f/float(_terrain->map().width));
		_groundProgram->uniform("invGridSize", 1.0f/float(_terrain->gridSize()));

		GLUtilities::bindTexture(_terrain->map(), 0);
		GLUtilities::bindTexture(_transitionNoise, 1);
		GLUtilities::bindTexture(_materials, 2);
		GLUtilities::bindTexture(_materialNormals, 3);
		GLUtilities::drawMesh(_terrain->mesh());

		// Debug view.
		if(_showWire){
			GLUtilities::setPolygonState(PolygonMode::LINE, Faces::ALL);
			GLUtilities::setDepthState(true, DepthEquation::LEQUAL, true);
			_groundProgram->uniform("debugCol", true);
			GLUtilities::drawMesh(_terrain->mesh());
			GLUtilities::setPolygonState(PolygonMode::FILL, Faces::ALL);
			GLUtilities::setDepthState(true, DepthEquation::LESS, true);
		}
	}

	// Render the ocean.
	if(_showOcean){
		// Start by copying the visible terrain.
		_waterEffects->bind();
		GLUtilities::setDepthState(false);
		_waterEffects->setViewport();
		_waterCopy->use();
		GLUtilities::bindTexture(_sceneBuffer->texture(0), 0);
		GLUtilities::bindTexture(_sceneBuffer->texture(1), 1);
		ScreenQuad::draw();
		GLUtilities::setDepthState(true);
		_waterEffects->unbind();

		_blur.process(_waterEffects->texture(0), *_waterEffectsBlur);

		_sceneBuffer->bind();
		_sceneBuffer->setViewport();
		_oceanProgram->use();
		_oceanProgram->uniform("mvp", mvp);
		_oceanProgram->uniform("shift", camPos );
		_oceanProgram->uniform("maxLevelX", _maxLevelX);
		_oceanProgram->uniform("maxLevelY", _maxLevelY);
		_oceanProgram->uniform("distanceScale", _distanceScale);
		_oceanProgram->uniform("lightDirection", _lightDirection);
		_oceanProgram->uniform("debugCol", false);
		_oceanProgram->uniform("lodPos", camPos);
		_oceanProgram->uniform("camDir", camDir);
		_oceanProgram->uniform("camPos", camPos);
		_oceanProgram->uniform("raycast", false);
		_oceanProgram->uniform("time", float(timeElapsed()));
		_oceanProgram->uniform("texelSize", _terrain->texelSize());
		_oceanProgram->uniform("invMapSize", 1.0f/float(_terrain->map().width));
		_oceanProgram->uniform("invGridSize", 1.0f/float(_terrain->gridSize()));

		GLUtilities::bindBuffer(_waves, 0);
		GLUtilities::bindTexture(_terrain->map(), 0);
		GLUtilities::bindTexture(_waterEffects->texture(0), 1);
		GLUtilities::bindTexture(_waterEffects->texture(1), 2);
		GLUtilities::bindTexture(_waterEffectsBlur->texture(0), 3);
		GLUtilities::drawTesselatedMesh(_oceanMesh, 4);

		// Debug view.
		if(_showWire){
			GLUtilities::setPolygonState(PolygonMode::LINE, Faces::ALL);
			GLUtilities::setDepthState(true, DepthEquation::LEQUAL, true);
			_oceanProgram->uniform("debugCol", true);
			GLUtilities::drawTesselatedMesh(_oceanMesh, 4);
			GLUtilities::setPolygonState(PolygonMode::FILL, Faces::ALL);
			GLUtilities::setDepthState(true, DepthEquation::LESS, true);
		}

		// Far ocean, using a cylinder as support to cast rays intersecting the ocean plane.
		_farOceanProgram->use();
		_farOceanProgram->uniform("mvp", mvp);
		_farOceanProgram->uniform("camPos", camPos);
		_farOceanProgram->uniform("lightDirection", _lightDirection);
		_farOceanProgram->uniform("debugCol", false);
		_farOceanProgram->uniform("time", float(timeElapsed()));
		_farOceanProgram->uniform("texelSize", _terrain->texelSize());
		_farOceanProgram->uniform("raycast", true);
		_farOceanProgram->uniform("invMapSize", 1.0f/float(_terrain->map().width));
		_farOceanProgram->uniform("invGridSize", 1.0f/float(_terrain->gridSize()));

		GLUtilities::bindBuffer(_waves, 0);
		GLUtilities::bindTexture(_terrain->map(), 0);
		GLUtilities::bindTexture(_waterEffects->texture(0), 1);
		GLUtilities::bindTexture(_waterEffects->texture(1), 2);
		GLUtilities::bindTexture(_waterEffectsBlur->texture(0), 3);
		GLUtilities::drawMesh(_farOceanMesh);

		// Debug view.
		if(_showWire){
			GLUtilities::setPolygonState(PolygonMode::LINE, Faces::ALL);
			GLUtilities::setDepthState(true, DepthEquation::LEQUAL, true);
			_farOceanProgram->uniform("debugCol", true);
			GLUtilities::drawMesh(_farOceanMesh);
			GLUtilities::setPolygonState(PolygonMode::FILL, Faces::ALL);
			GLUtilities::setDepthState(true, DepthEquation::LESS, true);
		}

	}
	_prims.end();

	// Render the sky.
	GLUtilities::setDepthState(true, DepthEquation::LEQUAL, false);

	_skyProgram->use();
	_skyProgram->uniform("clipToWorld", clipToWorld);
	_skyProgram->uniform("viewPos", camPos);
	_skyProgram->uniform("lightDirection", _lightDirection);
	GLUtilities::bindTexture(_precomputedScattering, 0);
	GLUtilities::drawMesh(*_skyMesh);

	_sceneBuffer->unbind();

	GLUtilities::setDepthState(false, DepthEquation::LESS, true);
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
		ImGui::Checkbox("Ocean##showcheck", &_showOcean);
		ImGui::Checkbox("Show wire", &_showWire);
		if(ImGui::CollapsingHeader("Tessellation")){
			ImGui::DragFloat("maxLevelX", &_maxLevelX);
			ImGui::DragFloat("maxLevelY", &_maxLevelY);
			ImGui::DragFloat("distanceScale", &_distanceScale);

		}

		if(ImGui::CollapsingHeader("Terrain")){
			_terrain->interface();
		}

		if(ImGui::CollapsingHeader("Ocean")){
			bool dirtyWaves = false;
			for(int i = 0; i < 8; ++i){
				auto & wave = _waves[i];
				const std::string name = "Wave " + std::to_string(i);
				if(ImGui::TreeNode(name.c_str())){
					bool active = wave.DiAngleActive[3] > 0.001f;
					if(ImGui::Checkbox("Enabled", &active)){
						wave.DiAngleActive[3] = active ? 1.0f : 0.0f;
						dirtyWaves = true;
					}
					if(active){
						dirtyWaves = ImGui::SliderFloat("Ai", &wave.AQwp[0], 0.0f, 1.0f) || dirtyWaves;
						dirtyWaves = ImGui::SliderFloat("Qi", &wave.AQwp[1], 0.0f, 1.0f) || dirtyWaves;
						dirtyWaves = ImGui::SliderFloat("wi", &wave.AQwp[2], 0.0f, 1.0f) || dirtyWaves;
						dirtyWaves = ImGui::SliderFloat("phi", &wave.AQwp[3], 0.0f, glm::pi<float>()) || dirtyWaves;
						if(ImGui::SliderFloat("Angle", &wave.DiAngleActive[2], 0.0f, glm::two_pi<float>())){
							dirtyWaves = true;
							wave.DiAngleActive[0] = std::cos(wave.DiAngleActive[2]);
							wave.DiAngleActive[1] = std::sin(wave.DiAngleActive[2]);
						}
					}
					ImGui::TreePop();
				}
				if(i == 2){
					ImGui::Separator();
				}
			}
			if(dirtyWaves){
				_waves.upload();
			}

		}

		if(ImGui::CollapsingHeader("Camera")){
			_userCamera.interface();
		}
	}
	ImGui::End();
}

void IslandApp::resize() {
	_sceneBuffer->resize(_config.renderingResolution());
	_waterEffects->resize(_config.renderingResolution()/2.0f);
	_waterEffectsBlur->resize(_config.renderingResolution()/2.0f);
}

void IslandApp::clean() {
	_sceneBuffer->clean();
	_waterEffects->clean();
	_waterEffectsBlur->clean();
	_blur.clean();
	_terrain->clean();
}

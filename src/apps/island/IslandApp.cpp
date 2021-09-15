#include "IslandApp.hpp"

#include "resources/Library.hpp"

IslandApp::IslandApp(RenderingConfig & config) : CameraApp(config),
	_oceanMesh("Ocean"), _farOceanMesh("Far ocean"),
	_surfaceNoise("surface noise"), _glitterNoise("glitter noise"),
	 _waves(8, DataUse::FRAME)
{
	_userCamera.projection(config.screenResolution[0] / config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	_userCamera.pose(glm::vec3(-2.234801,3.446842,-6.892219), glm::vec3(-1.869996,2.552125,-5.859552), glm::vec3(0.210734,0.774429,0.596532));
	
	// Framebuffer to store the rendered atmosphere result before tonemapping and upscaling to the window size.
	const glm::vec2 renderRes = _config.renderingResolution();
	const std::vector<Layout> formats = {Layout::RGBA32F, Layout::RGBA32F};
	const std::vector<Layout> formatsAndDepth = {Layout::RGBA32F, Layout::RGBA32F, Layout::DEPTH_COMPONENT32F};
	_sceneBuffer.reset(new Framebuffer(uint(renderRes[0]), uint(renderRes[1]), formatsAndDepth, "Scene"));
	_waterPos.reset(new Framebuffer(uint(renderRes[0]), uint(renderRes[1]), formats[1], "Water position"));
	_waterEffectsHalf.reset(new Framebuffer(uint(renderRes[0])/2, uint(renderRes[1])/2, formats[0], "Water effect half"));
	_waterEffectsBlur.reset(new Framebuffer(uint(renderRes[0])/2, uint(renderRes[1])/2, formats[0], "Water effect blur"));
	_environment.reset(new Framebuffer(TextureShape::Cube, 512, 512, 6, 1, {Layout::RGBA16F}, "Environment"));

	// Lookup table.
	_precomputedScattering = Resources::manager().getTexture("scattering-precomputed", Layout::RGBA16F, Storage::GPU);
	// Atmosphere screen quad.
	_skyProgram = Resources::manager().getProgram("atmosphere_island", "background_infinity", "atmosphere_island");
	_groundProgram = Resources::manager().getProgram("ground_island");
	_oceanProgram = Resources::manager().getProgram("ocean_island", "ocean_island", "ocean_island", "ocean_island", "ocean_island");
	_farOceanProgram = Resources::manager().getProgram("far_ocean_island", "far_ocean_island", "ocean_island");
	_waterCopy = Resources::manager().getProgram2D("water_copy");
	_underwaterProgram = Resources::manager().getProgram2D("ocean_underwater");
	// Final tonemapping screen quad.
	_tonemap = Resources::manager().getProgram2D("tonemap");

	// Sun direction.
	_lightDirection = glm::normalize(glm::vec3(0.660619f, 0.660619f, -0.661131f));
	_skyMesh = Resources::manager().getMesh("plane", Storage::GPU);

	// Ground.
	_terrain.reset(new Terrain(1024, 4567));

	// Sand normal maps.
	_sandMapSteep = Resources::manager().getTexture("sand_normal_steep", Layout::RGBA8, Storage::GPU);
	_sandMapFlat = Resources::manager().getTexture("sand_normal_flat", Layout::RGBA8, Storage::GPU);

	// High detail noise.
	_surfaceNoise.width = _surfaceNoise.height = 512;
	_surfaceNoise.depth = _surfaceNoise.levels = 1,
	_surfaceNoise.shape = TextureShape::D2;
	_surfaceNoise.images.emplace_back(_surfaceNoise.width, _surfaceNoise.height, 4);
	for(uint y = 0; y < _surfaceNoise.height; ++y){
		for(uint x = 0; x < _surfaceNoise.width; ++x){
			glm::vec3 dir = Random::sampleSphere();
			dir[2] = std::abs(dir[2]);
			const float wd = Random::Float();
			_surfaceNoise.images[0].rgba(x,y) = glm::vec4(dir, wd);
		}
	}
	_surfaceNoise.upload(Layout::RGBA32F, true);
	// Additional glitter noise with custom mipmaps.
	_glitterNoise.width = _glitterNoise.height = 512;
	_glitterNoise.depth = 1;
	_glitterNoise.levels = _glitterNoise.getMaxMipLevel()+1;
	_glitterNoise.shape = TextureShape::D2;
	_glitterNoise.images.emplace_back(_glitterNoise.width, _glitterNoise.height, 4);
	for(uint y = 0; y < _glitterNoise.height; ++y){
		for(uint x = 0; x < _glitterNoise.width; ++x){
			glm::vec3 dir = Random::sampleSphere();
			_glitterNoise.images[0].rgba(x,y) = glm::vec4(dir, 0.0f);
		}
	}
	for(uint lid = 1; lid < _glitterNoise.levels; ++lid){
		const uint tw = _glitterNoise.width / (1 << lid);
		const uint th = _glitterNoise.height / (1 << lid);
		_glitterNoise.images.emplace_back(tw, th, 4);
		for(uint y = 0; y < th; ++y){
			for(uint x = 0; x < tw; ++x){
				_glitterNoise.images[lid].rgba(x,y) = _glitterNoise.images[lid-1].rgba(2*x,2*y);
			}
		}
	}
	_glitterNoise.upload(Layout::RGBA32F, false);

	// Ocean.
	_oceanMesh = Library::generateGrid(_gridOceanRes, 1.0f);
	_oceanMesh.upload();
	_farOceanMesh = Library::generateCylinder(64, 128.0f, 256.0f);
	_farOceanMesh.upload();
	_absorbScatterOcean = Resources::manager().getTexture("absorbscatterwater", Layout::SRGB8_ALPHA8, Storage::GPU);
	_caustics = Resources::manager().getTexture("caustics", Layout::R8, Storage::GPU);
	_waveNormals = Resources::manager().getTexture("wave_normals", Layout::RGBA8, Storage::GPU);
	_foam = Resources::manager().getTexture("foam", Layout::SRGB8_ALPHA8, Storage::GPU);
	_brdfLUT = Resources::manager().getTexture("brdf-precomputed", Layout::RG16F, Storage::GPU);

	// Tesselation options.
	const float pSize = 128.0f;
	_maxLevelX = std::log2(pSize);
	_maxLevelY = pSize;
	_distanceScale = 1.0f / (float(_sceneBuffer->width()) / 1920.0f) * 6.0f;

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
		wv.AQwp[1] = 3.0f*Random::Float(0.1f, 0.5f);
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
		wv.AQwp[1] = 3.0f*Random::Float(0.6f, 1.0f);
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
	const glm::vec2 invRenderSize = 1.0f / glm::vec2(_sceneBuffer->width(), _sceneBuffer->height());
	const float time = _stopTime ? 0.1f : float(timeElapsed());
	// If needed, update the skybox.
	if(_shouldUpdateSky){
		GPU::setDepthState(false);
		GPU::setBlendState(false);
		GPU::setCullState(false, Faces::BACK);
		_environment->setViewport();

		_skyProgram->use();
		_skyProgram->uniform("viewPos", glm::vec3(0.0f));
		_skyProgram->uniform("lightDirection", _lightDirection);
		_skyProgram->texture(_precomputedScattering, 0);

		for(uint lid = 0; lid < 6; ++lid){
			_environment->bind(lid, 0, Framebuffer::Operation::DONTCARE, Framebuffer::Operation::DONTCARE, Framebuffer::Operation::DONTCARE);
			const glm::mat4 clipToWorldFace  = glm::inverse(Library::boxVPs[lid]);
			_skyProgram->uniform("clipToWorld", clipToWorldFace);
			GPU::drawMesh(*_skyMesh);
		}

		_terrain->generateShadowMap(_lightDirection);
		_shouldUpdateSky = false;
	}

	_sceneBuffer->bind(glm::vec4(0.0f), 1.0f);
	_sceneBuffer->setViewport();
	
	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

	// Render the ground.
	if(_showTerrain){

		const glm::vec3 frontPos = camPos + camDir;
		// Clamp based on the terrain heightmap dimensions in world space.
		const float extent = 0.25f * std::abs(float(_terrain->map().width) * _terrain->texelSize() - 0.5f*_terrain->meshSize());
		glm::vec3 frontPosClamped = glm::clamp(frontPos, -extent, extent);
		frontPosClamped[1] = 0.0f;

		// Compensate for grid translation.
		const Frustum camFrustum(mvp);

		_groundProgram->use();
		_groundProgram->uniform("mvp", mvp);
		_groundProgram->uniform("shift", frontPosClamped);
		_groundProgram->uniform("lightDirection", _lightDirection);
		_groundProgram->uniform("camDir", camDir);
		_groundProgram->uniform("camPos", camPos);
		_groundProgram->uniform("texelSize", _terrain->texelSize());
		_groundProgram->uniform("invMapSize", 1.0f/float(_terrain->map().width));
		_groundProgram->uniform("invGridSize", 1.0f/float(_terrain->gridSize()));

		_groundProgram->texture(_terrain->map(), 0);
		_groundProgram->texture(_terrain->shadowMap(), 1);
		_groundProgram->texture(_surfaceNoise, 2);
		_groundProgram->texture(_glitterNoise, 3);
		_groundProgram->texture(_sandMapSteep, 4);
		_groundProgram->texture(_sandMapFlat, 5);


		for(const Terrain::Cell & cell : _terrain->cells()){
			// Compute equivalent of vertex shader vertex transformation.
			const float levelSize = std::exp2(float(cell.level)) * _terrain->texelSize();
			glm::vec3 mini = _terrain->texelSize() * cell.mesh.bbox.minis + glm::round(frontPosClamped/levelSize) * levelSize;
			glm::vec3 maxi = _terrain->texelSize() * cell.mesh.bbox.maxis + glm::round(frontPosClamped/levelSize) * levelSize;
			mini[1] = -5.0f; maxi[1] = 5.0f;
			BoundingBox box(mini, maxi);
			if(!camFrustum.intersects(box)){
				continue;
			}
			_groundProgram->uniform("debugCol", false);
			GPU::drawMesh(cell.mesh);

			// Debug view.
			if(_showWire){
				GPU::setPolygonState(PolygonMode::LINE);
				GPU::setDepthState(true, TestFunction::LEQUAL, true);
				_groundProgram->uniform("debugCol", true);
				GPU::drawMesh(cell.mesh);
				GPU::setPolygonState(PolygonMode::FILL);
				GPU::setDepthState(true, TestFunction::LESS, true);
			}
		}
	}
	
	// Render the sky.
	if(_showSky){
		GPU::setDepthState(true, TestFunction::LEQUAL, false);
		GPU::setCullState(false, Faces::BACK);
		GPU::setBlendState(false);

		_skyProgram->use();
		_skyProgram->uniform("clipToWorld", clipToWorld);
		_skyProgram->uniform("viewPos", camPos);
		_skyProgram->uniform("lightDirection", _lightDirection);
		_skyProgram->texture(_precomputedScattering, 0);
		GPU::drawMesh(*_skyMesh);
	}

	// Render the ocean.

	if(_showOcean){
		const bool isUnderwater = camPos.y < 0.00f;

		// Start by copying the visible terrain info.
		// Blit full res position map.
		GPU::blit(*_sceneBuffer->texture(1), *_waterPos, Filter::NEAREST);

		if(isUnderwater){
			// Blit color as-is if underwater (blur will happen later)
			GPU::blit(*_sceneBuffer->texture(0), *_waterEffectsHalf, Filter::LINEAR);
		} else {
			// Else copy, downscale, apply caustics and blur.
			GPU::setDepthState(false);
			GPU::setCullState(true, Faces::BACK);
			GPU::setBlendState(false);

			_waterEffectsHalf->bind(Framebuffer::Operation::DONTCARE);
			_waterEffectsHalf->setViewport();
			_waterCopy->use();
			_waterCopy->texture(_sceneBuffer->texture(0), 0);
			_waterCopy->texture(_sceneBuffer->texture(1), 1);
			_waterCopy->texture(_caustics, 2);
			_waterCopy->texture(_waveNormals, 3);
			_waterCopy->uniform("time", time);
			ScreenQuad::draw();

			_blur.process(_waterEffectsHalf->texture(0), *_waterEffectsBlur);
		}

		// Render the ocean waves.
		_sceneBuffer->bind(Framebuffer::Operation::LOAD, Framebuffer::Operation::LOAD, Framebuffer::Operation::DONTCARE);
		_sceneBuffer->setViewport();
		GPU::setDepthState(true, TestFunction::LESS, true);
		GPU::setBlendState(false);
		GPU::setCullState(true, isUnderwater ? Faces::FRONT : Faces::BACK);

		_oceanProgram->use();
		_oceanProgram->uniform("mvp", mvp);
		_oceanProgram->uniform("shift", glm::round(camPos));
		_oceanProgram->uniform("maxLevelX", _maxLevelX);
		_oceanProgram->uniform("maxLevelY", _maxLevelY);
		_oceanProgram->uniform("distanceScale", _distanceScale);
		_oceanProgram->uniform("underwater", isUnderwater);
		_oceanProgram->uniform("debugCol", false);
		_oceanProgram->uniform("camDir", camDir);
		_oceanProgram->uniform("camPos", camPos);
		_oceanProgram->uniform("distantProxy", false);
		_oceanProgram->uniform("time", time);
		_oceanProgram->uniform("invTargetSize", invRenderSize);
		_oceanProgram->uniform("invTexelSize", 1.0f/_terrain->texelSize());
		_oceanProgram->uniform("invMapSize", 1.0f/float(_terrain->map().width));
		_oceanProgram->uniform("useTerrain", _showTerrain);

		_oceanProgram->buffer(_waves, 0);
		_oceanProgram->texture(_foam, 0);
		_oceanProgram->texture(_waterEffectsHalf->texture(0), 1);
		_oceanProgram->texture(_waterPos->texture(0), 2);
		_oceanProgram->texture(_waterEffectsBlur->texture(0), 3);
		_oceanProgram->texture(_absorbScatterOcean, 4);
		_oceanProgram->texture(_waveNormals, 5);
		_oceanProgram->texture(_environment->texture(), 6);
		_oceanProgram->texture(_brdfLUT, 7);
		_oceanProgram->texture(_terrain->shadowMap(), 8);
		GPU::drawTesselatedMesh(_oceanMesh, 4);

		// Debug view.
		if(_showWire){
			GPU::setPolygonState(PolygonMode::LINE);
			GPU::setDepthState(true, TestFunction::LEQUAL, true);
			_oceanProgram->uniform("debugCol", true);
			GPU::drawTesselatedMesh(_oceanMesh, 4);
			GPU::setPolygonState(PolygonMode::FILL);
			// GPU::setDepthState(true, TestFunction::LESS, true);
		}

		if(isUnderwater){
			// We do the low-res copy and blur now, because we need the blurred ocean surface to be visible.
			GPU::setCullState(true, Faces::BACK);
			GPU::setDepthState(false);
			GPU::setBlendState(false);

			_waterEffectsHalf->bind(Framebuffer::Operation::LOAD);
			_waterEffectsHalf->setViewport();
			_waterCopy->use();
			_waterCopy->texture(_sceneBuffer->texture(0), 0);
			_waterCopy->texture(_sceneBuffer->texture(1), 1);
			_waterCopy->texture(_caustics, 2);
			_waterCopy->texture(_waveNormals, 3);
			_waterCopy->uniform("time", time);
			ScreenQuad::draw();

			_blur.process(_waterEffectsHalf->texture(0), *_waterEffectsBlur);

			// Blit full res position map.
			GPU::blit(*_sceneBuffer->texture(1), *_waterPos, Filter::NEAREST);

			// Render full screen effect.
			_sceneBuffer->bind(Framebuffer::Operation::LOAD);
			_sceneBuffer->setViewport();
			GPU::setCullState(true, Faces::BACK);
			GPU::setDepthState(false);
			GPU::setBlendState(false);

			_underwaterProgram->use();
			_underwaterProgram->uniform("mvp", mvp);
			_underwaterProgram->uniform("camDir", camDir);
			_underwaterProgram->uniform("camPos", camPos);
			_underwaterProgram->uniform("time", time);
			_underwaterProgram->uniform("invTargetSize", invRenderSize);

			_underwaterProgram->buffer(_waves, 0);
			_underwaterProgram->texture(_foam, 0);
			_underwaterProgram->texture(_waterEffectsHalf->texture(0), 1);
			_underwaterProgram->texture(_waterPos->texture(0), 2);
			_underwaterProgram->texture(_waterEffectsBlur->texture(0), 3);
			_underwaterProgram->texture(_absorbScatterOcean, 4);
			_underwaterProgram->texture(_waveNormals, 5);
			_underwaterProgram->texture(_environment->texture(), 6);
			ScreenQuad::draw();
			GPU::setDepthState(true, TestFunction::LESS, true);

		} else {
			// Far ocean, using a cylinder as support to cast rays intersecting the ocean plane.
			GPU::setDepthState(true, TestFunction::ALWAYS, true);
			GPU::setCullState(true, Faces::BACK);
			GPU::setBlendState(false);

			_farOceanProgram->use();
			_farOceanProgram->uniform("mvp", mvp);
			_farOceanProgram->uniform("camPos", camPos);
			_farOceanProgram->uniform("debugCol", false);
			_farOceanProgram->uniform("time", time);
			_farOceanProgram->uniform("distantProxy", true);
			_farOceanProgram->uniform("waterGridHalf", float(_gridOceanRes-2)*0.5f);
			_farOceanProgram->uniform("groundGridHalf", _terrain->meshSize()*0.5f);
			_farOceanProgram->uniform("invTargetSize", invRenderSize);
			_farOceanProgram->uniform("underwater", isUnderwater);
			_farOceanProgram->uniform("invTexelSize", 1.0f/_terrain->texelSize());
			_farOceanProgram->uniform("invMapSize", 1.0f/float(_terrain->map().width));
			_farOceanProgram->uniform("useTerrain", _showTerrain);

			_farOceanProgram->buffer(_waves, 0);
			_farOceanProgram->texture(_foam, 0);
			_farOceanProgram->texture(_waterEffectsHalf->texture(0), 1);
			_farOceanProgram->texture(_waterPos->texture(0), 2);
			_farOceanProgram->texture(_waterEffectsBlur->texture(0), 3);
			_farOceanProgram->texture(_absorbScatterOcean, 4);
			_farOceanProgram->texture(_waveNormals, 5);
			_farOceanProgram->texture(_environment->texture(), 6);
			_farOceanProgram->texture(_brdfLUT, 7);
			_farOceanProgram->texture(_terrain->shadowMap(), 8);
			GPU::drawMesh(_farOceanMesh);

			// Debug view.
			if(_showWire){
				GPU::setPolygonState(PolygonMode::LINE);
				GPU::setDepthState(true, TestFunction::LEQUAL, true);
				_farOceanProgram->uniform("debugCol", true);
				GPU::drawMesh(_farOceanMesh);
				GPU::setPolygonState(PolygonMode::FILL);
			}
		}

	}

	// Tonemapping and final screen.
	GPU::setDepthState(false, TestFunction::LESS, true);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);
	
	GPU::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
	Framebuffer::backbuffer()->bind(Framebuffer::Operation::DONTCARE, Framebuffer::Operation::DONTCARE, Framebuffer::Operation::DONTCARE);
	_tonemap->use();
	_tonemap->uniform("customExposure", 1.0f);
	_tonemap->uniform("apply", true);
	_tonemap->texture(_sceneBuffer->texture(), 0);
	ScreenQuad::draw();
}

void IslandApp::update() {
	CameraApp::update();

	if(ImGui::Begin("Island")){
		ImGui::Text("%.1f ms, %.1f fps", frameTime() * 1000.0f, frameRate());
		ImGui::Text("Rendering res.: %ux%u", _sceneBuffer->width(), _sceneBuffer->height());

		// Light parameters.
		ImGui::PushItemWidth(120);
		if(ImGui::DragFloat("Azimuth", &_lightAzimuth, 0.1f, 0.0f, 360.0f, "%.1f°")){
			_lightAzimuth = glm::clamp(_lightAzimuth, 0.0f, 360.0f);
			_shouldUpdateSky = true;
		}
		ImGui::SameLine();
		if(ImGui::DragFloat("Elevation", &_lightElevation, 0.1f, -15.0f, 90.0f, "%.1f°")){
			_lightElevation = glm::clamp(_lightElevation, -15.0f, 90.0f);
			_shouldUpdateSky = true;
		}
		if(_shouldUpdateSky){
			const float elevRad = _lightElevation / 180 * glm::pi<float>();
			const float azimRad = _lightAzimuth / 180 * glm::pi<float>();
			_lightDirection = glm::vec3(std::cos(azimRad) * std::cos(elevRad), std::sin(elevRad), std::sin(azimRad) * std::cos(elevRad));
		}
		ImGui::PopItemWidth();

		ImGui::Checkbox("Terrain##showcheck", &_showTerrain);
		ImGui::SameLine();
		ImGui::Checkbox("Ocean##showcheck", &_showOcean);
		ImGui::SameLine();
		ImGui::Checkbox("Sky##showcheck", &_showSky);
		ImGui::Checkbox("Show wire", &_showWire); ImGui::SameLine();
		ImGui::Checkbox("Stop time", &_stopTime);

		if(ImGui::CollapsingHeader("Tessellation")){
			ImGui::DragFloat("maxLevelX", &_maxLevelX);
			ImGui::DragFloat("maxLevelY", &_maxLevelY);
			ImGui::DragFloat("distanceScale", &_distanceScale);

		}

		if(ImGui::CollapsingHeader("Terrain")){
			const bool dirtyShadowMap = _terrain->interface();
			if(dirtyShadowMap){
				_terrain->generateShadowMap(_lightDirection);
			}
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
	_waterPos->resize(_config.renderingResolution());
	_waterEffectsHalf->resize(_config.renderingResolution()/2.0f);
	_waterEffectsBlur->resize(_config.renderingResolution()/2.0f);
}

IslandApp::~IslandApp() {
	_oceanMesh.clean();
	_farOceanMesh.clean();
}

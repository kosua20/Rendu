
#include "input/Input.hpp"
#include "raycaster/Intersection.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GLUtilities.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "generation/Random.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

#include "AtmosphereApp.hpp"

AtmosphereApp::AtmosphereApp(RenderingConfig & config) : CameraApp(config), _scattering("Scattering LUT") {
	_userCamera.projection(config.screenResolution[0] / config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	// Framebuffer to store the rendered atmosphere result before tonemapping and upscaling to the window size.
	const glm::vec2 renderRes = _config.renderingResolution();
	_atmosphereBuffer.reset(new Framebuffer(uint(renderRes[0]), uint(renderRes[1]), {Layout::RGB32F, Filter::LINEAR_NEAREST, Wrap::CLAMP}, false, "Atmosphere"));
	// Atmosphere screen quad.
	_atmosphere = Resources::manager().getProgram2D("atmosphere_params");
	// Final tonemapping screen quad.
	_tonemap = Resources::manager().getProgram2D("tonemap");
	// Sun direction.
	_lightDirection = glm::normalize(glm::vec3(0.337f, 0.174f, -0.925f));
	// Populate lookup table.
	updateSky();

	GLUtilities::setDepthState(true);
	checkGLError();
}

void AtmosphereApp::draw() {
	// Render.
	const glm::mat4 camToWorld = glm::inverse(_userCamera.view());
	const glm::mat4 clipToCam  = glm::inverse(_userCamera.projection());

	// Draw the atmosphere.
	GLUtilities::setDepthState(false);
	_atmosphereBuffer->bind();
	_atmosphereBuffer->setViewport();
	GLUtilities::clearColor({0.0f, 0.0f, 0.0f, 1.0f});

	_atmosphere->use();
	const glm::mat4 camToWorldNoT = glm::mat4(glm::mat3(camToWorld));
	const glm::mat4 clipToWorld	  = camToWorldNoT * clipToCam;
	_atmosphere->uniform("clipToWorld", clipToWorld);
	_atmosphere->uniform("viewPos", _userCamera.position());
	_atmosphere->uniform("lightDirection", _lightDirection);
	_atmosphere->uniform("altitude", _altitude);
	// Send the atmosphere parameters.
	_atmosphere->uniform("atmoParams.sunColor", _atmoParams.sunColor);
	_atmosphere->uniform("atmoParams.kRayleigh", _atmoParams.kRayleigh);
	_atmosphere->uniform("atmoParams.groundRadius", _atmoParams.groundRadius);
	_atmosphere->uniform("atmoParams.topRadius", _atmoParams.topRadius);
	_atmosphere->uniform("atmoParams.sunIntensity", _atmoParams.sunIntensity);
	_atmosphere->uniform("atmoParams.kMie", _atmoParams.kMie);
	_atmosphere->uniform("atmoParams.heightRayleigh", _atmoParams.heightRayleigh);
	_atmosphere->uniform("atmoParams.heightMie", _atmoParams.heightMie);
	_atmosphere->uniform("atmoParams.gMie", _atmoParams.gMie);
	_atmosphere->uniform("atmoParams.sunAngularRadius", _atmoParams.sunRadius);
	_atmosphere->uniform("atmoParams.sunAngularRadiusCos", _atmoParams.sunRadiusCos);

	ScreenQuad::draw(_scattering);
	_atmosphereBuffer->unbind();

	// Tonemapping and final screen.
	GLUtilities::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
	Framebuffer::backbuffer()->bind(Framebuffer::Mode::SRGB);
	_tonemap->use();
	ScreenQuad::draw(_atmosphereBuffer->texture());
	Framebuffer::backbuffer()->unbind();
}

void AtmosphereApp::update() {
	CameraApp::update();

	if(ImGui::Begin("Atmosphere")) {
		ImGui::Text("%.1f ms, %.1f fps", frameTime() * 1000.0f, frameRate());

		// Sun parameters.
		ImGui::PushItemWidth(120);
		bool shouldUpdateSky = false;
		if(ImGui::DragFloat("Azimuth", &_lightAzimuth, 0.1f, 0.0f, 360.0f, "%.1f°")) {
			_lightAzimuth	= glm::clamp(_lightAzimuth, 0.0f, 360.0f);
			shouldUpdateSky = true;
		}
		ImGui::SameLine();
		if(ImGui::DragFloat("Elevation", &_lightElevation, 0.1f, -15.0f, 90.0f, "%.1f°")) {
			_lightElevation = glm::clamp(_lightElevation, -15.0f, 90.0f);
			shouldUpdateSky = true;
		}
		ImGui::PopItemWidth();
		
		if(shouldUpdateSky) {
			const float elevRad = _lightElevation / 180 * glm::pi<float>();
			const float azimRad = _lightAzimuth / 180 * glm::pi<float>();
			_lightDirection		= glm::vec3(std::cos(azimRad) * std::cos(elevRad), std::sin(elevRad), std::sin(azimRad) * std::cos(elevRad));
		}

		ImGui::DragFloat("Altitude", &_altitude, 10.0f, 0.0f, 0.0f, "%.0fm", 2.0f);

		if(ImGui::CollapsingHeader("Atmosphere parameters")) {
			bool updateScattering = false;

			updateScattering = ImGui::InputInt("Resolution", &_tableRes) || updateScattering;
			updateScattering = ImGui::InputInt("Samples", &_tableSamples) || updateScattering;

			if(ImGui::Button("Reset")) {
				_atmoParams		 = Sky::AtmosphereParameters();
				updateScattering = true;
			}
			updateScattering = ImGui::SliderFloat("Mie height", &_atmoParams.heightMie, 100.0f, 20000.0f) || updateScattering;

			updateScattering = ImGui::SliderFloat("Mie K", &_atmoParams.kMie, 1e-6f, 100e-6f, "%.6f") || updateScattering;

			updateScattering = ImGui::SliderFloat("Rayleigh height", &_atmoParams.heightRayleigh, 100.0f, 20000.0f) || updateScattering;
			updateScattering = ImGui::SliderFloat3("Rayleigh K", &_atmoParams.kRayleigh[0], 1e-6f, 100e-6f, "%.6f") || updateScattering;

			updateScattering = ImGui::SliderFloat("Ground radius", &_atmoParams.groundRadius, 1e6f, 10e6f) || updateScattering;
			updateScattering = ImGui::SliderFloat("Atmosphere radius", &_atmoParams.topRadius, 1e6f, 10e6f) || updateScattering;

			if(updateScattering) {
				updateSky();
			}

			ImGui::SliderFloat("Mie G", &_atmoParams.gMie, 0.0f, 1.0f);
			if(ImGui::SliderFloat("Sun diameter", &_atmoParams.sunRadius, 0.0f, 0.1f)) {
				_atmoParams.sunRadiusCos = std::cos(_atmoParams.sunRadius);
			}
			ImGui::SliderFloat("Sun intensity", &_atmoParams.sunIntensity, 0.0f, 20.0f);
			
		}
	}
	ImGui::End();
}

void AtmosphereApp::resize() {
	_atmosphereBuffer->resize(_config.renderingResolution());
}

void AtmosphereApp::precomputeTable(const Sky::AtmosphereParameters & params, uint samples, Image & table) {

	// Parameters.
	const uint res = table.width;

	System::forParallel(0, res, [&](size_t y) {
		for(size_t x = 0; x < res; ++x) {
			// Move to 0,1.
			// No need to take care of the 0.5 shift as we are working with indices
			const float xf = float(x) / (res - 1.0f);
			const float yf = float(y) / (res - 1.0f);
			// Position and ray direction.
			// x becomes the height
			// y become the cosine
			const glm::vec3 currPos = glm::vec3(0.0f, (params.topRadius - params.groundRadius) * xf + params.groundRadius, 0.0f);
			const float cosA		= 2.0f * yf - 1.0f;
			const float sinA		= std::sqrt(1.0f - cosA * cosA);
			const glm::vec3 sunDir	= -glm::normalize(glm::vec3(sinA, cosA, 0.0f));
			// Check when the ray leaves the atmosphere.
			glm::vec2 interSecondTop;
			const bool didHitSecondTop = Intersection::sphere(currPos, sunDir, params.topRadius, interSecondTop);
			// Divide the distance traveled through the atmosphere in samplesCount parts.
			const float secondStepSize = didHitSecondTop ? interSecondTop.y / float(samples) : 0.0f;

			// Accumulate optical distance for both scatterings.
			float rayleighSecondDist = 0.0;
			float mieSecondDist		 = 0.0;

			// March along the secondary ray.
			for(unsigned int j = 0; j < samples; ++j) {
				// Compute the current position along the ray, ...
				const glm::vec3 currSecondPos = currPos + (float(j) + 0.5f) * secondStepSize * sunDir;
				// ...and its distance to the ground (as we are in planet space).
				const float currSecondHeight = glm::length(currSecondPos) - params.groundRadius;
				// Compute density based on the characteristic height of Rayleigh and Mie.
				const float rayleighSecondStep = exp(-currSecondHeight / params.heightRayleigh) * secondStepSize;
				const float mieSecondStep	   = exp(-currSecondHeight / params.heightMie) * secondStepSize;
				// Accumulate optical distances.
				rayleighSecondDist += rayleighSecondStep;
				mieSecondDist += mieSecondStep;
			}

			// Compute associated attenuation.
			const glm::vec3 secondaryAttenuation = exp(-(params.kMie * mieSecondDist + params.kRayleigh * rayleighSecondDist));
			table.rgb(int(x), int(y))			 = secondaryAttenuation;
		}
	});
}

void AtmosphereApp::updateSky() {
	Log::Info() << Log::Resources << "Updating sky..." << std::flush;
	_scattering.width = _scattering.height = uint(_tableRes);
	_scattering.levels = _scattering.depth = 1;
	_scattering.shape					   = TextureShape::D2;
	_scattering.clean();
	_scattering.images.emplace_back(_scattering.width, _scattering.height, 3);

	// Update the lookup table.
	precomputeTable(_atmoParams, uint(_tableSamples), _scattering.images[0]);

	_scattering.upload({Layout::RGB32F, Filter::LINEAR, Wrap::CLAMP}, false);

	Log::Info() << " done." << std::endl;
}

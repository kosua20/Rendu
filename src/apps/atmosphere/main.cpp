#include "input/Input.hpp"
#include "Application.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GLUtilities.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "generation/Random.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/**
 \defgroup AtmosphericScattering Atmospheric scattering
 \brief Demonstrate real-time approximate atmospheric scattering simulation.
 \see GPU::Frag::Atmosphere
 \ingroup Applications
 */

/** \brief Demo application for the atmospheric scattering shader.
 \ingroup AtmosphericScattering
 */
class AtmosphereApp final : public CameraApp {
public:
	
	/** Constructor
	 \param config rendering config
	 */
	AtmosphereApp(RenderingConfig & config) : CameraApp(config) {
		_userCamera.projection(config.screenResolution[0] / config.screenResolution[1], 1.34f, 0.1f, 100.0f);
		// Framebuffer to store the rendered atmosphere result before tonemapping and upscaling to the window size.
		const glm::vec2 renderRes = _config.renderingResolution();
		_atmosphereBuffer.reset(new Framebuffer(uint(renderRes[0]), uint(renderRes[1]), {Layout::RGB32F, Filter::LINEAR_NEAREST, Wrap::CLAMP}, false));
		// Lookup table.
		_precomputedScattering = Resources::manager().getTexture("scattering-precomputed", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);
		// Atmosphere screen quad.
		_atmosphere = Resources::manager().getProgram2D("atmosphere_basic");
		// Final tonemapping screen quad.
		_tonemap = Resources::manager().getProgram2D("tonemap");
		// Sun direction.
		_lightDirection = glm::normalize(glm::vec3(0.437f, 0.082f, -0.896f));
		
		GLUtilities::setDepthState(true);
		checkGLError();
	}
	
	/** \copydoc CameraApp::draw */
	void draw() override {
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
		const glm::mat4 clipToWorld   = camToWorldNoT * clipToCam;
		_atmosphere->uniform("clipToWorld", clipToWorld);
		_atmosphere->uniform("viewPos", _userCamera.position());
		_atmosphere->uniform("lightDirection", _lightDirection);
		ScreenQuad::draw(_precomputedScattering);
		_atmosphereBuffer->unbind();
		
		// Tonemapping and final screen.
		GLUtilities::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
		Framebuffer::backbuffer()->bind(Framebuffer::Mode::SRGB);
		_tonemap->use();
		ScreenQuad::draw(_atmosphereBuffer->texture());
		Framebuffer::backbuffer()->unbind();
	}
	
	/** \copydoc CameraApp::update */
	void update() override {
		CameraApp::update();
		
		if(ImGui::Begin("Atmosphere")){
			ImGui::Text("%.1f ms, %.1f fps", frameTime() * 1000.0f, frameRate());
			if(ImGui::DragFloat3("Light dir", &_lightDirection[0], 0.05f, -1.0f, 1.0f)) {
				_lightDirection = glm::normalize(_lightDirection);
			}
		}
		ImGui::End();
	}
	
	/** \copydoc CameraApp::resize */
	void resize() override {
		_atmosphereBuffer->resize(_config.renderingResolution());
	}
	
	/** \copydoc CameraApp::clean */
	void clean() override {
		_atmosphereBuffer->clean();
	}
	
private:
	std::unique_ptr<Framebuffer> _atmosphereBuffer; ///< Scene framebuffer.
	const Program * _atmosphere; ///< Atmospheric scattering shader.
	const Program * _tonemap; ///< Tonemapping shader.
	const Texture * _precomputedScattering; ///< Precomputed lookup table.
	glm::vec3 _lightDirection; ///< Sun light direction.
};

/**
 The main function of the atmospheric scattering demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup AtmosphericScattering
 */
int main(int argc, char ** argv) {
	
	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}
	
	Window window("Atmosphere", config);
	
	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/atmosphere");
	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}
	
	// Seed random generator.
	Random::seed();
	
	AtmosphereApp app(config);
	
	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
	}
	
	// Cleaning.
	app.clean();
	Resources::manager().clean();
	window.clean();
	
	return 0;
}

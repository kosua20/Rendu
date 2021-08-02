#include "input/Input.hpp"
#include "input/ControllableCamera.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/GPU.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/ScreenQuad.hpp"
#include "system/Config.hpp"
#include "system/System.hpp"
#include "system/Window.hpp"
#include "generation/Random.hpp"
#include "Common.hpp"

/**
 \defgroup Playground Playground
 \brief A basic playground for testing ideas.
 \ingroup Applications
 */

/**
 The main function of the playground.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup Playground
 */
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	Window window("Playground", config, false);
	
	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}
	
	// Seed random generator.
	Random::seed();
	// Query the renderer identifier, and the supported GPU API version.
	std::string vendor, renderer, version, shaderVersion;
	GPU::deviceInfos(vendor, renderer, version, shaderVersion);
	Log::Info() << Log::GPU << "Vendor: " << vendor << "." << std::endl;
	Log::Info() << Log::GPU << "Internal renderer: " << renderer << "." << std::endl;
	Log::Info() << Log::GPU << "Versions: Driver: " << version << ", API: " << shaderVersion << "." << std::endl;

	// Query the extensions.
	const std::vector<std::string> extensions = GPU::supportedExtensions();
	// Log extensions.
	size_t extensionCount = extensions.size();
	for(const auto& ext : extensions){
		if(ext[0] == '-'){
			--extensionCount;
		}
	}

	if(!extensions.empty()) {
		Log::Info() << Log::GPU << "Extensions detected (" << extensionCount << "): " << std::endl;
		for(size_t i = 0; i < extensions.size(); ++i) {
			const bool isHeader = extensions[i][0] == '-';
			Log::Info() << (isHeader ? "\n" : "") << extensions[i] << (isHeader ? "\n" : ", ") << std::flush;
		}
		Log::Info() << std::endl;
	}
	const std::string titleHeader = "Extensions (" + std::to_string(extensionCount) + ")";

	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

	// Setup the timer.
	double timer		 = System::time();
	double fullTime		 = 0.0;
	double remainingTime = 0.0;
	const double dt		 = 1.0 / 120.0; // Small physics timestep.

	Program * program = Resources::manager().getProgram("object", "object_basic", "object_basic_random");
	Program * program3 = Resources::manager().getProgram2D("passthrough");
	const Mesh * mesh		= Resources::manager().getMesh("light_sphere", Storage::GPU);

	Program * program2 = Resources::manager().getProgram("object_basic_color");
	Mesh mesh2("Plane");
	mesh2.positions = {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f)};
	mesh2.colors = {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f)};
	mesh2.indices = {0, 1, 2, 0, 2, 3};
	mesh2.upload();

	Mesh mesh3("Triangle");
	mesh3.positions = {glm::vec3(-0.5f, -0.5f, 0.1f), glm::vec3(0.5f, -0.5f, 0.1f), glm::vec3(0.0f, 1.0f, 0.1f)};
	mesh3.colors = {glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 1.0f)};
	mesh3.indices = {0, 2, 1};
	mesh3.upload();


	ControllableCamera camera;

	camera.pose(glm::vec3(0.0f,0.0f,3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	camera.projection(config.screenResolution[0] / config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	bool showImGuiDemo = false;

	const Texture* tex = Resources::manager().getTexture("debug-grid", {Layout::RGBA16F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

	Texture tex2("textest");
	tex2.width = tex2.height = 8;
	tex2.depth = tex2.levels = 1;
	tex2.shape = TextureShape::D2;
	tex2.images.emplace_back(tex2.width, tex2.height, 4);
	auto& texData = tex2.images.back();

	uint count = 0;
	Descriptor desc = {Layout::RGBA8, Filter::LINEAR, Wrap::CLAMP};
	Framebuffer fb(400, 300, desc , true, "testfb");


	for(uint dy = 0; dy < tex2.height; ++dy){
	   for(uint dx = 0; dx < tex2.width; ++dx){
		   float r = 0.0f; float g = 0.0f; float b = 0.0f;
		   uint x = (dx + 1)%8;
		   uint y = (dy + 1)%8;
		   if(x < 2){
			   r = 1.0f;
		   } else if (x < 4){
			   g = 1.0f;
		   } else if(x < 6){
			   b = 1.0f;
		   }
		   if(y < 2){
			   b = 0.5f;
		   } else if (y < 4){
			   r = 0.5f;
		   } else if(y < 6){
			   g = 0.5f;
		   }
		   r = float(9 % 60)/60.0f;
		   texData.rgba(x,y) = glm::vec4(r,g,b,0.5f);
	   }
   }
   tex2.upload({Layout::RGBA32F, Filter::NEAREST_LINEAR, Wrap::CLAMP}, true);

   mesh3.positions.clear();
   mesh3.positions = {glm::vec3(-0.5f, -0.5f, 0.1f), glm::vec3(0.5f, -0.5f, 0.1f), glm::vec3(0.0f, 1.0f, 0.1f)};
   mesh3.upload();

	Filter imageInterp = Filter::LINEAR;

	// Start the display/interaction loop.
	while(window.nextFrame()) {
		
		// Reload resources.
		if(Input::manager().triggered(Input::Key::P)) {
			Resources::manager().reload();
		}

		// Compute the time elapsed since last frame
		const double currentTime = System::time();
		double frameTime		 = currentTime - timer;
		timer					 = currentTime;

		camera.update();

		// Physics simulation
		// First avoid super high frametime by clamping.
		if(frameTime > 0.2) {
			frameTime = 0.2;
		}
		// Accumulate new frame time.
		remainingTime += frameTime;
		// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
		while(remainingTime > 0.2 * dt) {
			double deltaTime = fmin(remainingTime, dt);
			// Update physics and camera.
			camera.physics(deltaTime);
			// Update timers.
			fullTime += deltaTime;
			remainingTime -= deltaTime;
		}

		if(Input::manager().resized()){
			fb.resize(Input::manager().size());
		}

		// Render.
		const glm::mat4 MVP		   = camera.projection() * camera.view();
		GPU::setDepthState(true, TestFunction::LESS, true);
		fb.bind(glm::vec4(0.2f, 0.3f, 0.25f, 1.0f), 1.0f, Framebuffer::Operation::DONTCARE);

		fb.setViewport();

		if(ImGui::Combo("Filtering", reinterpret_cast<int *>(&imageInterp), "Nearest\0Linear\0\0")) {
			tex->gpu->setFiltering(imageInterp);
		}
		//GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::SRC_ALPHA, BlendFunction::ONE_MINUS_SRC_ALPHA);

		++count;

		program2->use();
		program2->uniform("mvp", MVP);
		program2->uniform("color", glm::vec3(1.0f, 0.5f, 0.0f));
		program2->texture(tex, 0);
		GPU::drawMesh(mesh2);
		//GPU::setCullState(true, Faces::BACK);
		//GPU::setBlendState(false);


		//program2->texture(tex2, 0);
		program2->uniform("color", glm::vec3(0.0f, 0.5f, 1.0f));
		GPU::drawMesh(mesh3);

		//GPU::setViewport(screenSize[0]/2, 0, screenSize[0]/2, screenSize[1]);
		//GPU::drawMesh(mesh2);
		//GPU::drawMesh(mesh3);

		//Framebuffer::backbuffer()->bind(glm::vec4(1.0f, 0.5f, 0.2f, 1.0f), 1.0f, Framebuffer::Operation::DONTCARE);
		//Framebuffer::backbuffer()->setViewport();
		GPU::blit(fb, *Framebuffer::backbuffer(), Filter::LINEAR);
		Framebuffer::backbuffer()->bind(Framebuffer::Operation::LOAD);
		Framebuffer::backbuffer()->setViewport();
		GPU::setDepthState(false);

		program->use();
		program->uniform("mvp", MVP);
		GPU::drawMesh(*mesh);

		GPU::setViewport(0, 0, 300, 520);
		program3->use();
		program3->texture(fb.texture(), 0);
		ScreenQuad::draw();

		ImGui::Text("ImGui is functional!");
		ImGui::SameLine();
		ImGui::Checkbox("Show demo", &showImGuiDemo);
		ImGui::Text("%.1f ms, %.1f fps", frameTime * 1000.0f, 1.0f/frameTime);
		ImGui::Separator();

		ImGui::Text("OpengGL vendor: %s", vendor.c_str());
		ImGui::Text("Internal renderer: %s", renderer.c_str());
		ImGui::Text("Versions: Driver: %s, GLSL: %s", version.c_str(), shaderVersion.c_str());
		if(ImGui::CollapsingHeader(titleHeader.c_str())) {
			for(const auto & ext : extensions) {
				ImGui::Text("%s", ext.c_str());
			}
		}

		if(showImGuiDemo) {
			ImGui::ShowDemoWindow();
		}


	}

	tex2.clean();
	mesh2.clean();
	mesh3.clean();

	return 0;
}

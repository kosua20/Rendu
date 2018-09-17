#include "Common.hpp"
#include "helpers/GenerationUtilities.hpp"
#include "input/Input.hpp"
#include "input/InputCallbacks.hpp"
#include "input/ControllableCamera.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/GLUtilities.hpp"
#include "resources/ImageUtilities.hpp"
#include "Config.hpp"

/**
 \defgroup ImageViewer Image Viewer
 \brief A basic image viewer, supports LDR and HDR images.
 \ingroup Applications
 */

/**
 The main function of the image viewer.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup ImageViewer
 */
int main(int argc, char** argv) {
	
	// First, init/parse/load configuration.
	Config config(argc, argv);
	if(!config.logPath.empty()){
		Log::setDefaultFile(config.logPath);
	}
	Log::setDefaultVerbose(config.logVerbose);
	
	GLFWwindow* window = Interface::initWindow("Image viewer", config);
	if(!window){
		return -1;
	}
	// Initialize random generator;
	Random::seed();
	
	glEnable(GL_CULL_FACE);
	
	// Create the rendering program.
	std::shared_ptr<ProgramInfos> program = Resources::manager().getProgram("image_display");
	
	// Infos on the current texture.
	TextureInfos imageInfos;
	
	// Settings.
	glm::vec3 bgColor(0.6f);
	float exposure = 1.0f;
	bool applyGamma = true;
	
	// Start the display/interaction loop.
	while (!glfwWindowShouldClose(window)) {
		// Update events (inputs,...).
		Input::manager().update();
		// Handle quitting.
		if(Input::manager().pressed(Input::KeyEscape)){
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		// Start a new frame for the interface.
		Interface::beginFrame();
		// Reload resources.
		if(Input::manager().triggered(Input::KeyP)){
			Resources::manager().reload();
		}
		
		// Screen infos.
		const glm::vec2 screenSize = Input::manager().size();
		glViewport(0, 0, (GLsizei)screenSize[0], (GLsizei)screenSize[1]);
		// Render the background.
		glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Render the image if non empty.
		bool hasImage = imageInfos.width > 0 && imageInfos.height > 0;
		if(hasImage){
			// Compute image and screen infos.
			const glm::vec2 imageSize(imageInfos.width, imageInfos.height);
			float screenRatio = std::max(screenSize[1], 1.0f) / std::max(screenSize[0], 1.0f);
			float imageRatio = imageSize[1] / imageSize[0];
			float widthRatio = screenSize[0] / imageSize[0];
			
			glEnable(GL_BLEND);
			
			// Render the image.
			glUseProgram(program->id());
			// Pass settings.
			glUniform1f(program->uniform("screenRatio"), screenRatio);
			glUniform1f(program->uniform("imageRatio"), imageRatio);
			glUniform1f(program->uniform("widthRatio"), widthRatio);
			glUniform1i(program->uniform("isHDR"), imageInfos.hdr);
			glUniform1f(program->uniform("exposure"), exposure);
			glUniform1i(program->uniform("gammaOutput"), applyGamma);
			// Draw.
			ScreenQuad::draw(imageInfos.id);
			
			glDisable(GL_BLEND);
		}
		
		// Interface.
		if(ImGui::Begin("Options")){
			// Image loader.
			if(ImGui::Button("Load image...")){
				std::string newImagePath;
				bool res = Interface::showPicker(Interface::Load, "../../../resources", newImagePath, "jpg,bmp,png,tga;exr");
				// If user picked a path, load the texture from disk.
				if(res && !newImagePath.empty()){
					Log::Info() << "Loading " << newImagePath << "." << std::endl;
					imageInfos = GLUtilities::loadTexture({newImagePath}, true);
				}
			}
			// Infos.
			if(hasImage){
				ImGui::Text(imageInfos.hdr ? "HDR image (%dx%d)." : "LDR image (%dx%d).", imageInfos.width, imageInfos.height);
			}
			
			// Gamma and exposure.
			ImGui::Checkbox("Gamma", &applyGamma);
			if(imageInfos.hdr){
				ImGui::PushItemWidth(50);
				ImGui::SliderFloat("Exposure", &exposure, 0.0f, 10.0f);
				ImGui::PopItemWidth();
			}
			
			// Background color.
			ImGui::ColorEdit3("Background", &bgColor[0]);
			
		}
		
		ImGui::End();
		
		// Then render the interface.
		Interface::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);

	}
	
	// Clean the interface.
	Interface::clean();
	// Remove the window.
	glfwDestroyWindow(window);
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	
	return 0;
}



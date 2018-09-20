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
	glm::bvec4 channelsFilter(true);
	// Filtering mode.
	enum FilteringMode {
		Nearest = 0, Linear = 1
	};
	FilteringMode imageInterp = Linear;
	// Orientation.
	glm::bvec2 flipAxis(false);
	int currentAngle = 0;
	// Scale and position.
	float pixelScale = 1.0f;
	float zoomSpeed = 0.01f;
	glm::vec2 mouseShift = glm::vec2(0.0f,0.0f);
	glm::vec2 mousePrev = glm::vec2(0.0f,0.0f);
	glm::vec3 fgColor(0.6f);
	
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
		
		// Update scale and position.
		// Scale when scrolling, with safety bounds.
		pixelScale += Input::manager().scroll().y * zoomSpeed;
		pixelScale = std::max(0.001f,std::min(1000.0f,pixelScale));
		// Register left-click and drag.
		if(Input::manager().triggered(Input::MouseLeft)){
			mousePrev = Input::manager().mouse();
		}
		if(Input::manager().pressed(Input::MouseLeft)){
			const glm::vec2 mouseNew = Input::manager().mouse();
			mouseShift += pixelScale * (mouseNew - mousePrev);
			mousePrev = mouseNew;
		}
		
		// Screen infos.
		const glm::vec2 screenSize = Input::manager().size();
		glViewport(0, 0, (GLsizei)screenSize[0], (GLsizei)screenSize[1]);
		// Render the background.
		glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Render the image if non empty.
		bool hasImage = imageInfos.width > 0 && imageInfos.height > 0;
		const bool isHorizontal = currentAngle == 1 || currentAngle == 3;
		
		if(hasImage){
			// Depending on the current rotation, the horizontal dimension of the image is the width or the height.
			const unsigned int widthIndex = isHorizontal ? 1 : 0;
			// Compute image and screen infos.
			const glm::vec2 imageSize(imageInfos.width, imageInfos.height);
			float screenRatio = std::max(screenSize[1], 1.0f) / std::max(screenSize[0], 1.0f);
			float imageRatio = imageSize[1-widthIndex] / imageSize[widthIndex];
			float widthRatio = screenSize[0] / imageSize[0] * imageSize[widthIndex] / imageSize[0];
			
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
			glUniform4f(program->uniform("channelsFilter"), channelsFilter[0], channelsFilter[1], channelsFilter[2], channelsFilter[3]);
			glUniform2f(program->uniform("flipAxis"), flipAxis[0], flipAxis[1]);
			glUniform2f(program->uniform("angleTrig"), std::cos(currentAngle*M_PI_2), std::sin(currentAngle*M_PI_2));
			glUniform1f(program->uniform("pixelScale"), pixelScale);
			glUniform2fv(program->uniform("mouseShift"), 1, &mouseShift[0]);
			
			// Draw.
			ScreenQuad::draw(imageInfos.id);
			
			glDisable(GL_BLEND);

			// Read back color under cursor when right-clicking.
			if(Input::manager().pressed(Input::MouseRight)){
				const glm::vec2 mousePosition = Input::manager().mouse(true);
				glReadPixels(int(mousePosition.x), int(mousePosition.y), 1, 1, GL_RGB, GL_FLOAT, &fgColor[0]);
			}
		
		}
		
		// Interface.
		ImGui::SetNextWindowPos(ImVec2(10,10));
		ImGui::SetNextWindowSize(ImVec2(285, 260), ImGuiCond_Appearing);
		if(ImGui::Begin("Image viewer")){
			
			// Infos.
			if(hasImage){
				ImGui::Text(imageInfos.hdr ? "HDR image (%dx%d)." : "LDR image (%dx%d).", imageInfos.width, imageInfos.height);
			} else {
				ImGui::Text("No image.");
			}
			
			// Image loader.
			if(ImGui::Button("Load image...")){
				std::string newImagePath;
				bool res = Interface::showPicker(Interface::Load, "../../../resources", newImagePath, "jpg,bmp,png,tga;exr");
				// If user picked a path, load the texture from disk.
				if(res && !newImagePath.empty()){
					Log::Info() << "Loading " << newImagePath << "." << std::endl;
					imageInfos = GLUtilities::loadTexture({newImagePath}, true);
					// Apply the proper filtering.
					const GLenum filteringSetting = (imageInterp == Nearest) ? GL_NEAREST : GL_LINEAR;
					glBindTexture(GL_TEXTURE_2D, imageInfos.id);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filteringSetting);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filteringSetting);
					glBindTexture(GL_TEXTURE_2D, 0);
					// Reset display settings.
					pixelScale = 1.0f;
					mouseShift = glm::vec2(0.0f);
					currentAngle = 0;
					flipAxis = glm::bvec2(false);
					channelsFilter = glm::vec4(true);
				}
			}
			ImGui::SameLine();
			// Save button.
			const bool saveImage = ImGui::Button("Save image");
			
			// Gamma and exposure.
			ImGui::Checkbox("Gamma", &applyGamma);
			if(imageInfos.hdr){
				ImGui::SameLine();
				ImGui::PushItemWidth(120);
				ImGui::SliderFloat("Exposure", &exposure, 0.0f, 10.0f);
				ImGui::PopItemWidth();
			}
			
			// Channels.
			ImGui::Checkbox("R", &channelsFilter[0]); ImGui::SameLine();
			ImGui::Checkbox("G", &channelsFilter[1]); ImGui::SameLine();
			ImGui::Checkbox("B", &channelsFilter[2]); ImGui::SameLine();
			ImGui::Checkbox("A", &channelsFilter[3]);
			
			// Filtering.
			if(ImGui::Combo("Filtering", (int*)(&imageInterp), "Nearest\0Linear\0\0")){
				const GLenum filteringSetting = (imageInterp == Nearest) ? GL_NEAREST : GL_LINEAR;
				glBindTexture(GL_TEXTURE_2D, imageInfos.id);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filteringSetting);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filteringSetting);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			
			// Image modifications.
			// Rotation.
			ImGui::Text("Rotate");
			ImGui::SameLine();
			if(ImGui::Button("<")){
				currentAngle--;
				if(currentAngle < 0){
					currentAngle += 4;
				}
			}
			ImGui::SameLine();
			if(ImGui::Button(">")){
				currentAngle=(currentAngle+1)%4;
			}
			ImGui::SameLine();
			// Mirror.
			ImGui::Text("Flip"); ImGui::SameLine();
			ImGui::Checkbox("X", &flipAxis[1]); ImGui::SameLine();
			ImGui::Checkbox("Y", &flipAxis[0]);
			
			// Colors.
			ImGui::ColorEdit3("Foreground", &fgColor[0]);
			ImGui::ColorEdit3("Background", &bgColor[0]);
			
			// Scaling speed.
			ImGui::SliderFloat("Zoom speed", &zoomSpeed, 0.001f, 0.1f, "%.3f", 4.0f);
			// Position
			if(ImGui::Button("Reset pos.")){
				pixelScale = 1.0f;
				mouseShift = glm::vec2(0.0f);
			}
			ImGui::SameLine();
			ImGui::Text("%.1f%%, (%d,%d)", 100.0f/pixelScale, int((-mouseShift.x+0.5)*imageInfos.width), int((mouseShift.y+0.5)*imageInfos.height));
			
			// Save the image in its current flip/rotation/channels/exposure/gamma settings.
			if(saveImage){
				std::string destinationPath;
				// Export either in LDR or HDR.
				bool res = Interface::showPicker(Interface::Save, "../../../resources", destinationPath, "png;exr");
				if(res && !destinationPath.empty()){
					const GLenum typedFormat = ImageUtilities::isHDR(destinationPath) ? GL_RGBA32F : GL_RGBA8;
					// Create a framebuffer at the right size and format, and render in it.
					const unsigned int outputWidth = isHorizontal ? imageInfos.height : imageInfos.width;
					const unsigned int outputHeight = isHorizontal ? imageInfos.width : imageInfos.height;
					std::shared_ptr<Framebuffer> framebuffer = std::make_shared<Framebuffer>(outputWidth, outputHeight, typedFormat, false);
					framebuffer->bind();
					framebuffer->setViewport();
					
					// Render the image in it.
					glEnable(GL_BLEND);
					glUseProgram(program->id());
					// No scaling or translation.
					glUniform1f(program->uniform("screenRatio"), 1.0f);
					glUniform1f(program->uniform("imageRatio"), 1.0f);
					glUniform1f(program->uniform("widthRatio"), 1.0f);
					glUniform1f(program->uniform("pixelScale"), 1.0f);
					glUniform2f(program->uniform("mouseShift"), 0.0f, 0.0f);
					ScreenQuad::draw(imageInfos.id);
					glDisable(GL_BLEND);
					
					framebuffer->unbind();
					
					// Then save it to the given path.
					GLUtilities::saveFramebuffer(framebuffer, outputWidth, outputHeight, destinationPath.substr(0, destinationPath.size()-4), true, false);
				}
			}
			
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



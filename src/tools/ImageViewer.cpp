#include "input/Input.hpp"
#include "system/System.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUObjects.hpp"
#include "resources/Image.hpp"
#include "graphics/Framebuffer.hpp"
#include "system/Config.hpp"
#include "system/Window.hpp"
#include "Common.hpp"

/**
 \defgroup ImageViewer Image Viewer
 \brief A basic image viewer, supports LDR and HDR images.
 \ingroup Tools
 */

/**
 The main function of the image viewer.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup ImageViewer
 */
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	Window window("Image viewer", config);
	
	Resources::manager().addResources("../../../resources/imageviewer");

	// Create the rendering program.
	Program * program = Resources::manager().getProgram("image_display");

	// Infos on the current texture.
	Texture imageInfos("image");
	bool isFloat = false;

	// Settings.
	glm::vec3 bgColor(0.3f);
	float exposure  = 1.0f;
	bool applyGamma = true;
	glm::bvec4 channelsFilter(true);
	// Filtering mode.
	Filter imageInterp = Filter::LINEAR;
	// Orientation.
	glm::bvec2 flipAxis(false);
	int currentAngle = 0;
	// Scale and position.
	float pixelScale	 = 1.0f;
	float zoomSpeed		 = 0.01f;
	glm::vec2 mouseShift = glm::vec2(0.0f, 0.0f);
	glm::vec2 mousePrev  = glm::vec2(0.0f, 0.0f);
	glm::vec3 fgColor(0.6f);
	GPUAsyncTask readbackTask = 0;

	// Start the display/interaction loop.
	while(window.nextFrame()) {

		// Update scale and position.
		// Scale when scrolling, with safety bounds.
		pixelScale += Input::manager().scroll().y * zoomSpeed;
		pixelScale = std::max(0.001f, std::min(1000.0f, pixelScale));
		// Register left-click and drag.
		if(Input::manager().triggered(Input::Mouse::Left)) {
			mousePrev = Input::manager().mouse();
		}
		if(Input::manager().pressed(Input::Mouse::Left)) {
			const glm::vec2 mouseNew = Input::manager().mouse();
			mouseShift += pixelScale * (mouseNew - mousePrev);
			mousePrev = mouseNew;
		}

		// Render the background.
		window.bind(glm::vec4(bgColor, 1.0f), 1.0f);
		window.setViewport();

		const glm::ivec2 screenSize = Input::manager().size();

		// Render the image if non empty.
		bool hasImage			= imageInfos.width > 0 && imageInfos.height > 0;
		const bool isHorizontal = currentAngle == 1 || currentAngle == 3;

		if(hasImage) {
			// Depending on the current rotation, the horizontal dimension of the image is the width or the height.
			const unsigned int widthIndex = isHorizontal ? 1 : 0;
			// Compute image and screen infos.
			const glm::vec2 imageSize(imageInfos.width, imageInfos.height);
			float screenRatio = float(std::max(screenSize[1], 1)) / float(std::max(screenSize[0], 1));
			float imageRatio  = imageSize[1 - widthIndex] / imageSize[widthIndex];
			float widthRatio  = float(screenSize[0]) / imageSize[0] * imageSize[widthIndex] / imageSize[0];

			GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::SRC_ALPHA, BlendFunction::ONE_MINUS_SRC_ALPHA);
			GPU::setDepthState(false);
			GPU::setCullState(false);

			// Render the image.
			program->use();
			// Pass settings.
			program->uniform("screenRatio", screenRatio);
			program->uniform("imageRatio", imageRatio);
			program->uniform("widthRatio", widthRatio);
			program->uniform("isHDR", isFloat);
			program->uniform("exposure", exposure);
			program->uniform("gammaOutput", applyGamma);
			program->uniform("filtering", imageInterp == Filter::LINEAR);
			const glm::vec4 chanFilts(channelsFilter);
			const glm::vec2 flips(flipAxis);
			const glm::vec2 angles(std::cos(float(currentAngle) * glm::half_pi<float>()), std::sin(float(currentAngle) * glm::half_pi<float>()));

			program->uniform("channelsFilter", chanFilts);
			program->uniform("flipAxis", flips);
			program->uniform("angleTrig", angles);
			program->uniform("pixelScale", pixelScale);
			program->uniform("mouseShift", mouseShift);
			program->texture(imageInfos, 0);
			// Draw.
			ScreenQuad::draw();

			// Read back color under cursor when right-clicking.
			if(Input::manager().pressed(Input::Mouse::Right)) {
				const glm::vec2 pos = Input::manager().mouse();
				const uint w = window.color().width;
				const uint h = window.color().height;
				glm::vec2 mousePosition = glm::floor(glm::vec2(pos.x * float(w), pos.y * float(h)));
				mousePosition		  = glm::clamp(mousePosition, glm::vec2(0.0f), glm::vec2(w, h));

				readbackTask = GPU::downloadTextureAsync( window.color(), mousePosition, glm::uvec2(2), 1, [&fgColor](const Texture& result){
					fgColor = result.images[0].rgba(0, 0);
				});
			}
		}

		// Interface.
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::SetNextWindowSize(ImVec2(285, 270), ImGuiCond_Appearing);
		if(ImGui::Begin("Image viewer")) {

			// Infos.
			if(hasImage) {
				ImGui::Text(isFloat ? "HDR image (%dx%d)." : "LDR image (%dx%d).", imageInfos.width, imageInfos.height);
			} else {
				ImGui::Text("No image.");
			}

			// Image loader.
			if(ImGui::Button("Load image...")) {
				std::string newImagePath;
				bool res = System::showPicker(System::Picker::Load, "../../../resources", newImagePath, "jpg,bmp,png,tga;exr");
				// If user picked a path, load the texture from disk.
				if(res && !newImagePath.empty()) {
					Log::Info() << "Loading " << newImagePath << "." << std::endl;
					isFloat = Image::isFloat(newImagePath);
					// Load all images without gamma correction.
					const Layout typedFormat = isFloat ? Layout::RGBA32F : Layout::RGBA8;
					// For LDR images we have to apply the gamma correction by default (because we skipped it just above).
					// For HDR images there is no correction by default.
					applyGamma = !isFloat;

					imageInfos.clean();
					imageInfos.shape  = TextureShape::D2;
					imageInfos.depth  = 1;
					imageInfos.levels = 1;
					imageInfos.images.emplace_back();
					Image & img   = imageInfos.images.back();
					const int ret = img.load(newImagePath, 4, false, false);
					if(ret != 0) {
						Log::Error() << Log::Resources << "Unable to load the texture at path " << newImagePath << "." << std::endl;
						continue;
					}
					imageInfos.width  = img.width;
					imageInfos.height = img.height;
					imageInfos.upload(typedFormat, false);
					imageInfos.clearImages();

					// Reset display settings.
					pixelScale	 = 1.0f;
					mouseShift	 = glm::vec2(0.0f);
					currentAngle   = 0;
					flipAxis	   = glm::bvec2(false);
					channelsFilter = glm::vec4(true);
				}
			}
			ImGui::SameLine();
			// Save button.
			const bool saveImage = ImGui::Button("Save image");

			// Gamma and exposure.
			ImGui::Checkbox("Gamma", &applyGamma);
			if(isFloat) {
				ImGui::SameLine();
				ImGui::PushItemWidth(120);
				ImGui::SliderFloat("Exposure", &exposure, 0.0f, 10.0f);
				ImGui::PopItemWidth();
			}

			// Channels.
			ImGui::Checkbox("R", &channelsFilter[0]);
			ImGui::SameLine();
			ImGui::Checkbox("G", &channelsFilter[1]);
			ImGui::SameLine();
			ImGui::Checkbox("B", &channelsFilter[2]);
			ImGui::SameLine();
			ImGui::Checkbox("A", &channelsFilter[3]);

			// Filtering.
			ImGui::Combo("Filtering", reinterpret_cast<int *>(&imageInterp), "Nearest\0Linear\0\0");

			// Image modifications.
			// Rotation.
			ImGui::Text("Rotate");
			ImGui::SameLine();
			if(ImGui::Button("<")) {
				currentAngle--;
				if(currentAngle < 0) {
					currentAngle += 4;
				}
			}
			ImGui::SameLine();
			if(ImGui::Button(">")) {
				currentAngle = (currentAngle + 1) % 4;
			}
			ImGui::SameLine();
			// Mirror.
			ImGui::Text("Flip");
			ImGui::SameLine();
			ImGui::Checkbox("X", &flipAxis[1]);
			ImGui::SameLine();
			ImGui::Checkbox("Y", &flipAxis[0]);

			// Colors.
			ImGui::ColorEdit3("Foreground", &fgColor[0]);
			ImGui::ColorEdit3("Background", &bgColor[0]);

			// Scaling speed.
			ImGui::SliderFloat("Zoom speed", &zoomSpeed, 0.001f, 0.1f, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
			// Position
			if(ImGui::Button("Reset pos.")) {
				pixelScale = 1.0f;
				mouseShift = glm::vec2(0.0f);
			}
			ImGui::SameLine();
			ImGui::Text("%.1f%%, (%d,%d)", 100.0f / pixelScale, int((-mouseShift.x + 0.5) * imageInfos.width), int((mouseShift.y + 0.5) * imageInfos.height));

			// Save the image in its current flip/rotation/channels/exposure/gamma settings.
			if(saveImage) {
				std::string destinationPath;
				// Export either in LDR or HDR.
				bool res = System::showPicker(System::Picker::Save, "../../../resources", destinationPath, "png;exr");
				if(res && !destinationPath.empty()) {
					const Layout format = Image::isFloat(destinationPath) ? Layout::RGBA32F : Layout::RGBA8;
					// Create a framebuffer at the right size and format, and render in it.
					const unsigned int outputWidth  = isHorizontal ? imageInfos.height : imageInfos.width;
					const unsigned int outputHeight = isHorizontal ? imageInfos.width : imageInfos.height;
					Framebuffer framebuffer(outputWidth, outputHeight, format, "Save output");
					framebuffer.bind(glm::vec4(0.0f,0.0f,0.0f,1.0f), Load::Operation::DONTCARE, Load::Operation::DONTCARE);
					framebuffer.setViewport();

					// Render the image in it.
					GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::SRC_ALPHA, BlendFunction::ONE_MINUS_SRC_ALPHA);
					GPU::setDepthState(false);
					GPU::setCullState(false);

					program->use();
					// No scaling or translation.
					const glm::vec2 zeros(0.0f);
					program->uniform("screenRatio", 1.0f);
					program->uniform("imageRatio", 1.0f);
					program->uniform("widthRatio", 1.0f);
					program->uniform("pixelScale", 1.0f);
					program->uniform("mouseShift", zeros);
					program->texture(imageInfos, 0);
					ScreenQuad::draw();

					// Then save it to the given path.
					GPU::saveTexture(*framebuffer.texture(0), destinationPath.substr(0, destinationPath.size() - 4), Image::Save::NONE);

				}
			}
		}
		ImGui::End();

	}

	GPU::cancelAsyncOperation(readbackTask);
	return 0;
}

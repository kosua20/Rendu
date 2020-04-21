#include "resources/Image.hpp"
#include "resources/ResourcesManager.hpp"
#include "renderers/Probe.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/GLUtilities.hpp"
#include "input/Input.hpp"
#include "input/ControllableCamera.hpp"
#include "system/System.hpp"
#include "system/Window.hpp"
#include "system/TextUtilities.hpp"
#include "system/Random.hpp"
#include "Common.hpp"

#include <sstream>
#include <array>

/**
 \defgroup BRDFEstimator BRDF Estimation
 \brief Precompute BRDF-related data for real-time rendering.
 \details Perform cubemap GGX convolution, irradiance SH coefficients computation, and linearized BRDF look-up table precomputation.
 \see GPU::Frag::Cubemap_convo
 \see GPU::Frag::Brdf_sampler
 \see GPU::Frag::Skybox_shcoeffs
 \see DeferredRendering
 \ingroup Tools
 */

/// Cubemap default prefixes.
const std::vector<std::string> suffixes = {"_px", "_nx", "_py", "_ny", "_pz", "_nz"};

/**
 Load a cubemap on both the CPU and GPU from an input path.
 \param inputPath the base path on disk
 \param cubemapInfos will contain the cubemap infos once sent to the GPU
 \ingroup BRDFEstimator
 */
void loadCubemap(const std::string & inputPath, Texture & cubemapInfos) {
	std::string cubemapPath = inputPath;
	const std::string ext   = TextUtilities::removeExtension(cubemapPath);
	cubemapPath				= cubemapPath.substr(0, cubemapPath.size() - 3);
	Log::Info() << "Loading " << cubemapPath << "..." << std::endl;
	std::vector<std::string> pathSides(6);
	for(int i = 0; i < 6; ++i) {
		pathSides[i].append(cubemapPath);
		pathSides[i].append(suffixes[i]);
		pathSides[i].append(ext);
	}

	cubemapInfos.clean();
	cubemapInfos.shape  = TextureShape::Cube;
	cubemapInfos.depth  = 6;
	cubemapInfos.levels = 1;
	for(const auto & filePath : pathSides) {
		cubemapInfos.images.emplace_back();
		Image & image = cubemapInfos.images.back();
		const int ret = image.load(filePath, 4, false, false);
		if(ret != 0) {
			Log::Error() << Log::Resources << "Unable to load the texture at path " << filePath << "." << std::endl;
		}
	}
	cubemapInfos.width  = cubemapInfos.images[0].width;
	cubemapInfos.height = cubemapInfos.images[0].height;
	cubemapInfos.upload({Layout::RGBA32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, false);
}

/**
 Compute a series of cubemaps convolved with a BRDF using increasing roughness values. The cubemaps form a mipmap pyramid.
 \note We choose to keep the levels in separate textures for easier visualisation. This could be revisited.
 \param cubemapInfos the source HDR cubemap
 \param levelsCount the number of mipmap levels to generate
 \param outputSide the side size of the lvel 0 cubemap faces
 \param samplesCount the number of samples to use in the convolution
 \param cubeLevels will contain the texture infos for each level
 \ingroup BRDFEstimator
 */
void computeCubemapConvolution(const Texture & cubemapInfos, int levelsCount, int outputSide, int samplesCount, std::vector<Texture> & cubeLevels) {

	cubeLevels.clear();

	// Generate 6 view-projection matrices corresponding to 6 cameras at a 90° angle and with a 90° field of view.
	std::vector<glm::mat4> MVPs(6);
	{
		const glm::mat4 projection = glm::perspective(glm::half_pi<float>(), 1.0f, 0.1f, 200.0f);
		const glm::vec3 ups[6]	 = {glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, -1.0, 0.0)};
		const glm::vec3 centers[6] = {glm::vec3(1.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -1.0)};
		for(int i = 0; i < 6; ++i) {
			MVPs[i] = projection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), centers[i], ups[i]);
		}
	}
	// Create shader program for roughness pre-convolution.
	const auto programCubemap = Resources::manager().getProgram("cubemap_convo", "skybox_basic", "cubemap_convo");
	const auto mesh			  = Resources::manager().getMesh("skybox", Storage::GPU);

	// Generate convolution map for increments of roughness.
	Log::Info() << Log::Utilities << "Convolving BRDF with cubemap." << std::endl;

	for(int level = 0; level < levelsCount; ++level) {

		const unsigned int w  = outputSide / int(std::pow(2, level));
		const unsigned int h  = w;
		const float roughness = float(level) / float(levelsCount - 1);

		Log::Info() << Log::Utilities << "Level " << level << " (size=" << w << ", r=" << roughness << "): " << std::flush;

		// Create local framebuffer.
		const Descriptor resDesc = {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::CLAMP};
		Framebuffer resultFramebuffer(TextureShape::Cube, w, w, 6, 1, {resDesc}, false);
		
		// Iterate over faces.
		for(size_t i = 0; i < 6; ++i) {
			Log::Info() << "." << std::flush;

			resultFramebuffer.bind(i);
			// Clear texture slice.
			GLUtilities::clearColorAndDepth({0.0f, 0.0f, 0.0f, 1.0f}, 1.0f);
			GLUtilities::setViewport(0, 0, int(w), int(h));
			GLUtilities::setDepthState(false);
			programCubemap->use();
			// Pass roughness parameters.
			programCubemap->uniform("mipmapRoughness", roughness);
			programCubemap->uniform("mvp", MVPs[i]);
			programCubemap->uniform("samplesCount", samplesCount);
			// Attach source cubemap and compute.
			GLUtilities::bindTexture(&cubemapInfos, 0);
			GLUtilities::drawMesh(*mesh);
			resultFramebuffer.unbind();
			GLUtilities::setDepthState(false);
			// Force synchronization.
			GLUtilities::sync();
		}

		// Now resultFramebuffer contain the texture data. But its lifetime is limited to this scope.
		// Thus we perform a copy to our final texture.
		cubeLevels.emplace_back();
		Texture & levelInfos = cubeLevels.back();
		GLUtilities::blit(*resultFramebuffer.textureId(), levelInfos, Filter::NEAREST);
		resultFramebuffer.clean();

		Log::Info() << std::endl;
	}
	GLUtilities::setDepthState(true);
}

/** Export the pre-convolved cubemap levels.
 \param cubeLevels the textures to export as mipmap levels
 \param outputPath the based destination path
 \ingroup BRDFEstimator
 */
void exportCubemapConvolution(std::vector<Texture> & cubeLevels, const std::string & outputPath) {
	for(int level = 0; level < int(cubeLevels.size()); ++level) {
		Texture & texture = cubeLevels[level];
		GLUtilities::downloadTexture(texture);

		const std::string levelPath = outputPath + "_" + std::to_string(level);
		for(int i = 0; i < 6; ++i) {
			const std::string faceLevelPath = levelPath + suffixes[i];
			const int ret					= texture.images[i].save(faceLevelPath + ".exr", false, true);
			if(ret != 0) {
				Log::Error() << "Unable to save cubemap face to path \"" << faceLevelPath << "\"." << std::endl;
			}
		}
	}
}

/** Compute and export a linearized BRDF look-up table.
 \param outputSide the side size of the 2D output map
 \param outputPath the destination path
 \ingroup BRDFEstimator
 */
void computeAndExportLookupTable(const int outputSide, const std::string & outputPath) {
	// Render the lookup table.
	const Descriptor desc = {Layout::RG32F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	const auto bakingFramebuffer = std::make_shared<Framebuffer>(outputSide, outputSide, desc, false);
	const auto brdfProgram		 = Resources::manager().getProgram2D("brdf_sampler");
	bakingFramebuffer->bind();
	GLUtilities::setViewport(0, 0, int(bakingFramebuffer->width()), int(bakingFramebuffer->height()));
	GLUtilities::clearColor(glm::vec4(0.0f));
	GLUtilities::setDepthState(false);
	brdfProgram->use();
	ScreenQuad::draw();
	bakingFramebuffer->unbind();
	GLUtilities::setDepthState(true);
	GLUtilities::saveFramebuffer(*bakingFramebuffer, uint(outputSide), uint(outputSide), outputPath, true);
}

/**
 Compute either a series of cubemaps convolved with a BRDF using increasing roughness values, irradiance spherical harmonics coefficients, or generate a linearized BRDF look-up table.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup BRDFEstimator
 */
int main(int argc, char ** argv) {
	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/pbrdemo");

	Window window("BRDF Extractor", config);

	// Seed random generator.
	Random::seed();

	ControllableCamera camera;
	camera.projection(config.screenResolution[0] / config.screenResolution[1], glm::pi<float>() * 0.4f, 0.1f, 10.0f);
	camera.pose(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	const auto program					= Resources::manager().getProgram("skybox_basic");
	const auto programSH				= Resources::manager().getProgram("skybox_shcoeffs", "skybox_basic", "skybox_shcoeffs");
	const auto mesh						= Resources::manager().getMesh("skybox", Storage::GPU);
	const Texture * cubemapInfosDefault = Resources::manager().getTexture("debug-cube", {Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

	Texture cubemapInfos;
	std::vector<glm::vec3> SCoeffs(9);
	std::vector<Texture> cubeLevels;

	double timer = System::time();

	// UI parameters.
	int outputSide   = 512;
	int levelsCount  = 6;
	int samplesCount = 32768;
	int showLevel	= 0;
	enum VisualizationMode : int {
		INPUT,
		SH_COEFFS,
		BRDF_CONV
	};
	int mode = INPUT;

	while(window.nextFrame()) {

		// Update camera.
		double currentTime = System::time();
		double frameTime   = currentTime - timer;
		timer			   = currentTime;
		camera.update();
		camera.physics(frameTime);

		// Begin GUI setup.
		if(ImGui::Begin("BRDF extractor")) {

			/// Loading section.
			if(ImGui::Button("Load cubemap...")) {
				std::string cubemapPath;
				if(System::showPicker(System::Picker::Load, "../../../resources/pbrdemo/cubemaps/", cubemapPath, "jpg,bmp,png,tga;exr") && !cubemapPath.empty()) {
					loadCubemap(cubemapPath, cubemapInfos);
					// Reset state.
					SCoeffs.clear();
					SCoeffs.resize(9, glm::vec3(0.0f));
					cubeLevels.clear();
					mode = INPUT;
				}
			}
			ImGui::Separator();

			/// Computations section.
			ImGui::PushItemWidth(172);

			if(ImGui::SliderInt("Map size", &outputSide, 16, 512)) {
				outputSide = std::max(outputSide, 16);
			}

			if(ImGui::InputInt("Roughness levels", &levelsCount)) {
				while(outputSide / std::pow(2, levelsCount) < 4.0f) {
					levelsCount -= 1;
				}
				levelsCount = std::max(2, levelsCount);
			}

			ImGui::InputInt("Samples", &samplesCount);

			// Compute convolution between BRDF and cubemap for a series of roughness.
			if(ImGui::Button("Compute convolved BRDF")) {
				computeCubemapConvolution(cubemapInfos, levelsCount, outputSide, samplesCount, cubeLevels);
				mode = BRDF_CONV;
			}

			// Compute SH irradiance coefficients for the cubemap.
			if(ImGui::Button("Compute SH coefficients")) {
				Probe::extractIrradianceSHCoeffs(cubemapInfos, SCoeffs);
				std::stringstream outputStr;
				for(int i = 0; i < 9; ++i) {
					outputStr << "\t" << SCoeffs[i][0] << " " << SCoeffs[i][1] << " " << SCoeffs[i][2] << std::endl;
				}
				Log::Info() << Log::Utilities << "Coefficients:" << std::endl
							<< outputStr.str() << std::endl;
				programSH->cacheUniformArray("shCoeffs", SCoeffs);
				mode = SH_COEFFS;
			}

			ImGui::PopItemWidth();
			ImGui::Separator();

			/// Export section.
			// Export SH coefficients to text file.
			if(ImGui::Button("Export SH coefficients...")) {
				std::string outputPath;
				if(System::showPicker(System::Picker::Save, ".", outputPath, "txt") && !outputPath.empty()) {
					std::stringstream outputStr;
					for(int i = 0; i < 9; ++i) {
						outputStr << SCoeffs[i][0] << " " << SCoeffs[i][1] << " " << SCoeffs[i][2] << std::endl;
					}
					Resources::saveStringToExternalFile(outputPath, outputStr.str());
				}
			}

			// Export preconvolved cubemaps.
			if(ImGui::Button("Export convolved BRDF maps...")) {
				std::string outputPath;
				if(System::showPicker(System::Picker::Save, ".", outputPath, "exr") && !outputPath.empty()) {
					TextUtilities::removeExtension(outputPath);
					exportCubemapConvolution(cubeLevels, outputPath);
				}
			}

			// Compute and export the two coefficients of the BRDF linear approximation.
			if(ImGui::Button("Export BRDF look-up table...")) {
				std::string outputPath;
				if(System::showPicker(System::Picker::Save, ".", outputPath, "exr") && !outputPath.empty()) {
					TextUtilities::removeExtension(outputPath);
					computeAndExportLookupTable(outputSide, outputPath);
				}
			}
			ImGui::Separator();

			/// Visualisation section.
			ImGui::RadioButton("Input", &mode, INPUT);
			ImGui::SameLine();
			ImGui::RadioButton("Conv. BRDF", &mode, BRDF_CONV);
			ImGui::SameLine();
			ImGui::RadioButton("SH coeffs", &mode, SH_COEFFS);

			if(mode == BRDF_CONV) {
				ImGui::SliderInt("Current level", &showLevel, 0, int(cubeLevels.size()) - 1);
				ImGui::Text("Roughness: %.3f", float(showLevel) / float(cubeLevels.size() - 1));
			}
		}

		ImGui::End();

		/// Rendering.
		const glm::ivec2 screenSize = Input::manager().size();
		const glm::mat4 mvp		   = camera.projection() * camera.view();


		Framebuffer::backbuffer()->bind();
		GLUtilities::setViewport(0, 0, screenSize[0], screenSize[1]);
		GLUtilities::clearColorAndDepth({0.5f, 0.5f, 0.5f, 1.0f}, 1.0f);

		// Render main cubemap.
		if(cubemapInfos.gpu) {
			const auto & programToUse = mode == SH_COEFFS ? programSH : program;
			Texture * texToUse		  = &cubemapInfos;
			if(mode == BRDF_CONV && !cubeLevels.empty()) {
				texToUse = &cubeLevels[showLevel];
			}

			GLUtilities::setDepthState(true);
			programToUse->use();
			GLUtilities::setCullState(false);
			GLUtilities::bindTexture(texToUse, 0);
			programToUse->uniform("mvp", mvp);
			GLUtilities::drawMesh(*mesh);
			GLUtilities::setDepthState(false);
		}

		// Render reference cubemap in the bottom right corner.
		GLUtilities::clearDepth(1.0f);
		const float gizmoScale	   = 0.2f;
		const glm::ivec2 gizmoSize = glm::ivec2(gizmoScale * glm::vec2(screenSize));
		GLUtilities::setViewport(0, 0, gizmoSize[0], gizmoSize[1]);
		GLUtilities::setDepthState(true);
		program->use();
		GLUtilities::setCullState(false);
		GLUtilities::bindTexture(cubemapInfosDefault, 0);
		program->uniform("mvp", mvp);
		GLUtilities::drawMesh(*mesh);
		GLUtilities::setDepthState(false);
		GLUtilities::setViewport(0, 0, screenSize[0], screenSize[1]);
		Framebuffer::backbuffer()->unbind();
		
	}

	// Clean resources.
	Resources::manager().clean();
	window.clean();

	return 0;
}

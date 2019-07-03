#include "Common.hpp"
#include "helpers/Random.hpp"
#include "resources/ImageUtilities.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/ScreenQuad.hpp"
#include "input/Input.hpp"
#include "helpers/Interface.hpp"
#include "helpers/TextUtilities.hpp"
#include "input/ControllableCamera.hpp"


/**
 \defgroup BRDFEstimator BRDF Estimation
 \brief Perform cubemap GGX convolution, irradiance SH coefficients computation, and linearized BRDF look-up table pre-computation.
 \see GLSL::Frag::Cubemap_convo
 \see GLSL::Frag::Brdf_sampler
 \see GLSL::Frag::Skybox_shcoeffs
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
void loadCubemap(const std::string & inputPath, TextureInfos & cubemapInfos){
	std::string cubemapPath = inputPath;
	const std::string ext = TextUtilities::removeExtension(cubemapPath);
	cubemapPath = cubemapPath.substr(0, cubemapPath.size()-3);
	Log::Info() << "Loading " << cubemapPath << "..." << std::endl;
	// Apply the proper format and filtering.
	const GLenum typedFormat = ImageUtilities::isFloat(inputPath) ? GL_RGBA32F : GL_SRGB8_ALPHA8;
	std::vector<std::string> pathSides;
	for(int i = 0; i < 6; ++i){
		pathSides.push_back(cubemapPath + suffixes[i] + ext);
	}
	
	cubemapInfos = GLUtilities::loadTexture(GL_TEXTURE_CUBE_MAP, {pathSides}, {typedFormat, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE}, Storage::BOTH);
	Log::Info() << "Info:" << cubemapInfos.images.size() << std::endl;

}

/**
\brief Decompose an existing cubemap irradiance onto the nine first elements of the spherical harmonic basis.
\details Perform approximated convolution as described in Ramamoorthi, Ravi, and Pat Hanrahan. "An efficient representation for irradiance environment maps.", Proceedings of the 28th annual conference on Computer graphics and interactive techniques. ACM, 2001.
\param cubemap the cubemap to extract SH coefficients from
\return the 9 RGB coefficients of the SH decomposition
\ingroup BRDFEstimator
*/
std::vector<glm::vec3> computeSHCoeffs(const TextureInfos & cubemap){
	// Indices conversions from cubemap UVs to direction.
	const std::vector<int> axisIndices = { 0, 0, 1, 1, 2, 2 };
	const std::vector<float> axisMul = { 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f};

	const std::vector<int> horizIndices = { 2, 2, 0, 0, 0, 0};
	const std::vector<float> horizMul = { -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

	const std::vector<int> vertIndices = { 1, 1, 2, 2, 1, 1};
	const std::vector<float> vertMul = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f};


	// Spherical harmonics coefficients.
	Log::Info() << Log::Utilities << "Computing SH coefficients." << std::endl;
	glm::vec3 LCoeffs[9];
	for(int i = 0; i < 9; ++i){
		LCoeffs[i] = glm::vec3(0.0f,0.0f,0.0f);
	}

	const float y0 = 0.282095f;
	const float y1 = 0.488603f;
	const float y2 = 1.092548f;
	const float y3 = 0.315392f;
	const float y4 = 0.546274f;

	float denom = 0.0f;
	const unsigned int side = cubemap.width;
	
	if(cubemap.width != cubemap.height){
		Log::Error() <<  Log::Utilities << "Expecting squared size cubemap." << std::endl;
		return {};
	}
	
	for(unsigned int i = 0; i < 6; ++i){
		const auto & currentSide = cubemap.images[i];
		for(unsigned int y = 0; y < side; ++y){
			for(unsigned int x = 0; x < side; ++x){
				
				const float v = -1.0f + 1.0f/float(side) + float(y) * 2.0f/float(side);
				const float u = -1.0f + 1.0f/float(side) + float(x) * 2.0f/float(side);
				
				glm::vec3 pos = glm::vec3(0.0f,0.0f,0.0f);
				pos[axisIndices[i]] = axisMul[i];
				pos[horizIndices[i]] = horizMul[i] * u;
				pos[vertIndices[i]] = vertMul[i] * v;
				pos = glm::normalize(pos);
				
				// Normalization factor.
				const float fTmp = 1.0f + u*u + v*v;
				const float weight = 4.0f/(sqrt(fTmp)*fTmp);
				denom += weight;
				
				// HDR color.
				const size_t pixelPos = (y*side+x)*currentSide.components;
				const glm::vec3 hdr = weight * glm::vec3(currentSide.pixels[pixelPos+0],
														 currentSide.pixels[pixelPos+1],
														 currentSide.pixels[pixelPos+2]);
				
				// Y0,0  = 0.282095
				LCoeffs[0] += hdr * y0;
				// Y1,-1 = 0.488603 y
				LCoeffs[1] += hdr * (y1 * pos[1]);
				// Y1,0  = 0.488603 z
				LCoeffs[2] += hdr * (y1 * pos[2]);
				// Y1,1  = 0.488603 x
				LCoeffs[3] += hdr * (y1 * pos[0]);
				// Y2,-2 = 1.092548 xy
				LCoeffs[4] += hdr * (y2 * (pos[0] * pos[1]));
				// Y2,-1 = 1.092548 yz
				LCoeffs[5] += hdr * (y2 * pos[1] * pos[2]);
				// Y2,0  = 0.315392 (3z^2 - 1)
				LCoeffs[6] += hdr * (y3 * (3.0f * pos[2] * pos[2] - 1.0f));
				// Y2,1  = 1.092548 xz
				LCoeffs[7] += hdr * (y2 * pos[0] * pos[2]);
				// Y2,2  = 0.546274 (x^2 - y^2)
				LCoeffs[8] += hdr * (y4 * (pos[0] * pos[0] - pos[1] * pos[1]));
				
			}
		}
	}

	// Normalization.
	for(int i = 0; i < 9; ++i){
		LCoeffs[i] *= 4.0/denom;
	}

	// To go from radiance to irradiance, we need to apply a cosine lobe convolution on the sphere in spatial domain.
	// This can be expressed as a product in frequency (on the SH basis) domain, with constant pre-computed coefficients.
	// See:	Ramamoorthi, Ravi, and Pat Hanrahan. "An efficient representation for irradiance environment maps."
	//		Proceedings of the 28th annual conference on Computer graphics and interactive techniques. ACM, 2001.

	const float c1 = 0.429043f;
	const float c2 = 0.511664f;
	const float c3 = 0.743125f;
	const float c4 = 0.886227f;
	const float c5 = 0.247708f;

	std::vector<glm::vec3> SCoeffs(9);
	SCoeffs[0] = c4 * LCoeffs[0] - c5 * LCoeffs[6];
	SCoeffs[1] = 2.0f * c2 * LCoeffs[1];
	SCoeffs[2] = 2.0f * c2 * LCoeffs[2];
	SCoeffs[3] = 2.0f * c2 * LCoeffs[3];
	SCoeffs[4] = 2.0f * c1 * LCoeffs[4];
	SCoeffs[5] = 2.0f * c1 * LCoeffs[5];
	SCoeffs[6] = c3 * LCoeffs[6];
	SCoeffs[7] = 2.0f * c1 * LCoeffs[7];
	SCoeffs[8] = c1 * LCoeffs[8];

	return SCoeffs;
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
void computeCubemapConvolution(const TextureInfos & cubemapInfos, int levelsCount, int outputSide, int samplesCount, std::vector<TextureInfos> & cubeLevels){
	
	cubeLevels.clear();
	
	// Generate 6 view-projection matrices corresponding to 6 cameras at a 90° angle and with a 90° field of view.
	std::vector<glm::mat4> MVPs(6);
	{
		const glm::mat4 projection = glm::perspective(float(M_PI/2.0), 1.0f, 0.1f, 200.0f);
		const glm::vec3 ups[6] = { glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0) ,glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0)};
		const glm::vec3 centers[6] = { glm::vec3(1.0,0.0,0.0), glm::vec3(-1.0,0.0,0.0), glm::vec3(0.0,1.0,0.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0) };
		for(int i = 0; i < 6; ++i){
			MVPs[i] = projection * glm::lookAt(glm::vec3(0.0f,0.0f,0.0f), centers[i], ups[i]);
		}
	}
	// Create shader program for roughness pre-convolution.
	const auto programCubemap = Resources::manager().getProgram("cubemap_convo", "skybox_basic", "cubemap_convo");
	const auto mesh = Resources::manager().getMesh("skybox");
	
	// Generate convolution map for increments of roughness.
	Log::Info() << Log::Utilities << "Convolving BRDF with cubemap." << std::endl;
	
	for(int level = 0; level < levelsCount; ++level){
		
		const unsigned int w = outputSide / int(std::pow(2, level));
		const unsigned int h = w;
		const float roughness = level / float(levelsCount - 1);
		
		Log::Info() << Log::Utilities << "Level " << level << " (size=" << w << ", r=" << roughness << "): " << std::flush;
		
		// Create cubemap texture for this mipmap level.
		TextureInfos levelInfos;
		levelInfos.cubemap = true;
		levelInfos.mipmap = 1;
		levelInfos.descriptor = {GL_RGB32F, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE};
		levelInfos.width = w;
		levelInfos.height = h;
		levelInfos.id = GLUtilities::createTexture(GL_TEXTURE_CUBE_MAP, levelInfos.descriptor, levelInfos.mipmap);
		
		// Allocate cubemap storage.
		GLenum type, format;
		GLUtilities::getTypeAndFormat(levelInfos.descriptor.typedFormat, type, format);
		glBindTexture(GL_TEXTURE_CUBE_MAP, levelInfos.id);
		for(int i = 0; i < 6; ++i){
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, levelInfos.descriptor.typedFormat,
						 w, h, 0, format, type, NULL);
		}
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		
		// Iterate over faces.
		for(size_t i = 0; i < 6; ++i){
			Log::Info() << "." << std::flush;
			
			// Create local framebuffer.
			const auto resultFramebuffer = std::make_shared<Framebuffer>(w, h, GL_RGB32F, false);
			resultFramebuffer->bind();
			// Use the cubemap texture as a backing texture for the framebuffer.
			glBindTexture(GL_TEXTURE_CUBE_MAP, levelInfos.id);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, levelInfos.id, 0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			
			// Clear texture slice.
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glUseProgram(programCubemap->id());
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glViewport(0,0,w,h);
			// Pass roughness parameters.
			glUniform1f(programCubemap->uniform("mimapRoughness"), roughness);
			glUniformMatrix4fv(programCubemap->uniform("mvp"), 1, GL_FALSE, &MVPs[i][0][0]);
			glUniform1i(programCubemap->uniform("samplesCount"), samplesCount);
			// Attach source cubemap.
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapInfos.id);
			// Draw.
			GLUtilities::drawMesh(*mesh);
			resultFramebuffer->unbind();
			glDisable(GL_DEPTH_TEST);
			// Force synchronization.
			glFlush();
			glFinish();
		}
		// Store the new texture.
		cubeLevels.push_back(levelInfos);
		Log::Info() << std::endl;
	}
	
	glUseProgram(0);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
}

/** Export the pre-convolved cubemap levels.
 \param cubeLevels the textures to export as mipmap levels
 \param outputPath the based destination path
 \ingroup BRDFEstimator
 */
void exportCubemapConvolution(const std::vector<TextureInfos> &cubeLevels, const std::string & outputPath){
	
	for(int level = 0; level < int(cubeLevels.size()); ++level){
		const std::string levelPath = outputPath + "_" + std::to_string(level);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeLevels[level].id);
		GLenum type, format;
		GLUtilities::getTypeAndFormat(cubeLevels[level].descriptor.typedFormat, type, format);
		const int levelSide = cubeLevels[level].width;
		for(int i = 0; i < 6; ++i){
			const std::string faceLevelPath = levelPath + suffixes[i];
			Image image(levelSide, levelSide, 3);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, type, &image.pixels[0]);
			const int ret = ImageUtilities::saveHDRImage(faceLevelPath + ".exr", image, false, true);
			if(ret != 0){
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
void computeAndExportLookupTable(const int outputSide, const std::string & outputPath){
	// Render the lookup table.
	const auto bakingFramebuffer = std::make_shared<Framebuffer>(outputSide, outputSide, GL_RG32F, false);
	const auto brdfProgram = Resources::manager().getProgram2D("brdf_sampler");
	bakingFramebuffer->bind();
	glViewport(0,0,bakingFramebuffer->width(), bakingFramebuffer->height());
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(brdfProgram->id());
	ScreenQuad::draw();
	bakingFramebuffer->unbind();
	glEnable(GL_DEPTH_TEST);
	GLUtilities::saveFramebuffer(*bakingFramebuffer, (unsigned int)outputSide, (unsigned int)outputSide, outputPath, true);
}

/**
 Compute either a series of cubemaps convolved with a BRDF using increasing roughness values, irradiance spherical harmonics coefficients, or generate a linearized BRDF look-up table.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup BRDFEstimator
 */
int main(int argc, char** argv) {
	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv+argc));
	Resources::manager().addResources("../../../resources/pbrdemo");
	
	GLFWwindow* window = Interface::initWindow("BRDF Extractor", config);
	if(!window){
		return -1;
	}
	// Initialize random generator;
	Random::seed();
	
	ControllableCamera camera;
	camera.projection(config.screenResolution[0]/config.screenResolution[1], M_PI*0.4, 0.1f, 10.0f);
	camera.pose(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	const auto program = Resources::manager().getProgram("skybox_basic");
	const auto programSH = Resources::manager().getProgram("skybox_shcoeffs", "skybox_basic", "skybox_shcoeffs");
	const auto mesh = Resources::manager().getMesh("skybox");
	const TextureInfos * cubemapInfosDefault = Resources::manager().getCubemap("debug-cube", {GL_RGB8, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE});
	
	TextureInfos cubemapInfos;
	std::vector<glm::vec3> SCoeffs(9);
	std::vector<TextureInfos> cubeLevels;
	//std::vector<Image> cubemapSides;
	
	double timer = glfwGetTime();
	
	// UI parameters.
	int outputSide = 512;
	int levelsCount = 6;
	int samplesCount = 32768;
	int showLevel = 0;
	enum VisualizationMode : int {
		INPUT, SH_COEFFS, BRDF_CONV
	};
	int mode = INPUT;
	
	while (!glfwWindowShouldClose(window)) {
		// Update events (inputs,...).
		Input::manager().update();
		// Handle quitting/reloading.
		if(Input::manager().pressed(Input::KeyEscape)){
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if(Input::manager().released(Input::KeyP)){
			Resources::manager().reload();
		}
		
		Interface::beginFrame();
		
		// Update camera.
		double currentTime = glfwGetTime();
		double frameTime = currentTime - timer;
		timer = currentTime;
		camera.update();
		camera.physics(frameTime);
		
		// Begin GUI setup.
		if(ImGui::Begin("BRDF extractor")){
			
			/// Loading section.
			if(ImGui::Button("Load cubemap...")){
				std::string cubemapPath;
				if(Interface::showPicker(Interface::Picker::Load, "../../../resources/pbrdemo/cubemaps/", cubemapPath, "jpg,bmp,png,tga;exr") && !cubemapPath.empty()){
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
			
			if(ImGui::SliderInt("Map size", &outputSide, 16, 512)){
				outputSide = std::max(outputSide, 16);
			}
			
			if(ImGui::InputInt("Roughness levels", &levelsCount)){
				while(outputSide / std::pow(2, levelsCount) < 4.0f){
					levelsCount -= 1;
				}
				levelsCount = std::max(2, levelsCount);
			}
			
			ImGui::InputInt("Samples", &samplesCount);
			
			// Compute convolution between BRDF and cubemap for a series of roughness.
			if(ImGui::Button("Compute convolved BRDF")){
				computeCubemapConvolution(cubemapInfos, levelsCount, outputSide, samplesCount, cubeLevels);
				mode = BRDF_CONV;
			}
			
			// Compute SH irradiance coefficients for the cubemap.
			if(ImGui::Button("Compute SH coefficients")){
				SCoeffs = computeSHCoeffs(cubemapInfos);
				std::stringstream outputStr;
				for(int i = 0; i < 9; ++i){
					outputStr << "\t" << SCoeffs[i][0] << " " << SCoeffs[i][1] << " " << SCoeffs[i][2] << std::endl;
				}
				Log::Info() << Log::Utilities << "Coefficients:" << std::endl << outputStr.str() << std::endl;
				programSH->cacheUniformArray("shCoeffs", SCoeffs);
				mode = SH_COEFFS;
			}
			
			ImGui::PopItemWidth();
			ImGui::Separator();
			
			
			/// Export section.
			// Export SH coefficients to text file.
			if(ImGui::Button("Export SH coefficients...")){
				std::string outputPath;
				if(Interface::showPicker(Interface::Picker::Save, ".", outputPath, "txt") && !outputPath.empty()){
					std::stringstream outputStr;
					for(int i = 0; i < 9; ++i){
						outputStr << SCoeffs[i][0] << " " << SCoeffs[i][1] << " " << SCoeffs[i][2] << std::endl;
					}
					const std::string destinationPath = outputPath;
					Resources::saveStringToExternalFile(destinationPath, outputStr.str());
				}
			}
			
			// Export preconvolved cubemaps.
			if(ImGui::Button("Export convolved BRDF maps...")){
				std::string outputPath;
				if(Interface::showPicker(Interface::Picker::Save, ".", outputPath, "exr") && !outputPath.empty()){
					TextUtilities::removeExtension(outputPath);
					exportCubemapConvolution(cubeLevels, outputPath);
				}
			}
			
			// Compute and export the two coefficients of the BRDF linear approximation.
			if(ImGui::Button("Export BRDF look-up table...")){
				std::string outputPath;
				if(Interface::showPicker(Interface::Picker::Save, ".", outputPath, "exr") && !outputPath.empty()){
					TextUtilities::removeExtension(outputPath);
					computeAndExportLookupTable(outputSide, outputPath);
				}
			}
			ImGui::Separator();
			
			
			/// Visualisation section.
			ImGui::RadioButton("Input", &mode, INPUT); ImGui::SameLine();
			ImGui::RadioButton("Conv. BRDF", &mode, BRDF_CONV); ImGui::SameLine();
			ImGui::RadioButton("SH coeffs", &mode, SH_COEFFS);
			
			if(mode == BRDF_CONV){
				ImGui::SliderInt("Current level", &showLevel, 0, cubeLevels.size()-1);
				ImGui::Text("Roughness: %.3f", showLevel / float(cubeLevels.size() - 1));
			}
		}
		
		ImGui::End();
		
		/// Rendering.
		const glm::vec2 screenSize = Input::manager().size();
		const glm::mat4 mvp = camera.projection() * camera.view();
		
		glClearColor(0.5, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0, screenSize[0], screenSize[1]);
		
		// Render main cubemap.
		if(cubemapInfos.id > 0){
			const auto & programToUse = mode == SH_COEFFS ? programSH : program;
			glEnable(GL_DEPTH_TEST);
			glUseProgram(programToUse->id());
			glDisable(GL_CULL_FACE);
			glActiveTexture(GL_TEXTURE0);
			if(mode == BRDF_CONV && !cubeLevels.empty()){
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubeLevels[showLevel].id);
			} else {
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapInfos.id);
			}
			glUniformMatrix4fv(programToUse->uniform("mvp"), 1, GL_FALSE,  &mvp[0][0]);
			GLUtilities::drawMesh(*mesh);
			glUseProgram(0);
			glDisable(GL_DEPTH_TEST);
		}
		
		// Render reference cubemap in the bottom right corner.
		glClear(GL_DEPTH_BUFFER_BIT);
		const float gizmoScale = 0.2f;
		const glm::vec2 gizmoSize = gizmoScale * screenSize;
		glViewport(0.0f, 0.0f, gizmoSize[0], gizmoSize[1]);
		glEnable(GL_DEPTH_TEST);
		glUseProgram(program->id());
		glDisable(GL_CULL_FACE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapInfosDefault->id);
		glUniformMatrix4fv(program->uniform("mvp"), 1, GL_FALSE,  &mvp[0][0]);
		GLUtilities::drawMesh(*mesh);
		glUseProgram(0);
		glDisable(GL_DEPTH_TEST);
		glViewport(0,0, screenSize[0], screenSize[1]);
		
		Interface::endFrame();
		checkGLError();
		glfwSwapBuffers(window);
	}
	
	Interface::clean();
	// Clean resources.
	Resources::manager().clean();
	// Close GL context and any other GLFW resources.
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}

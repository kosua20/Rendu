#include "ShaderEditor.hpp"
#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/ShaderCompiler.hpp"
#include "generation/Random.hpp"
#include "generation/PerlinNoise.hpp"
#include "system/TextUtilities.hpp"
#include "resources/Texture.hpp"
#include "resources/ResourcesManager.hpp"

const uint kShaderEditorVersionMajor = 1;
const uint kShaderEditorVersionMinor = 0;
const uint kShaderEditorVersionFixes = 1;

const float kPerlinNoiseScale = 0.25f;

const std::string kFlagName = "flag";
const std::string kIntName = "int";
const std::string kFloatName = "float";
const std::string kVecName = "vect";
const std::string kColorName = "col";
const std::string kHelpMessage = "Reload: Enter or Ctrl/Cmd+B\nReload and reset values: Shift+Enter or Ctrl/Cmd+Shift+B\nPlay/pause: Space\nShow panel: Tab\nCtrl/Cmd+1: horizontal layout\nCtrl/Cmd+2: vertical layout\nCtrl/Cmd+3: freeform layout\nCtrl/Cmd+F: display render in sub-window";

ShaderEditor::ShaderEditor(RenderingConfig & config) : CameraApp(config), _noise("Uniform 2D"), _perlin("Perlin 2D"), _directions("Directions"), _noise3D("Uniform 3D"), _perlin3D("Perlin 3D") {
	// Setup render buffer.
	const glm::uvec2 res(_config.renderingResolution());

	_currFrame.reset(new Framebuffer(TextureShape::D2, res[0], res[1], 1, 1, {Layout::RGBA16F}, "Current frame"));

	_prevFrame.reset(new Framebuffer(TextureShape::D2, res[0], res[1], 1, 1, {Layout::RGBA16F}, "Previous frame"));

	// We don't want the resources manager to alter the program.
	const std::string vShader = Resources::manager().getStringWithIncludes("shaderbench.vert");
	const std::string fShader = Resources::manager().getStringWithIncludes("shaderbench.frag");
	_currProgram.reset(new Program("User program", vShader, fShader));

	_shaderPath = "";
	_shaderName = "(default)";
	_passthrough = Resources::manager().getProgram2D("passthrough");

	_userCamera.ratio(config.screenResolution[0] / config.screenResolution[1]);
	_startTime = System::time();

	_fallbackTex = Resources::manager().getTexture("non-2d-texture", Layout::RGBA8, Storage::GPU);

	// Noise texture.
	{
		_noise.width = _noise.height = 512;
		_noise.depth = _noise.levels = 1;
		_noise.shape = TextureShape::D2;
		_noise.images.emplace_back(_noise.width, _noise.height, 4);
		Image & noiseImg = _noise.images[0];
		System::forParallel(0, size_t(noiseImg.height), [&noiseImg](size_t y){
			for(uint x = 0; x < noiseImg.width; ++x){
				noiseImg.rgba(int(x), int(y)) = glm::vec4(Random::Float(), Random::Float(), Random::Float(), Random::Float());
			}
		});
		_noise.upload(Layout::RGBA32F, false);
	}
	// Perlin noise texture.
	{
		_perlin.width = _perlin.height = 1024;
		_perlin.depth = _perlin.levels = 1;
		_perlin.shape = TextureShape::D2;
		_perlin.images.emplace_back(_perlin.width, _perlin.height, 4);
		Image& img = _perlin.images[0];

		PerlinNoise perlinGen;
		for(uint cid = 0; cid < img.components; ++cid)
		{
			// Large offset to ensure different values in the different channels.
			const glm::vec3 offset(float(_perlin.width * cid));
			// Scale to have multiple octaves available.
			const uint scaleDenom = cid + 2;
			const float scale = kPerlinNoiseScale / float(scaleDenom * scaleDenom);
			perlinGen.generatePeriodic(img, cid, scale, 0.0f, offset);
		}

		System::forParallel(0, size_t(img.height), [&img](size_t y){
			for(uint x = 0; x < img.width; ++x){
				img.rgba(int(x), int(y)) = 0.5f * img.rgba(int(x), int(y)) + 0.5f;
			}
		});
		_perlin.upload(Layout::RGBA32F, false);
	}

	// Random directions texture.
	{
		_directions.width = _directions.height = 64;
		_directions.depth = _directions.levels = 1;
		_directions.shape = TextureShape::D2;
		_directions.images.emplace_back(_directions.width, _directions.height, 4);
		Image & dirImg = _directions.images[0];
		System::forParallel(0, size_t(dirImg.height), [&dirImg](size_t y){
			for(uint x = 0; x < dirImg.width; ++x){
				dirImg.rgb(int(x), int(y)) = glm::normalize(Random::sampleSphere());
			}
		});
		_directions.upload(Layout::RGBA32F, false);
	}
	{
		_noise3D.width = _noise3D.height = _noise3D.depth = 256;
		_noise3D.levels = 1;
		_noise3D.shape = TextureShape::D3;

		for(uint d = 0; d < _noise3D.depth; ++d){
			_noise3D.images.emplace_back(_noise3D.width, _noise3D.height, 4);
			auto & img = _noise3D.images[d];
			System::forParallel(0, size_t(img.height), [&img](size_t y){
				for(uint x = 0; x < img.width; ++x){
					img.rgba(int(x), int(y)) = glm::vec4(Random::Float(), Random::Float(), Random::Float(), Random::Float());
				}
			});
		}
		_noise3D.upload(Layout::RGBA32F, false);
	}

	{
		_perlin3D.width = _perlin3D.height = _perlin3D.depth = 128;
		_perlin3D.levels = 1;
		_perlin3D.shape = TextureShape::D3;
		PerlinNoise perlinGen;
		for(uint d = 0; d < _perlin3D.depth; ++d){
			_perlin3D.images.emplace_back(_perlin3D.width, _perlin3D.height, 4);
			auto & img = _perlin3D.images[d];

			for(uint cid = 0; cid < img.components; ++cid)
			{
				// Large offset to ensure different values in the different channels.
				// along with different values from the 2D Perlin noise.
				const float baseOffset = float(_perlin.width * 10 + cid * _perlin3D.width);
				const glm::vec3 offset(baseOffset);
				// Scale to have multiple octaves available.
				const uint scaleDenom = cid + 1;
				const float scale = kPerlinNoiseScale / float(scaleDenom * scaleDenom);
				perlinGen.generatePeriodic(img, cid, scale, float(d), offset);
			}

			System::forParallel(0, size_t(img.height), [&img](size_t y){
				for(uint x = 0; x < img.width; ++x){
					img.rgba(int(x), int(y)) = 0.5f * img.rgba(int(x), int(y)) + 0.5f;
				}
			});

		}
		_perlin3D.upload(Layout::RGBA32F, false);
	}

	// Reference textures.
	_textures.push_back(_prevFrame->texture());
	_textures.push_back(Resources::manager().getTexture("shadertoy-font", Layout::RGBA8, Storage::GPU));
	_textures.push_back(Resources::manager().getTexture("debug-grid", Layout::SRGB8_ALPHA8, Storage::GPU));
	_textures.push_back(&_noise);
	_textures.push_back(&_perlin);
	_textures.push_back(&_directions);
	_textures.push_back(&_noise3D);
	_textures.push_back(&_perlin3D);

	// Set all values for the default shader.
	{
		_currProgram->uniform("gamma", 2.2f);
		_currProgram->uniform("specExponent", 128.0f);
		_currProgram->uniform("radius", 0.5f);
		_currProgram->uniform("epsilon", 0.001f);
		_currProgram->uniform("skyBottom", glm::vec3(0.001f, 0.008f, 0.025f));
		_currProgram->uniform("skyLight", glm::vec3(0.0f, 0.064f, 0.427f));
		_currProgram->uniform("skyTop", glm::vec3(0.0f, 0.463f, 1.0f));
		_currProgram->uniform("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
		_currProgram->uniform("sphereColor", glm::vec3(0.865f, 0.303f, 0.0f));
		_currProgram->uniform("ground0", glm::vec3(0.0f, 0.15f, 0.22f));
		_currProgram->uniform("ground1", glm::vec3(0.015f, 0.213f, 0.28f));
		_currProgram->uniform("lightDirection", glm::vec4(-1.8f, 1.6f, 1.7f, 0.0f));
		_currProgram->uniform("stepCount", 128);
		_currProgram->uniform("showPlane", true);
	}
	// If the default shader uses more default uniforms, set them up, and restore all values
	// so that we have something interesting to show at load time.
	restoreUniforms();
}

void ShaderEditor::draw() {

	// Precompute values for all internal uniforms.
	const glm::mat4 & view = _userCamera.view();
	const glm::mat4 & proj = _userCamera.projection();
	const glm::mat4 viewproj = proj * view;
	const glm::mat4 viewInv = glm::inverse(view);
	const glm::mat4 projInv = glm::inverse(proj);
	const glm::mat4 viewprojInv = glm::inverse(viewproj);
	const glm::mat4 normalMat = glm::transpose(viewInv);
	const glm::vec3 & camPos = _userCamera.position();
	const glm::vec3 & camUp = _userCamera.up();
	const glm::vec3 & camCenter = _userCamera.center();
	const float fov = _userCamera.fov();
	const glm::vec3 screenSize(_currFrame->width(), _currFrame->height(), 0.0f);

	// Update timing.
	float deltaTime = 0.0f;
	if(!_paused){
		// If not paused, move forward along the timeline.
		const double timeFull = System::time();
		const double localTime = timeFull - _startTime;
		deltaTime = float(localTime - _currentTime);
		_currentTime = localTime;
		++_frame;
	}
	// Mouse buttons state and location.
	glm::vec4 mouseState(0.0f);
	if(Input::manager().pressed(Input::Mouse::Left)){
		const glm::vec2 mpos = Input::manager().mouse();
		mouseState[0] = mpos[0];
		mouseState[1] = mpos[1];
	}
	mouseState[2] = float(Input::manager().triggered(Input::Mouse::Left));
	mouseState[3] = float(Input::manager().triggered(Input::Mouse::Right));

	// Clear content.
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	_currFrame->bind(glm::vec4(0.0f), 1.0f);
	_currFrame->setViewport();
	_currProgram->use();

	// Predefined uniforms.

	_currProgram->uniform("iTime", float(_currentTime));
	_currProgram->uniform("iTimeDelta", deltaTime);
	_currProgram->uniform("iFrame", float(_frame));
	_currProgram->uniform("iResolution", screenSize);
	_currProgram->uniform("iMouse", mouseState);

	_currProgram->uniform("iView", view);
	_currProgram->uniform("iProj", proj);
	_currProgram->uniform("iViewProj", viewproj);
	_currProgram->uniform("iViewInv", viewInv);
	_currProgram->uniform("iProjInv", projInv);
	_currProgram->uniform("iViewProjInv", viewprojInv);
	_currProgram->uniform("iNormalMat", normalMat);

	_currProgram->uniform("iCamPos", camPos);
	_currProgram->uniform("iCamUp", camUp);
	_currProgram->uniform("iCamCenter", camCenter);
	_currProgram->uniform("iCamFov", fov);

	// User defined uniforms.

	for(size_t i = 0; i < _flags.size(); ++i){
		_currProgram->uniform(_flags[i].name, _flags[i].value);
	}
	for(size_t i = 0; i < _integers.size(); ++i){
		_currProgram->uniform(_integers[i].name, _integers[i].value);
	}
	for(size_t i = 0; i < _floats.size(); ++i){
		_currProgram->uniform(_floats[i].name, _floats[i].value);
	}
	for(size_t i = 0; i < _vectors.size(); ++i){
		_currProgram->uniform(_vectors[i].name, _vectors[i].value);
	}
	for(size_t i = 0; i < _colors.size(); ++i){
		_currProgram->uniform(_colors[i].name, _colors[i].value);
	}

	// First texture is the prevous frame.
	_currProgram->texture(_prevFrame->texture(), 0);
	for(size_t i = 1; i < _textures.size(); ++i){
		_currProgram->texture(_textures[i], uint(i));
	}

	// Render user shader and time it.
	_timer.begin();
	ScreenQuad::draw();
	_timer.end();

	Swapchain::backbuffer()->bind(glm::vec4(0.3f,0.3f,0.3f, 1.0f));
	GPU::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));

	// If not in window mode, directly blit to the screne.
	if(!_windowed){
		_passthrough->use();
		_passthrough->texture(_currFrame->texture(), 0);
		ScreenQuad::draw();
	}

	std::swap(_currFrame, _prevFrame);
	_textures[0] = _prevFrame->texture();
}

void ShaderEditor::update() {

	CameraApp::update();

	handleShortcuts();

	// Compute GUI panel sizes.
	const float dens = Input::manager().density();
	const glm::ivec2 adjustedRes = _config.screenResolution/dens;
	const uint panelWidth = 410;
	const uint panelHeight = 300;
	const ImGuiCond updateMode = _layout == LayoutMode::FREEFORM ? ImGuiCond_FirstUseEver : ImGuiCond_Always;
	glm::vec2 panelPos(0.0f);
	glm::vec2 panelSize(0.0f);
	glm::vec2 windowPos(0.0f);
	glm::vec2 windowSize = adjustedRes;
	if(_layout == LayoutMode::VERTICAL){
		panelSize = glm::vec2(adjustedRes[0], panelHeight);
		panelPos = glm::vec2(0.0f, adjustedRes[1] - panelHeight);
		windowSize = glm::vec2(adjustedRes[0], adjustedRes[1] - panelHeight);
		windowPos = glm::vec2(0.0f);
	} else if(_layout == LayoutMode::HORIZONTAL){
		panelSize = glm::vec2(panelWidth, adjustedRes[1]);
		panelPos = glm::vec2(0.0f, 0.0f);
		windowSize = glm::vec2(adjustedRes[0] - panelWidth, adjustedRes[1]);
		windowPos = glm::vec2(panelWidth, 0.0f);
	}
	// Always put the log at the bottom right of the rendering window.
	const glm::vec2 logPos = windowPos + windowSize;

	// Display the rendering texture in a resizable sub window.
	if(_windowed){
		ImGui::SetNextWindowPos(windowPos, updateMode);
		ImGui::SetNextWindowSize(windowSize, updateMode);
		if(ImGui::Begin("Render", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus)){
			// Adjust the texture display to the window size.
			const ImVec2 winSize = ImGui::GetContentRegionAvail();
			ImGui::ImageButton("#Tex", *_currFrame->texture(), ImVec2(winSize.x, winSize.y), ImVec2(0.0,0.0), ImVec2(1.0,1.0));
			if (ImGui::IsItemHovered()) {
				ImGui::SetNextFrameWantCaptureMouse(false);
				ImGui::SetNextFrameWantCaptureKeyboard(false);
			}

			// If the aspect ratio changed, trigger a resize.
			const float ratioCurr = float(_currFrame->width()) / float(_currFrame->height());
			const float ratioWin = winSize.x / winSize.y;
			// \todo Derive a more robust threshold.
			if(std::abs(ratioWin - ratioCurr) > 0.01f){
				const glm::vec2 renderRes = float(_config.internalVerticalResolution) / float(winSize.y) * glm::vec2(winSize.x, winSize.y);
				_currFrame->resize(renderRes);
				_prevFrame->resize(renderRes);
				_userCamera.ratio(renderRes[0]/renderRes[1]);
			}
		}
		ImGui::End();
	}

	// Show the fixed log window only if there is an error message.
	if(!_compilationLog.empty()){
		ImGui::SetNextWindowPos(logPos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
		if(ImGui::Begin("Log", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_Modal)){
			ImGui::TextColored(ImVec4(0.9f, 0.0f, 0.0f, 1.0f), "Compilation failed, displaying last valid version.");
			ImGui::Text("%s", _compilationLog.c_str());
		}
		ImGui::End();
	}

	// On my macOS dev machine, fetching a query seems to improve performances
	// drastically, maybe marking the program as prioritary. So always fetch this query value.
	const uint frameTime = uint(_timer.value());

	// Don't display the panel if required.
	if(!_showGUI){
		return;
	}

	// Settings window.
	ImGui::SetNextWindowPos(panelPos, updateMode);
	ImGui::SetNextWindowSize(panelSize, updateMode);
	if(ImGui::Begin("Shader editor", &_showGUI)) {
		// Adjust the number of columns based on panel size.
		// Each small column is 100px, the large columns 200.
		const uint widthPix = uint(ImGui::GetWindowSize().x);
		const uint columnsCount = widthPix/100;

		// Shader load/image save.
		ImGui::Text("Shader: %s", _shaderName.c_str());
		if(ImGui::IsItemHovered()){
			ImGui::SetTooltip("%s", _shaderPath.c_str());
		}
		if(ImGui::Button("Load shader...")){
			std::string outPath;
			if(System::showPicker(System::Picker::Load, "", outPath) && !outPath.empty()){
				loadShader(outPath);
			}
		}
		ImGui::SameLine();
		if(ImGui::Button("Save image...")){
			std::string outPath;
			if(System::showPicker(System::Picker::Save, "", outPath, "png") && !outPath.empty()){
				TextUtilities::splitExtension(outPath);
				// Create a RGB8 framebuffer to save as png.
				Framebuffer tmp(_currFrame->width(), _currFrame->height(), Layout::RGBA8, "Temp");
				GPU::blit(*_currFrame, tmp, Filter::NEAREST);
				GPU::saveFramebuffer(tmp, outPath, Image::Save::IGNORE_ALPHA);
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Version %d.%d.%d (?)", kShaderEditorVersionMajor, kShaderEditorVersionMinor, kShaderEditorVersionFixes);
		if(ImGui::IsItemHovered()){
			ImGui::SetTooltip("%s", kHelpMessage.c_str());
		}

		// Rendering settings.
		if(ImGui::CollapsingHeader("Settings")){

			// Reorganize the GUI panels.
			ImGui::Text("Layout: ");
			ImGui::RadioButton("Horizontal", (int*)&_layout, int(LayoutMode::HORIZONTAL));
			ImGui::SameLine();
			ImGui::RadioButton("Vertical", (int*)&_layout, int(LayoutMode::VERTICAL));
			ImGui::SameLine();
			ImGui::RadioButton("Freeform", (int*)&_layout, int(LayoutMode::FREEFORM));
			ImGui::SameLine();
			// Display result in a subwindow.
			if(ImGui::Checkbox("Windowed", &_windowed)){
				if(!_windowed){
					_userCamera.ratio(_config.screenResolution[0] / _config.screenResolution[1]);
					resize();
				}
			}

			// Rendering info.
			ImGui::Text("Frame time: %5.3fms, resolution: %dx%d", float(frameTime)/1000000.0f, _currFrame->width(), _currFrame->height());

			// Play/pause/reset options and timing info.
			if(ImGui::Button("Pause##time")){
				_paused = !_paused;
				_startTime = System::time() - _currentTime;
			}
			ImGui::SameLine();
			if(ImGui::Button("Reset##time")){
				_startTime = System::time();
				_currentTime = 0.0;
				_frame = 0;
			}
			ImGui::SameLine();
			ImGui::Text("Time: %6.1fs   Frame: %d", float(_currentTime), _frame);
			// Custom internal render height.
			ImGui::PushItemWidth(94);
			if(ImGui::InputInt("Render height", &_config.internalVerticalResolution, 50, 200)) {
				_config.internalVerticalResolution = std::max(8, _config.internalVerticalResolution);
				resize();
			}
			ImGui::PopItemWidth();
		}

		// The big chunk: display all exposed uniforms.
		if(ImGui::CollapsingHeader("Uniforms")){
			// Copy uniforms for new shaders.
			if(ImGui::Button("Copy uniforms")){
				std::string res = generateParametersString("", false);
				// Brace yourself.
				std::stringstream uniformStr;
				uniformStr << "float iTime;\n" << "float iTimeDelta;\n" << "float iFrame;\n";
				uniformStr << "vec3 iResolution;\n" << "vec4 iMouse;\n" << "mat4 iView;\n";
				uniformStr << "mat4 iProj;\n" << "mat4 iViewProj;\n" << "mat4 iViewInv;\n";
				uniformStr << "mat4 iProjInv;\n" << "mat4 iViewProjInv;\n" << "mat4 iNormalMat;\n";
				uniformStr << "vec3 iCamPos;\n" << "vec3 iCamUp;\n" << "vec3 iCamCenter;\n";
				uniformStr << "float iCamFov;\n";
				res.append(uniformStr.str());
				ImGui::SetClipboardText(res.c_str());
			}
			ImGui::SameLine();
			// Copy currently set values for final shader export.
			if(ImGui::Button("Copy current values")){
				// Here we don't copy the internal parameters.
				const std::string res = generateParametersString("", true);
				ImGui::SetClipboardText(res.c_str());
			}

			// Editable uniforms lists.
			displayUniforms(columnsCount);
		}

		// Display textures (not modifiable)
		if(ImGui::CollapsingHeader("Textures")){
			ImGui::Columns((columnsCount*3)/4);
			for(uint i = 0; i < _textures.size(); ++i){
				// Small square display.
				ImGui::Text("%d: %s", i, _textures[i]->name().c_str());
				if(_textures[i]->shape == TextureShape::D2){
					ImGui::Image(*_textures[i], ImVec2(100,100));
				} else {
					ImGui::Image(*_fallbackTex, ImVec2(100,100));
				}
				ImGui::NextColumn();
			}
			ImGui::Columns();
		}

		// Camera settings.
		if(ImGui::CollapsingHeader("Camera settings")) {
			_userCamera.interface();
			if(ImGui::Button("Reset##cameraoptions")) {
				_userCamera.reset();
			}
		}

	}
	ImGui::End();
}

void ShaderEditor::handleShortcuts(){
	// Reload shader.
	const bool ctrlPressed = Input::manager().pressed(Input::Key::LeftSuper) || Input::manager().pressed(Input::Key::RightSuper) || Input::manager().pressed(Input::Key::LeftControl) || Input::manager().pressed(Input::Key::RightControl);
	const bool shiftPressed = Input::manager().pressed(Input::Key::LeftShift) || Input::manager().pressed(Input::Key::RightShift);

	if(Input::manager().triggered(Input::Key::Enter)
	   || (ctrlPressed && Input::manager().triggered(Input::Key::B))){
		if(!_shaderPath.empty()){
			// Reload the current shader.
			_compilationLog = reload(_shaderPath, shiftPressed);
		}
	}
	// Play/pause.
	if(Input::manager().triggered(Input::Key::Space)){
		_paused = !_paused;
		_startTime = System::time() - _currentTime;
	}
	// Hide the GUI panel.
	if(Input::manager().triggered(Input::Key::Tab)){
		_showGUI = !_showGUI;
	}
	// Layout mode.
	if(ctrlPressed && Input::manager().triggered(Input::Key::N1)){
		_layout = LayoutMode::HORIZONTAL;
	}
	if(ctrlPressed && Input::manager().triggered(Input::Key::N2)){
		_layout = LayoutMode::VERTICAL;
	}
	if(ctrlPressed && Input::manager().triggered(Input::Key::N3)){
		_layout = LayoutMode::FREEFORM;
	}
	// Window mode.
	if(ctrlPressed && Input::manager().triggered(Input::Key::F)){
		_windowed = !_windowed;
		if(!_windowed){
			_userCamera.ratio(_config.screenResolution[0] / _config.screenResolution[1]);
			resize();
		}
	}
}

void ShaderEditor::displayUniforms(uint columnsCount){
	// Boolean parameters list.
	if(ImGui::TreeNode("Flags")){
		ImGui::Columns(columnsCount);
		for(size_t i = 0; i < _flags.size(); ++i){
			ImGui::Checkbox(_flags[i].name.c_str(), &_flags[i].value);
			ImGui::NextColumn();
		}
		// Add/remove buttons.
		if(ImGui::Button(" + ##flag")){
			_flags.emplace_back();
			_flags.back().name = kFlagName + std::to_string(_flags.size()-1);
		}
		ImGui::SameLine();
		if(ImGui::Button(" - ##flag")){
			_flags.pop_back();
		}
		ImGui::Columns();
		ImGui::TreePop();
	}

	// Integer parameters list.
	if(ImGui::TreeNode("Integers")){
		// Larger columns, for the stepper buttons.
		ImGui::Columns(columnsCount/2);
		for(size_t i = 0; i < _integers.size(); ++i){
			ImGui::InputInt(_integers[i].name.c_str(), &_integers[i].value);
			ImGui::NextColumn();
		}
		// Add/remove buttons.
		if(ImGui::Button(" + ##int")){
			_integers.emplace_back();
			_integers.back().name = kIntName + std::to_string(_integers.size()-1);
		}
		ImGui::SameLine();
		if(ImGui::Button(" - ##int")){
			_integers.pop_back();
		}
		ImGui::Columns();
		ImGui::TreePop();
	}

	// Float parameters list.
	if(ImGui::TreeNode("Scalars")){
		for(size_t i = 0; i < _floats.size(); ++i){
			// Display a slider, and fields to set the min/max values.
			ImGui::PushID(int(i));
			ImGui::PushItemWidth(160);
			ImGui::SliderFloat(_floats[i].name.c_str(), &_floats[i].value, _floats[i].min, _floats[i].max); ImGui::SameLine();
			ImGui::PopItemWidth();
			ImGui::PushItemWidth(40);
			ImGui::InputFloat("Min", &_floats[i].min); ImGui::SameLine();
			ImGui::InputFloat("Max", &_floats[i].max);
			ImGui::PopItemWidth();
			ImGui::PopID();
		}
		// Add/remove buttons.
		if(ImGui::Button(" + ##float")){
			_floats.emplace_back();
			_floats.back().name = kFloatName + std::to_string(_floats.size()-1);
		}
		ImGui::SameLine();
		if(ImGui::Button(" - ##float")){
			_floats.pop_back();
		}
		ImGui::TreePop();
	}

	// Vector parameters list.
	if(ImGui::TreeNode("Vectors")){
		for(size_t i = 0; i < _vectors.size(); ++i){
			ImGui::DragFloat4(_vectors[i].name.c_str(), &_vectors[i].value[0], 0.1f);
		}
		// Add/remove buttons.
		if(ImGui::Button(" + ##vector")){
			_vectors.emplace_back();
			_vectors.back().name = kVecName + std::to_string(_vectors.size()-1);
		}
		ImGui::SameLine();
		if(ImGui::Button(" - ##vector")){
			_vectors.pop_back();
		}
		ImGui::TreePop();
	}

	// Color parameters list.
	if(ImGui::TreeNode("Colors")){
		ImGui::Columns(columnsCount);
		// Display a basic picker. Maybe allow HDR values?
		for(size_t i = 0; i < _colors.size(); ++i){
			ImGui::ColorEdit3(_colors[i].name.c_str(), &_colors[i].value[0], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_HDR);
			ImGui::NextColumn();
		}
		// Add/remove buttons.
		if(ImGui::Button(" + ##color")){
			_colors.emplace_back();
			_colors.back().name = kColorName + std::to_string(_colors.size()-1);
		}
		ImGui::SameLine();
		if(ImGui::Button(" - ##color")){
			_colors.pop_back();
		}
		ImGui::Columns();
		ImGui::TreePop();
	}
}

std::string ShaderEditor::reload(const std::string & shaderPath, bool syncUniforms){
	// Reload from disk.
	const std::string vShader = Resources::manager().getStringWithIncludes("shaderbench.vert");
	std::string fShader = Resources::loadStringFromExternalFile(shaderPath);
	TextUtilities::replace(fShader, "#version", "#define UNUSED_VERSION_INDICATOR_GPU_SHADER_LANGUAGE");
	// Before updating the program, try to compile the fragment shader and abort if there is some error.
	Program::Stage stage;
	std::string log;
	ShaderCompiler::compile(fShader, ShaderType::FRAGMENT, stage, true, log);
	ShaderCompiler::clean(stage);
	if(!log.empty()){
		return log;
	}
	_currProgram->reload(vShader, fShader);
	if(syncUniforms){
		restoreUniforms();
	}
	return log;
}

ShaderEditor::~ShaderEditor() {
	_currProgram->clean();

	_noise.clean();
	_perlin.clean();
	_directions.clean();
	_noise3D.clean();
	_perlin3D.clean();
}

void ShaderEditor::loadShader(const std::string & path){
	_shaderPath = path;
	_shaderName = TextUtilities::extractFilename(_shaderPath);
	TextUtilities::splitExtension(_shaderName);
	_compilationLog = reload(_shaderPath, true);
}

void ShaderEditor::resize() {
	// Same aspect ratio as the display resolution
	const glm::vec2 renderRes = _config.renderingResolution();
	// Only resize if we are not in window mode (else handled when displaying the window).
	if(!_windowed){
		_currFrame->resize(renderRes);
		_prevFrame->resize(renderRes);
	}
}

void ShaderEditor::restoreUniforms(){
	_flags.clear();
	_integers.clear();
	_floats.clear();
	_vectors.clear();
	_colors.clear();

	const std::vector<std::string> defaultNames = {"iTime", "iTimeDelta", "iFrame", "iResolution", "iMouse", "iCamPos", "iCamUp", "iCamCenter", "iCamFov"};

	const Program::Uniforms & uniforms = _currProgram->uniforms();
	for(const auto & def : uniforms){
		const Program::UniformDef& uniform = def.second;

		// Skip predefined uniforms.
		if(std::find(defaultNames.begin(), defaultNames.end(), uniform.name) != defaultNames.end()){
			continue;
		}
		
		switch (uniform.type) {
			case Program::UniformDef::Type::BOOL:
				_flags.emplace_back();
				_flags.back().name = uniform.name;
				_currProgram->getUniform(uniform.name, _flags.back().value);
				break;
			case Program::UniformDef::Type::INT:
				_integers.emplace_back();
				_integers.back().name = uniform.name;
				_currProgram->getUniform(uniform.name, _integers.back().value);
				break;
			case Program::UniformDef::Type::FLOAT:
				_floats.emplace_back();
				_floats.back().name = uniform.name;
				_currProgram->getUniform(uniform.name, _floats.back().value);
				_floats.back().min = 0.5f * _floats.back().value;
				_floats.back().max = _floats.back().value != 0.0f ? (2.0f * _floats.back().value) : 1.0f;
				break;
			case Program::UniformDef::Type::VEC3:
				_colors.emplace_back();
				_colors.back().name = uniform.name;
				_currProgram->getUniform(uniform.name, _colors.back().value);
				break;
			case Program::UniformDef::Type::VEC4:
				_vectors.emplace_back();
				_vectors.back().name = uniform.name;
				_currProgram->getUniform(uniform.name, _vectors.back().value);
				break;
			default:
				break;
		}
	}
	std::sort(_flags.begin(), _flags.end(), [](const BoolOption & a, const BoolOption & b){
		return a.name < b.name;
	});
	std::sort(_integers.begin(), _integers.end(), [](const IntOption & a, const IntOption & b){
		return a.name < b.name;
	});
	std::sort(_floats.begin(), _floats.end(), [](const FloatOption & a, const FloatOption & b){
		return a.name < b.name;
	});
	std::sort(_vectors.begin(), _vectors.end(), [](const VecOption & a, const VecOption & b){
		return a.name < b.name;
	});
	std::sort(_colors.begin(), _colors.end(), [](const ColorOption & a, const ColorOption & b){
		return a.name < b.name;
	});
}

std::string ShaderEditor::generateParametersString(const std::string & prefix, bool exportValues){
	std::stringstream uniformStr;
	for(size_t i = 0; i < _flags.size(); ++i){
		uniformStr << prefix << "bool " << _flags[i].name;
		if(exportValues){
			uniformStr << " = " << (_flags[i].value ? "true" : "false");
		}
		uniformStr << ";\n";
	}
	for(size_t i = 0; i < _integers.size(); ++i){
		uniformStr << prefix << "int " << _integers[i].name;
		if(exportValues){
			uniformStr << " = " << _integers[i].value;
		}
		uniformStr << ";\n";
	}
	for(size_t i = 0; i < _floats.size(); ++i){
		uniformStr << prefix << "float " << _floats[i].name;
		if(exportValues){
			uniformStr << " = " << _floats[i].value;
		}
		uniformStr << ";\n";
	}
	for(size_t i = 0; i < _vectors.size(); ++i){
		uniformStr << prefix << "vec4 " << _vectors[i].name;
		if(exportValues){
			uniformStr << " = vec4(" << _vectors[i].value[0] << ", " << _vectors[i].value[1] << ", " << _vectors[i].value[2] << ", " << _vectors[i].value[3] << ")";
		}
		uniformStr << ";\n";
	}
	for(size_t i = 0; i < _colors.size(); ++i){
		uniformStr << prefix << "vec3 " << _colors[i].name;
		if(exportValues){
			uniformStr << " = vec3(" << _colors[i].value[0] << ", " << _colors[i].value[1] << ", " << _colors[i].value[2] << ")";
		}
		uniformStr << ";\n";
	}
	return uniformStr.str();
}

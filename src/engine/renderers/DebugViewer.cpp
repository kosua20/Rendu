#include "DebugViewer.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/Texture.hpp"
#include "resources/Mesh.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/System.hpp"
#include "system/TextUtilities.hpp"

static const std::map<TextureShape, std::string> shapeNames = {
	{TextureShape::D1, "1D"},
	{TextureShape::Array1D, "1D array"},
	{TextureShape::D2, "2D"},
	{TextureShape::Array2D, "2D array"},
	{TextureShape::Cube, "Cube"},
	{TextureShape::ArrayCube, "Cube array"},
	{TextureShape::D3, "3D"}};

static const std::string debugSkipName = "@debugViewerSkipItem@";

DebugViewer * DebugViewer::_shared = nullptr;

DebugViewer::DebugViewer(bool silent) : _silent(silent) {
	if(!_silent) {
		_texDisplay = Resources::manager().getProgram2D("debug_texture_display");
	}
}

void DebugViewer::track(const Texture * tex) {
	if(_silent || tex->name() == debugSkipName) {
		return;
	}
	if(!tex->gpu) {
		Log::Warning() << "[DebugViewer] \"" << tex->name() << "\" has no GPU data." << std::endl;
		return;
	}
	// Generate default name if empty.
	std::string finalName = tex->name();
	if(finalName.empty()) {
		finalName = "Texture " + TextUtilities::padInt(_textureId++, 3);
	}

	// Check if this specific object already is registered, in that case just update the name.
	for(TextureInfos & infos : _textures) {
		if(infos.tex == tex) {
			infos.name = finalName;
			// Sort framebuffers list.
			std::sort(_textures.begin(), _textures.end(), [](const TextureInfos & a, const TextureInfos & b) {
				return a.name < b.name;
			});
			return;
		}
	}
	// Else create a new texture infos element.
	_textures.emplace_back();
	registerTexture(finalName, tex, _textures.back());

	// Sort textures list.
	std::sort(_textures.begin(), _textures.end(), [](const TextureInfos & a, const TextureInfos & b) {
		return a.name < b.name;
	});
}

void DebugViewer::track(const Framebuffer * buffer) {
	if(_silent || buffer->name() == debugSkipName) {
		return;
	}
	
	// Generate default name if empty.
	std::string finalName = buffer->name();
	if(finalName.empty()) {
		finalName = "Framebuffer " + TextUtilities::padInt(_bufferId++, 3);
	}
	finalName.append(" (" + shapeNames.at(buffer->shape()) + ")");

	// Check if this specific object already is registered, in that case just update the name.
	for(FramebufferInfos & infos : _framebuffers) {
		if(infos.buffer == buffer) {
			infos.name = finalName;
			// Sort framebuffers list.
			std::sort(_framebuffers.begin(), _framebuffers.end(), [](const FramebufferInfos & a, const FramebufferInfos & b) {
				return a.name < b.name;
			});
			return;
		}
	}
	// Else create a new framebuffer infos element.
	_framebuffers.emplace_back();
	FramebufferInfos & infos = _framebuffers.back();
	infos.buffer			 = buffer;
	infos.name				 = finalName;

	// Register color attachments.
	for(uint cid = 0; cid < buffer->attachments(); ++cid) {
		const std::string nameAttach = "Color " + std::to_string(cid); 
		infos.attachments.emplace_back();
		registerTexture(nameAttach, buffer->texture(cid), infos.attachments.back());
	}
	// Register depth attachment if it's a texture.
	const Texture * depthAttach = buffer->depthBuffer();
	if(depthAttach) {
		infos.attachments.emplace_back();
		registerTexture("Depth", depthAttach, infos.attachments.back());
	}
	// Sort framebuffers list.
	std::sort(_framebuffers.begin(), _framebuffers.end(), [](const FramebufferInfos & a, const FramebufferInfos & b) {
		return a.name < b.name;
	});
}

void DebugViewer::track(const Mesh * mesh) {
	if(_silent) {
		return;
	}
	if(!mesh->gpu) {
		Log::Warning() << "[DebugViewer] \"" << mesh->name() << "\" has no GPU data." << std::endl;
		return;
	}
	// Generate default name if empty.
	std::string finalName = mesh->name();
	if(finalName.empty()) {
		finalName = "Mesh " + TextUtilities::padInt(_meshId++, 3);
	}

	// Check if this specific object already is registered, in that case just update the name.
	for(MeshInfos & infos : _meshes) {
		if(infos.mesh == mesh) {
			infos.name = finalName;
			// Sort meshes list.
			std::sort(_meshes.begin(), _meshes.end(), [](const MeshInfos & a, const MeshInfos & b) {
				return a.name < b.name;
			});
			return;
		}
	}
	// Else create a new meshes infos element.
	_meshes.emplace_back();
	MeshInfos & infos = _meshes.back();
	infos.name = finalName;
	infos.mesh = mesh;

	// Sort meshes list.
	std::sort(_meshes.begin(), _meshes.end(), [](const MeshInfos & a, const MeshInfos & b) {
		return a.name < b.name;
	});
}

void DebugViewer::trackState(const std::string & name){
	if(_silent){
		return;
	}
	// Only update the state if it's currently displayed on screen,
	// or if it's the very first time it's queried.
	if(_states[name].visible || !_states[name].populated){
		GLUtilities::getState(_states[name].state);
		_states[name].populated = true;
	}
}

void DebugViewer::registerTexture(const std::string& name, const Texture* tex, TextureInfos & infos) {
	infos.name  = name;
	infos.tex   = tex;
	infos.gamma = tex->gpu->descriptor().isSRGB();

	// Setup display framebuffer.
	const Descriptor desc = {Layout::RGB8, Filter::NEAREST, Wrap::CLAMP};
	infos.display.reset(new Framebuffer(TextureShape::D2, tex->width, tex->height, 1, 1, {desc}, false, debugSkipName));

	// Build display full name with details.
	const std::string details = shapeNames.at(tex->shape) + " (" + tex->gpu->descriptor().string() + ")";
	infos.displayName		  = " - " + std::to_string(tex->width) + "x" + std::to_string(tex->height) + " - " + details + "##" + std::to_string(_winId++);
}


void DebugViewer::untrack(const Texture * tex) {
	auto end = std::remove_if(_textures.begin(), _textures.end(), [tex](const TextureInfos & infos) {
		return infos.tex == tex;
	});
	_textures.erase(end, _textures.end());
}

void DebugViewer::untrack(const Framebuffer * buffer) {
	auto end = std::remove_if(_framebuffers.begin(), _framebuffers.end(), [buffer](const FramebufferInfos & infos) {
		return infos.buffer == buffer;
	});
	_framebuffers.erase(end, _framebuffers.end());
}

void DebugViewer::untrack(const Mesh * mesh) {
	auto end = std::remove_if(_meshes.begin(), _meshes.end(), [mesh](const MeshInfos & infos) {
		return infos.mesh == mesh;
	});
	_meshes.erase(end, _meshes.end());
}

void DebugViewer::interface() {
	if(_silent) {
		return;
	}

	// Display menu bar listing all resources.
	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("Textures")) {
			for(TextureInfos & tex : _textures) {
				ImGui::PushID(tex.tex);
				ImGui::MenuItem(tex.name.c_str(), nullptr, &tex.visible);
				ImGui::PopID();
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Framebuffers")) {
			for(FramebufferInfos & buffer : _framebuffers) {
				ImGui::PushID(buffer.buffer);
				if(ImGui::BeginMenu(buffer.name.c_str())) {
					for(TextureInfos & tex : buffer.attachments) {
						ImGui::MenuItem(tex.name.c_str(), nullptr, &tex.visible);
					}
					ImGui::EndMenu();
				}
				ImGui::PopID();
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Meshes")) {
			for(MeshInfos & mesh : _meshes) {
				ImGui::PushID(mesh.mesh);
				ImGui::MenuItem(mesh.name.c_str(), nullptr, &mesh.visible);
				ImGui::PopID();
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("States")) {
			for(auto & infos : _states) {
				ImGui::MenuItem(infos.first.c_str(), nullptr, &infos.second.visible);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// Display all active windows.
	for(TextureInfos & tex : _textures) {
		if(!tex.visible) {
			continue;
		}
		displayTexture("", tex);
	}
	for(FramebufferInfos & buffer : _framebuffers) {
		for(TextureInfos & tex : buffer.attachments) {
			if(!tex.visible) {
				continue;
			}
			displayTexture(buffer.name + " - ", tex);
		}
	}
	for(MeshInfos & mesh : _meshes) {
		if(!mesh.visible) {
			continue;
		}
		displayMesh(mesh);
	}
	for(auto & infos : _states){
		if(!infos.second.visible){
			continue;
		}
		displayState(infos.first, infos.second);
	}
}

void DebugViewer::displayState(const std::string & name, StateInfos & infos){

	static const std::map<bool, std::string> bools = {{true, "yes"}, {false, "no"}};
	static const std::map<DepthEquation, std::string> depthEqs = {
			{DepthEquation::NEVER, "Never"},
			{DepthEquation::LESS, "Less"},
			{DepthEquation::LEQUAL, "Less or equal"},
			{DepthEquation::EQUAL, "Equal"},
			{DepthEquation::GREATER, "Greater"},
			{DepthEquation::GEQUAL, "Greater or equal"},
			{DepthEquation::NOTEQUAL, "Not equal"},
			{DepthEquation::ALWAYS, "Always"}};
	static const std::map<BlendEquation, std::string> blendEqs = {
			{BlendEquation::ADD, "Add"},
			{BlendEquation::SUBTRACT, "Subtract"},
			{BlendEquation::REVERSE_SUBTRACT, "Reverse subtract"},
			{BlendEquation::MIN, "Min"},
			{BlendEquation::MAX, "Max"}};
	static const std::map<BlendFunction, std::string> funcs = {
			{BlendFunction::ONE, "1"},
			{BlendFunction::ZERO, "0"},
			{BlendFunction::SRC_COLOR, "Src color"},
			{BlendFunction::ONE_MINUS_SRC_COLOR, "1 - src color"},
			{BlendFunction::SRC_ALPHA, "Src alpha"},
			{BlendFunction::ONE_MINUS_SRC_ALPHA, "1 - src alpha"},
			{BlendFunction::DST_COLOR, "Dst color"},
			{BlendFunction::ONE_MINUS_DST_COLOR, "1 - dst color"},
			{BlendFunction::DST_ALPHA, "Dst alpha"},
			{BlendFunction::ONE_MINUS_DST_ALPHA, "1 - dst alpha"}};
	static const std::map<Faces, std::string> faces = {
		{Faces::FRONT, "Front"},
		{Faces::BACK, "Back"},
		{Faces::ALL, "Front & back"}};

	const std::string finalName = "State - " + name;
	if(ImGui::Begin(finalName.c_str(), &infos.visible)) {
		const GPUState & st = infos.state;

		if(ImGui::CollapsingHeader("Blending")){
			std::stringstream str;
			str << "Blending: " << bools.at(st.blend) << "\n";
			str << "Blend equation: " << "RGB: " << blendEqs.at(st.blendEquationRGB) << ", A: " << blendEqs.at(st.blendEquationAlpha) << "\n";
			str << "Blend source: " << "RGB: " << funcs.at(st.blendSrcRGB) << ", A: " << funcs.at(st.blendSrcAlpha) << "\n";
			str << "Blend desti.: " << "RGB: " << funcs.at(st.blendDstRGB) << ", A: " << funcs.at(st.blendDstAlpha) << "\n";
			str << "Blend color: " << st.blendColor << "\n";
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}

		if(ImGui::CollapsingHeader("Depth")){
			std::stringstream str;
			str << "Depth test: " << bools.at(st.depthTest) << ", write: " << bools.at(st.depthWriteMask) << "\n";
			str << "Depth function: " << depthEqs.at(st.depthFunc) << "\n";
			str << "Depth clear: " << st.depthClearValue << "\n";
			str << "Depth range: " << st.depthRange << ", clamp: " << bools.at(st.depthClamp) << "\n";
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}

		if(ImGui::CollapsingHeader("Color")){
			std::stringstream str;
			str << "Color clear: " << st.colorClearValue << "\n";
			str << "Color write: " << bools.at(st.colorWriteMask[0]) << ", " << bools.at(st.colorWriteMask[1]) << ", " << bools.at(st.colorWriteMask[2]) << ", " << bools.at(st.colorWriteMask[3]) << "\n";
			str << "Framebuffer sRGB: " << bools.at(st.framebufferSRGB) << "\n";
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}

		if(ImGui::CollapsingHeader("Geometry")){
			std::stringstream str;
			str << "Culling: " << bools.at(st.cullFace) << ", " << faces.at(st.cullFaceMode) << "\n";
			str << "Polygon offset: point: " << bools.at(st.polygonOffsetPoint) << ", line: " << bools.at(st.polygonOffsetLine) << ", fill: " << bools.at(st.polygonOffsetFill) << "\n";
			str << "Polygon offset: factor: " << st.polygonOffsetFactor << ", units: " << st.polygonOffsetUnits << "\n";

			str << "Point size: " << st.pointSize << ", program: " << bools.at(st.programPointSize) << "\n";
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}

		if(ImGui::CollapsingHeader("Viewport")){
			std::stringstream str;
			str << "Scissor: test: " << bools.at(st.scissorTest) <<", box: " << st.scissorBox << "\n";
			str << "Viewport: " << st.viewport;
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}
	}
}

void DebugViewer::displayMesh(MeshInfos & mesh) {

	ImGui::SetNextWindowSize(ImVec2(280, 130), ImGuiCond_Once);
	const std::string finalWinName = "Mesh - " + mesh.name;

	if(ImGui::Begin(finalWinName.c_str(), &mesh.visible)) {
		ImGui::Columns(2);
		ImGui::Text("Vertices: %lu", mesh.mesh->positions.size());
		ImGui::NextColumn();
		ImGui::Text("Normals: %lu", mesh.mesh->normals.size());
		ImGui::NextColumn();
		ImGui::Text("Tangents: %lu", mesh.mesh->tangents.size());
		ImGui::NextColumn();
		ImGui::Text("Binormals: %lu", mesh.mesh->binormals.size());
		ImGui::NextColumn();
		ImGui::Text("Colors: %lu", mesh.mesh->colors.size());
		ImGui::NextColumn();
		ImGui::Text("UVs: %lu", mesh.mesh->texcoords.size());
		ImGui::NextColumn();
		ImGui::Text("Indices: %lu", mesh.mesh->indices.size());
		ImGui::Columns(0);
		const auto & bbox = mesh.mesh->bbox;
		if(!bbox.empty()){
			ImGui::Text("Bbox: min: %.3f, %.3f, %.3f", bbox.minis[0], bbox.minis[1], bbox.minis[2]);
			ImGui::Text("      max: %.3f, %.3f, %.3f", bbox.maxis[0], bbox.maxis[1], bbox.maxis[2]);
		}

	}
	ImGui::End();
}

void DebugViewer::displayTexture(const std::string & prefix, TextureInfos & tex) {
	float aspect = float(tex.tex->width) / std::max(float(tex.tex->height), 1.0f);
	if(tex.tex->shape & TextureShape::Cube) {
		aspect = 2.0f;
	}
	// Fixed width, height takes into account texture aspect ratio and upper settings bar.
	const float defaultWidth = 550.0f;
	ImGui::SetNextWindowSize(ImVec2(defaultWidth, defaultWidth / aspect + 75.0f), ImGuiCond_Once);
	const std::string finalWinName = prefix + tex.name + tex.displayName;

	if(ImGui::Begin(finalWinName.c_str(), &tex.visible)) {
		ImGui::Columns(2);

		ImGui::PushItemWidth(80);
		// Display options.
		if(ImGui::SliderInt("Level", &tex.mip, 0, int(tex.tex->levels) - 1)) {
			tex.mip = glm::clamp(tex.mip, 0, int(tex.tex->levels) - 1);
		}
		ImGui::SameLine();
		if(ImGui::SliderInt("Layer", &tex.layer, 0, int(tex.tex->depth) - 1)) {
			tex.layer = glm::clamp(tex.layer, 0, int(tex.tex->depth) - 1);
		}
		ImGui::PopItemWidth();

		ImGui::NextColumn();

		ImGui::DragFloatRange2("Range", &tex.range[0], &tex.range[1], 0.1f, -FLT_MAX, FLT_MAX);

		ImGui::NextColumn();

		ImGui::Checkbox("R", &tex.channels[0]);
		ImGui::SameLine();
		ImGui::Checkbox("G", &tex.channels[1]);
		ImGui::SameLine();
		ImGui::Checkbox("B", &tex.channels[2]);
		ImGui::SameLine();
		ImGui::Checkbox("A", &tex.channels[3]);
		ImGui::SameLine();

		ImGui::Checkbox("Gamma", &tex.gamma);
		ImGui::Columns(1);

		// Prepare the framebuffer content based on the texture type.
		updateDisplay(tex);

		// Display.
		const ImVec2 winSize = ImGui::GetContentRegionAvail();
		ImGui::ImageButton(*tex.display->texture(), ImVec2(winSize.x, winSize.y), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0), 0);
		if(ImGui::IsItemHovered()) {
			ImGui::CaptureMouseFromApp(false);
			ImGui::CaptureKeyboardFromApp(false);
		}
	}
	ImGui::End();
}

void DebugViewer::updateDisplay(const TextureInfos & tex) {

	static const std::map<TextureShape, uint> slots = {
		{TextureShape::D1, 0},
		{TextureShape::Array1D, 1},
		{TextureShape::D2, 2},
		{TextureShape::Array2D, 3},
		{TextureShape::Cube, 4},
		{TextureShape::ArrayCube, 5},
		{TextureShape::D3, 6}};

	tex.display->bind();
	tex.display->setViewport();

	_texDisplay->use();
	_texDisplay->uniform("layer", tex.layer);
	_texDisplay->uniform("level", tex.mip);
	_texDisplay->uniform("range", tex.range);
	_texDisplay->uniform("channels", glm::ivec4(tex.channels));
	_texDisplay->uniform("gamma", tex.gamma);
	_texDisplay->uniform("shape", int(tex.tex->shape));

	GLUtilities::bindTexture(tex.tex, slots.at(tex.tex->shape));
	ScreenQuad::draw();
	tex.display->unbind();
}

void DebugViewer::setDefault(DebugViewer * viewer) {
	_shared = viewer;
}

void DebugViewer::trackDefault(const Texture * tex) {
	if(!_shared) {
		return;
	}
	_shared->track(tex);
}

void DebugViewer::trackDefault(const Framebuffer * buffer) {
	if(!_shared) {
		return;
	}
	_shared->track(buffer);
}

void DebugViewer::trackDefault(const Mesh * mesh) {
	if(!_shared) {
		return;
	}
	_shared->track(mesh);
}

void DebugViewer::untrackDefault(const Texture * tex) {
	if(!_shared) {
		return;
	}
	_shared->untrack(tex);
}

void DebugViewer::untrackDefault(const Framebuffer * buffer) {
	if(!_shared) {
		return;
	}
	_shared->untrack(buffer);
}

void DebugViewer::untrackDefault(const Mesh * mesh) {
	if(!_shared) {
		return;
	}
	_shared->untrack(mesh);
}

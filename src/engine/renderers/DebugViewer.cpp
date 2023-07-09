#include "DebugViewer.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUObjects.hpp"
#include "resources/Texture.hpp"
#include "resources/Mesh.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/System.hpp"
#include "system/TextUtilities.hpp"

static const std::unordered_map<TextureShape, std::string> shapeNames = {
	{TextureShape::D1, "1D"},
	{TextureShape::Array1D, "1D array"},
	{TextureShape::D2, "2D"},
	{TextureShape::Array2D, "2D array"},
	{TextureShape::Cube, "Cube"},
	{TextureShape::ArrayCube, "Cube array"},
	{TextureShape::D3, "3D"}};

static const std::string debugSkipName = "@debugViewerSkipItem@";

DebugViewer * DebugViewer::_shared = nullptr;

DebugViewer::DebugViewer() {
	_texDisplay = Resources::manager().getProgram2D("debug_texture_display");
}

void DebugViewer::track(const Texture * tex) {
	if(tex->name() == debugSkipName) {
		return;
	}
	if(!tex->gpu) {
		Log::Warning() << "[DebugViewer] \"" << tex->name() << "\" has no GPU data." << std::endl;
		return;
	}
	// Generate default name if empty.
	std::string finalName = tex->name();
	if(finalName.empty()) {
		if(tex->drawable){
			finalName = "Drawable " + TextUtilities::padInt(_drawableId++, 3);
		} else {
			finalName = "Texture " + TextUtilities::padInt(_textureId++, 3);
		}
	}

	std::vector<TextureInfos>& texturesList = tex->drawable ? _drawables : _textures;
	// Check if this specific object already is registered, in that case just update the name.
	for(TextureInfos & infos : texturesList) {
		if(infos.tex == tex) {
			infos.name = finalName;
			// Sort texture list.
			std::sort(texturesList.begin(), texturesList.end(), [](const TextureInfos & a, const TextureInfos & b) {
				return a.name < b.name;
			});
			return;
		}
	}
	// Else create a new texture infos element.
	texturesList.emplace_back();
	registerTexture(finalName, tex, texturesList.back());

	// Sort textures list.
	std::sort(texturesList.begin(), texturesList.end(), [](const TextureInfos & a, const TextureInfos & b) {
		return a.name < b.name;
	});
}

void DebugViewer::track(const Mesh * mesh) {
	
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
	// Else create a new mesh infos element.
	_meshes.emplace_back();
	MeshInfos & infos = _meshes.back();
	infos.name = finalName;
	infos.mesh = mesh;

	// Sort meshes list.
	std::sort(_meshes.begin(), _meshes.end(), [](const MeshInfos & a, const MeshInfos & b) {
		return a.name < b.name;
	});
}

void DebugViewer::pushMarker(const std::string& category, const std::string& label, const glm::vec4& color){
	// Find the category or create it.
	MarkerCategoryInfos& collection = _markers[category];
	if(!collection.record){
		return;
	}

	std::vector<MarkerInfos>* level = &collection.markers;
	for(unsigned int depth = 0; depth < collection.depth; ++depth){
		level = &(level->back().markers);
	}

	MarkerInfos infos;
	infos.name = label;
	infos.color = color;
	infos.index = ++_markerId;
	level->push_back(infos);

	++collection.depth;
}

void DebugViewer::insertMarker(const std::string& category, const std::string& label, const glm::vec4& color){
	// Find the category or create it.
	MarkerCategoryInfos& collection = _markers[category];
	if(!collection.record){
		return;
	}

	std::vector<MarkerInfos>* level = &collection.markers;
	for(unsigned int depth = 0; depth < collection.depth; ++depth){
		level = &(level->back().markers);
	}

	MarkerInfos infos;
	infos.name = label;
	infos.color = color;
	infos.index = ++_markerId;
	level->push_back(infos);
}

void DebugViewer::popMarker(const std::string& category){
	MarkerCategoryInfos& collection = _markers[category];
	if(!collection.record){
		return;
	}

	assert(collection.depth > 0);
	--collection.depth;
}

void DebugViewer::trackState(const std::string & name){
	// Only update the state if it's currently displayed on screen,
	// or if it's the very first time it's queried.
	if(_states[name].visible || !_states[name].populated){
		GPU::getState(_states[name].state);
		_states[name].populated = true;
	}
}

void DebugViewer::registerTexture(const std::string& name, const Texture* tex, TextureInfos & infos) {
	infos.name  = name;
	infos.tex   = tex;

	const Layout format = tex->format;
	infos.gamma = GPUTexture::isSRGB(format);

	// Setup display texture.
	infos.display.reset(new Texture(debugSkipName));
	infos.display->setupAsDrawable(Layout::RGBA8, tex->width, tex->height, TextureShape::D2, 1, 1);

	// GPU format strings
	#define STRENUM(X) { Layout::X, #X}

	static const std::unordered_map<Layout, std::string> strFormats = {
		STRENUM(R8),
		STRENUM(RG8),
		STRENUM(RGBA8),
		STRENUM(SRGB8_ALPHA8),
		STRENUM(BGRA8),
		STRENUM(SBGR8_ALPHA8),
		STRENUM(R16),
		STRENUM(RG16),
		STRENUM(RGBA16),
		STRENUM(R8_SNORM),
		STRENUM(RG8_SNORM),
		STRENUM(RGBA8_SNORM),
		STRENUM(R16_SNORM),
		STRENUM(RG16_SNORM),
		STRENUM(R16F),
		STRENUM(RG16F),
		STRENUM(RGBA16F),
		STRENUM(R32F),
		STRENUM(RG32F),
		STRENUM(RGBA32F),
		STRENUM(RGB5_A1),
		STRENUM(A2_BGR10),
		STRENUM(A2_RGB10),
		STRENUM(DEPTH_COMPONENT32F),
		STRENUM(DEPTH24_STENCIL8),
		STRENUM(DEPTH_COMPONENT16),
		STRENUM(DEPTH32F_STENCIL8),
		STRENUM(R8UI),
		STRENUM(R16I),
		STRENUM(R16UI),
		STRENUM(R32I),
		STRENUM(R32UI),
		STRENUM(RG8I),
		STRENUM(RG8UI),
		STRENUM(RG16I),
		STRENUM(RG16UI),
		STRENUM(RG32I),
		STRENUM(RG32UI),
		STRENUM(RGBA8I),
		STRENUM(RGBA8UI),
		STRENUM(RGBA16I),
		STRENUM(RGBA16UI),
		STRENUM(RGBA32I),
		STRENUM(RGBA32UI)
	};

	#undef STRENUM

	// Build display full name with details.
	const std::string details = shapeNames.at(tex->shape) + " (" + strFormats.at(format) + ")";
	infos.displayName		  = " - " + std::to_string(tex->width) + "x" + std::to_string(tex->height) + " - " + details + "##" + std::to_string(_winId++);
}

void DebugViewer::untrack(const Texture * tex) {
	std::vector<TextureInfos>& textureList = tex->drawable ? _drawables : _textures;
	auto end = std::remove_if(textureList.begin(), textureList.end(), [tex](const TextureInfos & infos) {
		return infos.tex == tex;
	});
	textureList.erase(end, textureList.end());
}

void DebugViewer::untrack(const Mesh * mesh) {
	auto end = std::remove_if(_meshes.begin(), _meshes.end(), [mesh](const MeshInfos & infos) {
		return infos.mesh == mesh;
	});
	_meshes.erase(end, _meshes.end());
}

void DebugViewer::nextFrame(){
	++_frameCounter;
	_markerId = 0;

	// Record everything _markerFrequency frames.
	for(auto& infos : _markers){
		MarkerCategoryInfos& category = infos.second;
		category.record = (_frameCounter % category.frequency) == category.offset;
		if(category.record){
			category.depth = 0;
			category.markers.clear();
		}
	}
}

void DebugViewer::interface() {

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
		if(ImGui::BeginMenu("Drawables")) {
			for(TextureInfos & tex : _drawables) {
				ImGui::PushID(tex.tex);
				ImGui::MenuItem(tex.name.c_str(), nullptr, &tex.visible);
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
		if(ImGui::BeginMenu("Markers")) {
			for(auto & markers : _markers) {
				ImGui::MenuItem(markers.first.c_str(), nullptr, &markers.second.visible);
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

	for(TextureInfos & tex : _drawables) {
		if(!tex.visible) {
			continue;
		}
		displayTexture("", tex);
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

	for(auto & infos : _markers){
		if(!infos.second.visible){
			continue;
		}
		displayMarkers(infos.first, infos.second);
	}

	// Display raw metrics.
	displayMetrics();
}

void DebugViewer::displayMetrics(){
	const GPU::Metrics & metrics = GPU::getMetrics();
	if(ImGui::Begin("Metrics##DEBUGVIEWER")){
		if(ImGui::CollapsingHeader("Global", ImGuiTreeNodeFlags_DefaultOpen)){
			ImGui::Text("Uploads: %llu", metrics.uploads);
			ImGui::Text("Downloads: %llu", metrics.downloads);
			ImGui::Text("Textures: %llu", metrics.textures);
			ImGui::Text("Buffers: %llu", metrics.buffers);
			ImGui::Text("Programs: %llu", metrics.programs);
			ImGui::Text("Pipelines: %llu", metrics.pipelines);
		}
		if(ImGui::CollapsingHeader("Per-frame", ImGuiTreeNodeFlags_DefaultOpen)){
			ImGui::Text("Blits: %llu", metrics.blitCount);
			ImGui::Text("Render passes: %llu", metrics.renderPasses);
			ImGui::Text("Pipeline bindings: %llu", metrics.pipelineBindings);
			ImGui::Text("Mesh bindings: %llu", metrics.meshBindings);
			ImGui::Text("Screen quads: %llu", metrics.quadCalls);
			ImGui::Text("Draw calls: %llu", metrics.drawCalls);
		}
	}
	ImGui::End();
}

void DebugViewer::displayMarker(const MarkerInfos& marker){
	ImGui::PushID(marker.index);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(marker.color[0], marker.color[1], marker.color[2], marker.color[3]));
	if(marker.markers.empty()){
		ImGui::BulletText("%s", marker.name.c_str());
	} else {
		if(ImGui::TreeNode(marker.name.c_str())){
			for(const MarkerInfos& child : marker.markers){
				displayMarker(child);
			}
			ImGui::TreePop();
		}
	}
	ImGui::PopStyleColor();
	ImGui::PopID();
}

void DebugViewer::displayMarkers(const std::string & name, MarkerCategoryInfos& category){
	ImGui::SetNextWindowSize(ImVec2(280, 480), ImGuiCond_Once);
	const std::string finalWinName = "Makers - " + name;

	if(ImGui::Begin(finalWinName.c_str(), &category.visible)) {

		// Header of settings.
		ImGui::PushItemWidth(120);
		if(ImGui::InputInt("Frequency", &category.frequency, 1, 1)){
			category.frequency = glm::max(1, category.frequency);
			category.offset = glm::clamp(category.offset, 0, category.frequency - 1);
		}
		if(ImGui::InputInt("Offset", &category.offset, 1, 1)){
			category.offset = glm::clamp(category.offset, 0, category.frequency - 1);
		}
		ImGui::PopItemWidth();
		ImGui::Separator();

		for(const MarkerInfos& marker : category.markers){
			displayMarker(marker);
		}
	}
	ImGui::End();
}

void DebugViewer::displayState(const std::string & name, StateInfos & infos){

	static const std::unordered_map<bool, std::string> bools = {{true, "yes"}, {false, "no"}};
	static const std::unordered_map<TestFunction, std::string> testEqs = {
			{TestFunction::NEVER, "Never"},
			{TestFunction::LESS, "Less"},
			{TestFunction::LEQUAL, "Less or equal"},
			{TestFunction::EQUAL, "Equal"},
			{TestFunction::GREATER, "Greater"},
			{TestFunction::GEQUAL, "Greater or equal"},
			{TestFunction::NOTEQUAL, "Not equal"},
			{TestFunction::ALWAYS, "Always"}};
	static const std::unordered_map<BlendEquation, std::string> blendEqs = {
			{BlendEquation::ADD, "Add"},
			{BlendEquation::SUBTRACT, "Subtract"},
			{BlendEquation::REVERSE_SUBTRACT, "Reverse subtract"},
			{BlendEquation::MIN, "Min"},
			{BlendEquation::MAX, "Max"}};
	static const std::unordered_map<BlendFunction, std::string> funcs = {
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
	static const std::unordered_map<Faces, std::string> faces = {
		{Faces::FRONT, "Front"},
		{Faces::BACK, "Back"},
		{Faces::ALL, "Front & back"}};
	static const std::unordered_map<StencilOp, std::string> ops = {
			{ StencilOp::KEEP, "Keep" },
			{ StencilOp::ZERO, "Zero" },
			{ StencilOp::REPLACE, "Replace" },
			{ StencilOp::INCR, "Increment & clamp" },
			{ StencilOp::INCRWRAP, "Increment & wrap" },
			{ StencilOp::DECR, "Decrement & clamp" },
			{ StencilOp::DECRWRAP, "Decrement & wrap" },
			{ StencilOp::INVERT, "Invert bitwise" }};

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
			str << "Depth function: " << testEqs.at(st.depthFunc) << "\n";
			//str << "Depth clear: " << st.depthClearValue << "\n";
			//str << "Depth range: " << st.depthRange << ", clamp: " << bools.at(st.depthClamp) << "\n";
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}

		if(ImGui::CollapsingHeader("Stencil")){
			std::stringstream str;
			str << "Stencil test: " << bools.at(st.stencilTest) << ", write: " << bools.at(st.stencilWriteMask) << "\n";
			str << "Stencil function: " << testEqs.at(st.stencilFunc) << "\n";
			str << "Stencil operations: " << "Fail: " << ops.at(st.stencilFail) << "\n";
			str << "\t Pass: stencil: " << ops.at(st.stencilPass) << ", depth: " << ops.at(st.stencilDepthPass) << "\n";
			str << "Stencil value: " << std::hex << uint(st.stencilValue);
			//str << ", stencil clear: " << std::hex << uint(st.stencilClearValue) << "\n";
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}

		if(ImGui::CollapsingHeader("Color")){
			std::stringstream str;
			//str << "Color clear: " << st.colorClearValue << "\n";
			str << "Color write: " << bools.at(st.colorWriteMask[0]) << ", " << bools.at(st.colorWriteMask[1]) << ", " << bools.at(st.colorWriteMask[2]) << ", " << bools.at(st.colorWriteMask[3]) << "\n";
			//str << "Support sRGB: " << bools.at(st.supportsRGB) << "\n";
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}

		if(ImGui::CollapsingHeader("Geometry")){
			std::stringstream str;
			str << "Culling: " << bools.at(st.cullFace) << ", " << faces.at(st.cullFaceMode) << "\n";
			//str << "Polygon offset: " << bools.at(st.polygonOffset) << "\n";
			//str << "Polygon offset: factor: " << st.polygonOffsetFactor << ", units: " << st.polygonOffsetUnits << "\n";

			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}

		if(ImGui::CollapsingHeader("Viewport")){
			std::stringstream str;
			//str << "Scissor: test: " << bools.at(st.scissorTest) <<", box: " << st.scissorBox << "\n";
			//str << "Viewport: " << st.viewport;
			const std::string strRes = str.str();
			ImGui::Text("%s", strRes.c_str());
		}
	}
	ImGui::End();
}

void DebugViewer::displayMesh(MeshInfos & mesh) {

	ImGui::SetNextWindowSize(ImVec2(280, 160), ImGuiCond_Once);
	const std::string finalWinName = "Mesh - " + mesh.name;

	if(ImGui::Begin(finalWinName.c_str(), &mesh.visible)) {
		const Mesh::Metrics & metrics = mesh.mesh->metrics();
		ImGui::Columns(2);
		ImGui::Text("Vertices: %lu", metrics.vertices);
		ImGui::NextColumn();
		ImGui::Text("Normals: %lu", metrics.normals);
		ImGui::NextColumn();
		ImGui::Text("Tangents: %lu", metrics.tangents);
		ImGui::NextColumn();
		ImGui::Text("Bitangents: %lu", metrics.bitangents);
		ImGui::NextColumn();
		ImGui::Text("Colors: %lu", metrics.colors);
		ImGui::NextColumn();
		ImGui::Text("UVs: %lu", metrics.texcoords);
		ImGui::NextColumn();
		ImGui::Text("Indices: %lu", metrics.indices);
		ImGui::Columns();
		const auto & bbox = mesh.mesh->bbox;
		if(!bbox.empty()){
			ImGui::Text("Bbox: min: %.3f, %.3f, %.3f", bbox.minis[0], bbox.minis[1], bbox.minis[2]);
			ImGui::Text("              max: %.3f, %.3f, %.3f", bbox.maxis[0], bbox.maxis[1], bbox.maxis[2]);
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
	const float defaultWidth = 570.0f;
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

		// Prepare the display content based on the texture type.
		updateDisplay(tex);

		// Display.
		const ImVec2 winSize = ImGui::GetContentRegionAvail();
		ImGui::ImageButton("#Tex", *tex.display, ImVec2(winSize.x, winSize.y), ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
		if(ImGui::IsItemHovered()) {
			ImGui::SetNextFrameWantCaptureKeyboard(false);
			ImGui::SetNextFrameWantCaptureMouse(false);
		}
	}
	ImGui::End();
}

void DebugViewer::updateDisplay(const TextureInfos & tex) {

	static const std::unordered_map<TextureShape, uint> slots = {
		{TextureShape::D1, 0},
		{TextureShape::Array1D, 1},
		{TextureShape::D2, 2},
		{TextureShape::Array2D, 3},
		{TextureShape::Cube, 4},
		{TextureShape::ArrayCube, 5},
		{TextureShape::D3, 6}};

	GPU::beginRender(Load::Operation::DONTCARE, tex.display.get());
	GPU::setViewport(*tex.display);

	_texDisplay->use();
	_texDisplay->uniform("layer", tex.layer);
	_texDisplay->uniform("level", tex.mip);
	_texDisplay->uniform("range", tex.range);
	_texDisplay->uniform("channels", glm::ivec4(tex.channels));
	_texDisplay->uniform("gamma", tex.gamma);
	_texDisplay->uniform("shape", int(tex.tex->shape));

	for(const auto& texSlot : slots){
		if(tex.tex->shape == texSlot.first){
			_texDisplay->texture(tex.tex, texSlot.second);
		} else {
			_texDisplay->texture(Resources::manager().getDefaultTexture(texSlot.first), texSlot.second);
		}
	}

	GPU::drawQuad();
	GPU::endRender();
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

void DebugViewer::trackDefault(const Mesh * mesh) {
	if(!_shared) {
		return;
	}
	_shared->track(mesh);
}

void DebugViewer::trackStateDefault(const std::string & name) {
	if(!_shared) {
		return;
	}
	_shared->trackState(name);
}

void DebugViewer::untrackDefault(const Texture * tex) {
	if(!_shared) {
		return;
	}
	_shared->untrack(tex);
}

void DebugViewer::untrackDefault(const Mesh * mesh) {
	if(!_shared) {
		return;
	}
	_shared->untrack(mesh);
}

void DebugViewer::pushMarkerDefault(const std::string& category, const std::string& label, const glm::vec4& color){
	if(!_shared) {
		return;
	}
	_shared->pushMarker(category, label, color);
}

void DebugViewer::insertMarkerDefault(const std::string& category, const std::string& label, const glm::vec4& color){
	if(!_shared) {
		return;
	}
	_shared->insertMarkerDefault(category, label, color);
}

void DebugViewer::popMarkerDefault(const std::string& category){
	if(!_shared) {
		return;
	}
	_shared->popMarker(category);
}

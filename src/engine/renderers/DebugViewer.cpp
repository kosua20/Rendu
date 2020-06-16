#include "DebugViewer.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/Texture.hpp"
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

DebugViewer::DebugViewer(bool silent){
	_texDisplay = Resources::manager().getProgram2D("debug_texture_display");
	_silent		= silent;
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
	for(Infos & infos : _textures) {
		if(infos.tex == tex) {
			infos.name = finalName;
			// Sort framebuffers list.
			std::sort(_textures.begin(), _textures.end(), [](const Infos & a, const Infos & b) {
				return a.name < b.name;
			});
			return;
		}
	}
	// Else create a new texture infos element.
	_textures.emplace_back();
	registerTexture(finalName, tex, _textures.back());

	// Sort textures list.
	std::sort(_textures.begin(), _textures.end(), [](const Infos & a, const Infos & b) {
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

void DebugViewer::registerTexture(const std::string& name, const Texture* tex, Infos& infos) {
	infos.name  = name;
	infos.tex   = tex;
	infos.gamma = tex->gpu->descriptor().isSRGB();

	// Setup display framebuffer.
	const Descriptor desc = {Layout::RGB8, Filter::NEAREST, Wrap::CLAMP};
	infos.display.reset(new Framebuffer(TextureShape::D2, tex->width, tex->height, 1, 1, {desc}, false, debugSkipName));

	// Build display full name with details.
	const std::string details = shapeNames.at(tex->shape) + " (" + tex->gpu->descriptor().string() + ")";
	infos.displayName		  = " - " + details + "##" + std::to_string(_winId++);
}


void DebugViewer::untrack(const Texture * tex) {
	auto end = std::remove_if(_textures.begin(), _textures.end(), [tex](const Infos & infos) {
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

void DebugViewer::interface() {
	if(_silent) {
		return;
	}

	// Display menu bar listing all resources.
	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("Textures")) {
			for(Infos & tex : _textures) {
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
					for(Infos & tex : buffer.attachments) {
						ImGui::MenuItem(tex.name.c_str(), nullptr, &tex.visible);
					}
					ImGui::EndMenu();
				}
				ImGui::PopID();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// Display all active windows.
	for(int tid = 0; tid < _textures.size(); ++tid) {
		Infos & tex = _textures[tid];
		if(!tex.visible) {
			continue;
		}
		displayTexture(tex, "");
	}
	for(FramebufferInfos & buffer : _framebuffers) {
		for(Infos & tex : buffer.attachments) {
			if(!tex.visible) {
				continue;
			}
			displayTexture(tex, buffer.name + " - ");
		}
	}
}

void DebugViewer::displayTexture(Infos & tex, const std::string & prefix) {
	float aspect = float(tex.tex->width) / std::max(float(tex.tex->height), 1.0f);
	if(tex.tex->shape & TextureShape::Cube) {
		aspect = 2.0f;
	}
	// Fixed width, height takes into account texture aspect ratio and upper settings bar.
	const float defaultWidth = 550.0f;
	ImGui::SetNextWindowSize(ImVec2(defaultWidth, defaultWidth / aspect + 75.0f), ImGuiCond_Once);
	const std::string finalWinName = prefix + tex.name + tex.displayName;
;
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

void DebugViewer::updateDisplay(const Infos & tex) {

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

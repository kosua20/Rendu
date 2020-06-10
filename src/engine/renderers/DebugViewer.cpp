#include "DebugViewer.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/System.hpp"

DebugViewer::DebugViewer(bool silent){
	_texDisplay = Resources::manager().getProgram2D("debug_texture_display");
	_silent		= silent;
}

void DebugViewer::track(const std::string& name, const Texture* tex) {
	if(_silent) {
		return;
	}
	if(!tex->gpu) {
		Log::Warning() << "[DebugViewer] \"" << name << "\" has no GPU data." << std::endl;
		return;
	}
	_textures.emplace_back();
	registerTexture(name, tex, _textures.back());

	// Sort textures list.
	std::sort(_textures.begin(), _textures.end(), [](const Infos & a, const Infos & b) {
		return a.name < b.name;
	});
}

void DebugViewer::track(const std::string & name, const Framebuffer * buffer) {
	if(_silent) {
		return;
	}
	_framebuffers.emplace_back();
	FramebufferInfos & infos = _framebuffers.back();
	infos.name				 = name;

	// Register color attachments.
	for(uint cid = 0; cid < buffer->attachments(); ++cid) {
		const std::string nameAttach = name + " - Color " + std::to_string(cid); 
		infos.attachments.emplace_back();
		registerTexture(nameAttach, buffer->texture(cid), infos.attachments.back());
	}
	// Register depth attachment if it's a texture.
	const Texture * depthAttach = buffer->depthBuffer();
	if(depthAttach) {
		const std::string nameAttach = name + " - Depth";
		infos.attachments.emplace_back();
		registerTexture(nameAttach, depthAttach, infos.attachments.back());
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
	infos.display.reset(new Framebuffer(TextureShape::D2, tex->width, tex->height, 1, 1, {desc}, false));

	// Build display full name with details.
	static const std::map<TextureShape, std::string> shapeNames = {
		{TextureShape::D1, "1D"},
		{TextureShape::Array1D, "1D array"},
		{TextureShape::D2, "2D"},
		{TextureShape::Array2D, "2D array"},
		{TextureShape::Cube, "Cube"},
		{TextureShape::ArrayCube, "Cube array"},
		{TextureShape::D3, "3D"}};

	const std::string details = shapeNames.at(tex->shape) + " (" + tex->gpu->descriptor().string() + ")";
	infos.displayName		  = name + " - " + details;
}

void DebugViewer::interface() {
	if(_silent) {
		return;
	}
	// Display menu bar listing all resources.
	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("Textures")) {
			for(Infos & tex : _textures) {
				ImGui::MenuItem(tex.name.c_str(), nullptr, &tex.visible);
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Framebuffers")) {
			for(FramebufferInfos & buffer : _framebuffers) {
				if(ImGui::BeginMenu(buffer.name.c_str())) {
					for(Infos & tex : buffer.attachments) {
						ImGui::MenuItem(tex.name.c_str(), nullptr, &tex.visible);
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// Display all active windows.
	for(Infos & tex : _textures) {
		if(!tex.visible) {
			continue;
		}
		displayTexture(tex);
	}
	for(FramebufferInfos & buffer : _framebuffers) {
		for(Infos & tex : buffer.attachments) {
			if(!tex.visible) {
				continue;
			}
			displayTexture(tex);
		}
	}


}

void DebugViewer::clean() {
	for(Infos & tex : _textures) {
		tex.display->clean();
	}
	for(FramebufferInfos & buffer : _framebuffers) {
		for(Infos & tex : buffer.attachments) {
			tex.display->clean();
		}
	}
}

void DebugViewer::displayTexture(Infos & tex) {
	float aspect = float(tex.tex->width) / std::max(float(tex.tex->height), 1.0f);
	if(tex.tex->shape & TextureShape::Cube) {
		aspect = 2.0f;
	}
	// Fixed width, height takes into account texture aspect ratio and upper settings bar.
	const float defaultWidth = 550.0f;
	ImGui::SetNextWindowSize(ImVec2(defaultWidth, defaultWidth / aspect + 75.0f), ImGuiCond_Once);
	if(ImGui::Begin(tex.displayName.c_str(), &tex.visible)) {
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

#include "resources/Texture.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "renderers/DebugViewer.hpp"
#include <imgui/imgui_impl_vulkan.h>

Texture::Texture(const std::string & name) : _name(name) {
}

void Texture::upload(const Descriptor & layout, bool updateMipmaps) {

	// Compute the last mip level if needed.
	if(updateMipmaps) {
		levels = getMaxMipLevel()+1;
	}

	// Create texture.
	GPU::setupTexture(*this, layout, false);
	GPU::uploadTexture(*this);

	// Generate mipmaps pyramid automatically.
	if(updateMipmaps) {
		GPU::generateMipMaps(*this);
	}

	// Track in debug mode.
	DebugViewer::trackDefault(this);
}

uint Texture::getMaxMipLevel() const {
	uint minDimension = width;
	if(shape & TextureShape::D2){
		minDimension = std::min(minDimension, height);
	}
	if(shape & TextureShape::D3){
		minDimension = std::min(minDimension, height);
		minDimension = std::min(minDimension, depth);
	}
	return uint(std::floor(std::log2(minDimension)));
}

void Texture::clearImages() {
	images.clear();
}

void Texture::clean() {
	clearImages();
	if(gpu) {
		DebugViewer::untrackDefault(this);
		gpu->clean();
	}
	gpu = nullptr;
}

glm::vec3 Texture::sampleCubemap(const glm::vec3 & dir) const {
	// Images are stored in the following order:
	// px, nx, py, ny, pz, nz
	const glm::vec3 abs = glm::abs(dir);
	int side = 0;
	float x  = 0.0f;
	float y  = 0.0f;
	float denom = 1.0f;
	if(abs.x >= abs.y && abs.x >= abs.z) {
		denom = abs.x;
		y	  = dir.y;
		// X faces.
		if(dir.x >= 0.0f) {
			side = 0;
			x	 = -dir.z;
		} else {
			side = 1;
			x	 = dir.z;
		}
		
	} else if(abs.y >= abs.x && abs.y >= abs.z) {
		denom = abs.y;
		x	  = dir.x;
		// Y faces.
		if(dir.y >= 0.0f) {
			side = 2;
			y	 = -dir.z;
		} else {
			side = 3;
			y	 = dir.z;
		}
	} else if(abs.z >= abs.x && abs.z >= abs.y) {
		denom = abs.z;
		y	  = dir.y;
		// Z faces.
		if(dir.z >= 0.0f) {
			side = 4;
			x	 = dir.x;
		} else {
			side = 5;
			x	 = -dir.x;
		}
	}
	x = 0.5f * (x / denom) + 0.5f;
	y = 0.5f * (-y / denom) + 0.5f;
	// Ensure seamless borders between faces by never sampling closer than one pixel to the edge.
	const float eps = 1.0f / float(std::min(images[side].width, images[side].height));
	x				= glm::clamp(x, 0.0f + eps, 1.0f - eps);
	y				= glm::clamp(y, 0.0f + eps, 1.0f - eps);
	return images[side].rgbl(x, y);
}

const std::string & Texture::name() const {
	return _name;
}

void ImGui::Image(const Texture & texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col){
	if(texture.gpu->imgui == VK_NULL_HANDLE){
		texture.gpu->imgui = ImGui_ImplVulkan_AddTexture(texture.gpu->sampler, texture.gpu->view, texture.gpu->defaultLayout);
	}
	ImGui::Image((ImTextureID)texture.gpu->imgui, size, uv0, uv1, tint_col, border_col);
}

bool ImGui::ImageButton(const Texture & texture, const ImVec2& size, const ImVec2& uv0,  const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col){
	if(texture.gpu->imgui == VK_NULL_HANDLE){
		texture.gpu->imgui = ImGui_ImplVulkan_AddTexture(texture.gpu->sampler, texture.gpu->view, texture.gpu->defaultLayout);
	}
	return ImGui::ImageButton((ImTextureID)texture.gpu->imgui, size, uv0, uv1, frame_padding, bg_col, tint_col);
}

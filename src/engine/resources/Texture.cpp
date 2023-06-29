#include "resources/Texture.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "graphics/SamplerLibrary.hpp"
#include "renderers/DebugViewer.hpp"
#include <imgui/imgui_impl_vulkan.h>

Texture::Texture(const std::string & name) : _name(name) {
}

void Texture::upload(const Layout & layout, bool updateMipmaps) {

	// Compute the last mip level if needed.
	if(updateMipmaps) {
		levels = getMaxMipLevel()+1;
	}
	format = layout;
	drawable = false;

	// Create texture.
	GPU::setupTexture(*this);
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

void Texture::allocateImages(uint channels, uint firstMip, uint mipCount){

	const uint effectiveFirstMip = std::min(levels - 1u, firstMip);
	const uint effectiveMipCount = std::min(mipCount, levels - effectiveFirstMip);
	const uint effectiveLastMip = effectiveFirstMip + effectiveMipCount - 1u;

	const bool is3D = shape & TextureShape::D3;

	uint totalCount = 0;
	uint currentCount = 0;
	for(uint mid = 0; mid < levels; ++mid){
		if(mid == effectiveFirstMip){
			currentCount = totalCount;
		}
		totalCount += is3D ? std::max<uint>(depth >> mid, 1u) : depth;
	}

	images.resize(totalCount);

	for(uint mid = effectiveFirstMip; mid <= effectiveLastMip; ++mid){
		const uint w = std::max<uint>(width >> mid, 1u);
		const uint h = std::max<uint>(height >> mid, 1u);
		// Compute the size and count of images.
		const uint imageCount = is3D ? std::max<uint>(depth >> mid, 1u) : depth;
		for(uint iid = 0; iid < imageCount; ++iid){
			const uint imageIndex = currentCount + iid;
			// Avoid reallocating existing images.
			if(images[imageIndex].components != channels){
				images[imageIndex] = Image(w, h, channels);
			}
		}
		currentCount += imageCount;
	}
}

void Texture::clean() {
	clearImages();
	if(gpu) {
		DebugViewer::untrackDefault(this);
		gpu->clean();
	}
	gpu = nullptr;
}

Texture::Texture(Texture &&) = default;

Texture::~Texture(){
	clean();
}

void Texture::setupAsDrawable(const Layout& layout, uint awidth, uint aheight, TextureShape ashape, uint amips, uint adepth){

	// Check that the shape is supported.
	if(ashape != TextureShape::D2 && ashape != TextureShape::Array2D && ashape != TextureShape::Cube && ashape != TextureShape::ArrayCube){
		Log::Error() << "GPU: Unsupported render texture shape." << std::endl;
		return;
	}

	// Number of layers based on shape.
	uint layers = 1;
	if(ashape == TextureShape::Array2D){
		layers = adepth;
	} else if(ashape == TextureShape::Cube){
		layers = 6;
	} else if(ashape == TextureShape::ArrayCube){
		layers = 6 * adepth;
	}

	width  = awidth;
	height = aheight;
	depth  = layers;
	levels = amips;
	shape  = ashape;
	format = layout;
	drawable = true;

	GPU::setupTexture(*this);

	// Track in debug mode.
	DebugViewer::trackDefault(this);
}

void Texture::resize(const glm::vec2& resolution){
	resize(uint(resolution[0]), uint(resolution[1]));
}
void Texture::resize(uint w, uint h){
	width = w;
	height = h;
	GPU::setupTexture(*this);
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
		GPUContext* context = GPU::getInternal();
		texture.gpu->imgui = ImGui_ImplVulkan_AddTexture(context->samplerLibrary.getDefaultSampler(), texture.gpu->view, texture.gpu->defaultLayout);
	}
	ImGui::Image((ImTextureID)texture.gpu->imgui, size, uv0, uv1, tint_col, border_col);
}

bool ImGui::ImageButton(const char* id, const Texture & texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bg_col, const ImVec4& tint_col){
	if(texture.gpu->imgui == VK_NULL_HANDLE){
		GPUContext* context = GPU::getInternal();
		texture.gpu->imgui = ImGui_ImplVulkan_AddTexture(context->samplerLibrary.getDefaultSampler(), texture.gpu->view, texture.gpu->defaultLayout);
	}
	return ImGui::ImageButton(id, (ImTextureID)texture.gpu->imgui, size, uv0, uv1, bg_col, tint_col);
}

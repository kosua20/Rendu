#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/Buffer.hpp"

GPUTexture::GPUTexture(const Layout & layoutFormat) :
	typedFormat(layoutFormat) {

	channels = VkUtils::getGPULayout(layoutFormat, format);

	const bool isDepth = layoutFormat == Layout::DEPTH_COMPONENT16 || layoutFormat == Layout::DEPTH_COMPONENT24 || layoutFormat == Layout::DEPTH_COMPONENT32F || layoutFormat == Layout::DEPTH24_STENCIL8 || layoutFormat == Layout::DEPTH32F_STENCIL8;
	const bool isStencil = layoutFormat == Layout::DEPTH24_STENCIL8 || layoutFormat == Layout::DEPTH32F_STENCIL8;

	aspect = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	if(isStencil){
		aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
}

void GPUTexture::clean() {
	if(owned){
		GPU::clean(*this);
	}
}

unsigned int GPUTexture::getChannelsCount(const Layout& format){
	VkFormat formatDummy;
	return VkUtils::getGPULayout(format, formatDummy);
}

bool GPUTexture::isSRGB(const Layout& format){
	return format == Layout::SRGB8_ALPHA8 || format == Layout::SBGR8_ALPHA8;
}

GPUBuffer::GPUBuffer(BufferType atype){
	mappable = (atype == BufferType::UNIFORM || atype == BufferType::CPUTOGPU || atype == BufferType::GPUTOCPU);
}

void GPUBuffer::clean(){
	GPU::clean(*this);
}

void GPUMesh::clean() {
	if(vertexBuffer){
		vertexBuffer->clean();
	}
	if(indexBuffer){
		indexBuffer->clean();
	}
	vertexBuffer.reset();
	indexBuffer.reset();
	count = 0;
	
	GPU::clean(*this);
}

bool GPUMesh::isEquivalent(const GPUMesh& other) const {
	return state.isEquivalent(other.state);
}

bool GPUMesh::State::isEquivalent(const GPUMesh::State& other) const {
	if(bindings.size() != other.bindings.size()){
		return false;
	}
	if(attributes.size() != other.attributes.size()){
		return false;
	}
	const size_t bindingCount = bindings.size();
	const size_t attributeCount = attributes.size();

	for(size_t bid = 0; bid < bindingCount; ++bid){
		const auto& bind = bindings[bid];
		const auto& obind = other.bindings[bid];
		if((bind.binding != obind.binding) ||
			(bind.stride != obind.stride) ||
			(bind.inputRate != obind.inputRate)){
			return false;
		}
	}

	for(size_t aid = 0; aid < attributeCount; ++aid){
		const auto& attr = attributes[aid];
		const auto& ottr = other.attributes[aid];
		if((attr.binding != ottr.binding) ||
		   (attr.format != ottr.format) ||
		   (attr.location != ottr.location) ||
		   (attr.offset != ottr.offset)){
			return false;
		}
	}
	return true;
}

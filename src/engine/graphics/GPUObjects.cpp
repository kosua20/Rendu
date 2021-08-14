#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "graphics/Framebuffer.hpp"

GPUTexture::GPUTexture(const Descriptor & texDescriptor) :
	_descriptor(texDescriptor) {

	channels = VkUtils::getGPULayout(texDescriptor.typedFormat(), format);
	wrapping = VkUtils::getGPUWrapping(texDescriptor.wrapping());
	VkUtils::getGPUFilter(texDescriptor.filtering(), imgFiltering, mipFiltering);

	const Layout layout = texDescriptor.typedFormat();
	const bool isDepth = layout == Layout::DEPTH_COMPONENT16 || layout == Layout::DEPTH_COMPONENT24 || layout == Layout::DEPTH_COMPONENT32F || layout == Layout::DEPTH24_STENCIL8 || layout == Layout::DEPTH32F_STENCIL8;
	const bool isStencil = layout == Layout::DEPTH24_STENCIL8 || layout == Layout::DEPTH32F_STENCIL8;

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

bool GPUTexture::hasSameLayoutAs(const Descriptor & other) const {
	return _descriptor == other;
}

void GPUTexture::setFiltering(Filter filtering) {
	(void)filtering;
	_descriptor  = Descriptor(_descriptor.typedFormat(), filtering, _descriptor.wrapping());
	// Update sampler parameters.
	wrapping = VkUtils::getGPUWrapping(_descriptor.wrapping());
	VkUtils::getGPUFilter(_descriptor.filtering(), imgFiltering, mipFiltering);
	// Recreate sampler.
	GPU::setupSampler(*this);
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

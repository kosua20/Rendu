#include "resources/Buffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

#include <cstring>

BufferBase::BufferBase(size_t sizeInBytes, BufferType atype, DataUse ausage) : type(atype), usage(ausage) {
	// Size depends on the usage.
	int multipler = 1;
	if(usage == DataUse::FRAME){
		multipler = 2;
	} else if(usage == DataUse::DYNAMIC){
		multipler = 1024;
	}
	// Assume an alignment of 256.
	const uint alignedSize = (sizeInBytes + 256 - 1) & ~(256 - 1);
	if(multipler > 1){
		sizeMax = multipler * alignedSize;
	} else {
		sizeMax = sizeInBytes;
	}

}

void BufferBase::clean() {
	if(gpu) {
		gpu->clean();
		gpu.reset();
	}
	gpu = nullptr;
}

BufferBase::~BufferBase() {
	clean();
}

TransferBuffer::TransferBuffer(size_t sizeInBytes, BufferType atype) :
	BufferBase(sizeInBytes, atype, DataUse::STATIC) {
		GPU::setupBuffer(*this);
}

void TransferBuffer::upload(size_t sizeInBytes, unsigned char * data, size_t offset){
	// If the GPU object is not allocated, do it first.
	if(!gpu){
		GPU::setupBuffer(*this);
	}
	// Then upload the data in one block.
	GPU::uploadBuffer(*this, sizeInBytes, data, offset);
}

void TransferBuffer::download(size_t sizeInBytes, unsigned char * data, size_t offset){
	if(!gpu){
		Log::Warning() << "No GPU data to download for the buffer." << std::endl;
		return;
	}
	GPU::downloadBuffer(*this, sizeInBytes, data, offset);
}

UniformBufferBase::UniformBufferBase(size_t sizeInBytes, DataUse use) : BufferBase(sizeInBytes, BufferType::UNIFORM, use), _baseSize(sizeInBytes)
{
	// Immediatly setup and allocate the GPU buffer.
	GPU::setupBuffer(*this);
	// Then map permanently the buffer.
	GPUContext* context = GPU::getInternal();
	
	_alignment = (sizeInBytes + context->uniformAlignment - 1) & ~(context->uniformAlignment - 1);
	_offset = sizeMax;
}

void UniformBufferBase::upload(unsigned char * data){
	// Move to the next copy in the buffer, warpping around.
	_offset += _alignment;
	if(_offset + _baseSize >= sizeMax){
		_offset = 0;
	}

	// Copy data.
	std::memcpy(gpu->mapped + _offset, data, _baseSize);

	GPU::flushBuffer(*this, _offset, _baseSize);

}

void UniformBufferBase::clean(){
	BufferBase::clean();
}

UniformBufferBase::~UniformBufferBase(){
	UniformBufferBase::clean();
}

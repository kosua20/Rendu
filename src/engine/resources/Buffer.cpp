#include "resources/Buffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

#include <cstring>



Buffer::Buffer(size_t sizeInBytes, BufferType atype) : type(atype), size(sizeInBytes) {
	GPU::setupBuffer(*this);
}

Buffer::Buffer(BufferType atype) : type(atype), size(0u) {
	// Don't set it up immediately.
}

void Buffer::clean() {
	if(gpu) {
		gpu->clean();
		gpu.reset();
	}
	gpu = nullptr;
}

Buffer::~Buffer() {
	clean();
}

void Buffer::upload(size_t sizeInBytes, unsigned char * data, size_t offset){
	// If the GPU object is not allocated, do it first.
	if(!gpu){
		GPU::setupBuffer(*this);
	}
	// Then upload the data in one block.
	GPU::uploadBuffer(*this, sizeInBytes, data, offset);
}

void Buffer::download(size_t sizeInBytes, unsigned char * data, size_t offset){
	if(!gpu){
		Log::Warning() << "No GPU data to download for the buffer." << std::endl;
		return;
	}
	GPU::downloadBufferSync(*this, sizeInBytes, data, offset);
}


UniformBufferBase::UniformBufferBase(size_t sizeInBytes, UniformFrequency use) : Buffer(BufferType::UNIFORM), _baseSize(sizeInBytes)
{
	// Number of instances of the buffer stored internally, based on usage.
	int multipler = 1;
	if(use == UniformFrequency::FRAME){
		multipler = 2;
	} else if(use == UniformFrequency::VIEW){
		multipler = 64;
	} else if(use == UniformFrequency::DYNAMIC){
		multipler = 1024;
	}

	// Compute expected alignment.
	GPUContext* context = GPU::getInternal();
	_alignment = (sizeInBytes + context->uniformAlignment - 1) & ~(context->uniformAlignment - 1);

	// Total size.
	if(multipler > 1){
		size = multipler * _alignment;
	} else {
		size = sizeInBytes;
	}

	// Immediatly setup and allocate the GPU buffer.
	GPU::setupBuffer(*this);

	// Place ourselves at the end, to artificially end up at the beginning at the first upload.
	_offset = size;
}

void UniformBufferBase::upload(unsigned char * data){
	// Move to the next copy in the buffer, warpping around.
	_offset += _alignment;
	if(_offset + _baseSize >= size){
		_offset = 0;
	}

	// Copy data.
	std::memcpy(gpu->mapped + _offset, data, _baseSize);

	GPU::flushBuffer(*this, _offset, _baseSize);

}

void UniformBufferBase::clean(){
	Buffer::clean();
}

UniformBufferBase::~UniformBufferBase(){
	UniformBufferBase::clean();
}

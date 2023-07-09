#include "resources/Buffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

#include <cstring>



Buffer::Buffer(size_t sizeInBytes, BufferType atype, const std::string & name) : type(atype), size(sizeInBytes), _name(name) {
	GPU::setupBuffer(*this);
}

Buffer::Buffer(BufferType atype, const std::string & name) : type(atype), size(0u), _name(name) {
	// Don't set it up immediately.
}

void Buffer::clean() {
	if(gpu) {
		gpu->clean();
		gpu.reset();
	}
	gpu = nullptr;
}

const std::string & Buffer::name() const {
	return _name;
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


UniformBufferBase::UniformBufferBase(size_t sizeInBytes, UniformFrequency use, const std::string& name) : Buffer(BufferType::UNIFORM, name), _baseSize(sizeInBytes)
{
	// Number of instances of the buffer stored internally, based on usage.
	int multipler = 1;
	if(use == UniformFrequency::FRAME){
		multipler = 2;
	} else if(use == UniformFrequency::VIEW){
		multipler = 16;
		_wrapAround = false;
	} else if(use == UniformFrequency::DYNAMIC){
		multipler = 64;
		_wrapAround = false;
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

	// Place ourselves at the end, to artificially end up at the beginning at the first upload for wrap around buffers.
	// For pooled buffers, start at 0, we'll loose one slot at the first frame.
	_offset = _wrapAround ? size : 0;
}

bool UniformBufferBase::upload(unsigned char * data){

	bool newBuffer = false;
	// Move to the next copy in the buffer, wrapping around.
	_offset += _alignment;
	// Can we upload at the next offset?
	if(_offset + _baseSize >= size){
		// If we wrap around, just go back to the beginning of the buffer, assuming the data there won't be used at the current frame.
		// Otherwise, allocate a new GPU buffer. We'll have to notify the user about this (to update descriptor sets for instance).
		if(!_wrapAround){
			// Not enough room, allocate a new buffer.
			GPU::setupBuffer(*this);
			newBuffer = true;
		}
		_offset = 0;
	}

	// Copy data.
	std::memcpy(gpu->mapped + _offset, data, _baseSize);

	GPU::flushBuffer(*this, _offset, _baseSize);
	return newBuffer;

}

void UniformBufferBase::clean(){
	Buffer::clean();
}

UniformBufferBase::~UniformBufferBase(){
	UniformBufferBase::clean();
}

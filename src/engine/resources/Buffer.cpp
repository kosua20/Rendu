#include "resources/Buffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"

BufferBase::BufferBase(size_t sizeInBytes, BufferType atype, DataUse ausage) :
	sizeMax(sizeInBytes), type(atype), usage(ausage) {
}

void BufferBase::setup(){
	GLUtilities::setupBuffer(*this);
}

void BufferBase::upload(size_t sizeInBytes, unsigned char * data, size_t offset){
	// If the GPU object is not allocated, do it first.
	if(!gpu){
		setup();
	}
	if(sizeInBytes == sizeMax){
		// Orphan the buffer so that we don't need to wait for it to be unused before overwriting it.
		GLUtilities::allocateBuffer(*this);
	}
	// Then upload the data.
	GLUtilities::uploadBuffer(*this, sizeInBytes, data, offset);
}

void BufferBase::download(size_t sizeInBytes, unsigned char * data, size_t offset){
	if(!gpu){
		Log::Warning() << "No GPU data to download for the buffer." << std::endl;
		return;
	}
	GLUtilities::downloadBuffer(*this, sizeInBytes, data, offset);
}

void BufferBase::clean() {
	if(gpu) {
		gpu->clean();
		gpu.reset();
	}
}

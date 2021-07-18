#include "resources/Buffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

BufferBase::BufferBase(size_t sizeInBytes, BufferType atype, DataUse ausage) : type(atype), usage(ausage) {
	// Size depends on the usage.
	int multipler = 1;
	if(usage == DataUse::FRAME){
		multipler = 2;
	} else if(usage == DataUse::DYNAMIC){
		multipler = 1000;
	}
	sizeMax = multipler * sizeInBytes;
}

void BufferBase::clean() {
	if(gpu) {
		gpu->clean();
		gpu.reset();
	}
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
	void* tmpMap = nullptr;
	vkMapMemory(context->device, gpu->data, 0, sizeMax, 0, &tmpMap);
	_mappedData = (char*)tmpMap;
	_alignment = (sizeInBytes + context->uniformAlignment - 1) & ~(context->uniformAlignment - 1);
	_offset = sizeMax;
}

void UniformBufferBase::upload(unsigned char * data){
	// Move to the next copy in the buffer, warpping around.
	_offset += _alignment;
	if(_offset >= sizeMax){
		_offset = 0;
	}

	// Copy data.
	std::memcpy(_mappedData + _offset, data, _baseSize);

	GPUContext* context = GPU::getInternal();
	// Flush changes.
	VkMappedMemoryRange range{};
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.memory = gpu->data;
	size_t begin = _offset;
	size_t end = _offset + _baseSize - 1;
	// Round begin and end to multiples of nonCoherentAtomSize containing the range.
	const size_t mapAlign = context->mappingAlignment;
	begin = (begin / mapAlign) * mapAlign;
	const size_t targetEnd = (end / mapAlign) * mapAlign;
	end = targetEnd + (targetEnd != end ? mapAlign : 0);

	range.offset = begin;
	range.size = end + 1 - begin;

	vkFlushMappedMemoryRanges(context->device, 1, &range);


}

void UniformBufferBase::clean(){
	if(gpu){
		GPUContext* context = GPU::getInternal();
		vkUnmapMemory(context->device, gpu->data);
		BufferBase::clean();
	}
}

UniformBufferBase::~UniformBufferBase(){
	UniformBufferBase::clean();
}

#pragma once
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

/** \brief General purpose GPU/CPU buffer, without a CPU backing store.
 \details This allows the class to be template-free (to avoid exposing GPU/GPUObjects details
 in headers), while preserving the "pass general Object to GPU for setup/upload &
 GPUObject only stores ID and enums" approach followed by Texture and Mesh.
 In practice you will want to use Buffer to benefit from CPU storage and simplified upload.
 \ingroup Resources
 */
class BufferBase {
public:

	/** Constructor.
	\param sizeInBytes the size of the buffer in bytes
	\param atype the target of the buffer (uniform, index, vertex)
	\param ausage the update frequency of the buffer content
	 */
	BufferBase(size_t sizeInBytes, BufferType atype, DataUse ausage);

	/** Cleanup all data.
	 */
	void clean();

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	BufferBase & operator=(const BufferBase &) = delete;

	/** Copy constructor (disabled). */
	BufferBase(const BufferBase &) = delete;

	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	BufferBase & operator=(BufferBase &&) = delete;

	/** Move constructor. */
	BufferBase(BufferBase &&) = delete;

	/** Destructor. */
	virtual ~BufferBase();

	const BufferType type; ///< Buffer type.
	const DataUse usage; ///< Buffer update frequency.
	size_t sizeMax; ///< Buffer size in bytes.

	std::unique_ptr<GPUBuffer> gpu; ///< The GPU data (optional).

};

class TransferBuffer : public BufferBase {
public:

	TransferBuffer(size_t sizeInBytes, BufferType atype);

	/** Upload data to the buffer. You have to take care of synchronization if
	 updating a subregion of the buffer that is currently in use, except if
	 sizeInBytes == size of the buffer, in which case the current buffer is
	 orphaned and a new one used (if the driver is nice).
	 \param sizeInBytes the size of the data to upload, in bytes
	 \param data the data to upload
	 \param offset offset in the buffer
	 */
	void upload(size_t sizeInBytes, unsigned char * data, size_t offset);

	/** Download data from the buffer.
	 \param sizeInBytes the size of the data to download, in bytes
	 \param data the storage to download to
	 \param offset offset in the buffer
	 */
	void download(size_t sizeInBytes, unsigned char * data, size_t offset);

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	TransferBuffer & operator=(const TransferBuffer &) = delete;

	/** Copy constructor (disabled). */
	TransferBuffer(const TransferBuffer &) = delete;

	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	TransferBuffer & operator=(TransferBuffer &&) = delete;

	/** Move constructor. */
	TransferBuffer(TransferBuffer &&) = delete;

};

class UniformBufferBase : public BufferBase {
public:

	UniformBufferBase(size_t sizeInBytes, DataUse use);

	void upload(unsigned char * data);

	void clean();

	size_t currentOffset() const { return _offset; }

	size_t baseSize() const { return _baseSize; }

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	UniformBufferBase & operator=(const UniformBufferBase &) = delete;

	/** Copy constructor (disabled). */
	UniformBufferBase(const UniformBufferBase &) = delete;

	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	UniformBufferBase & operator=(UniformBufferBase &&) = delete;

	/** Move constructor. */
	UniformBufferBase(UniformBufferBase &&) = delete;

	~UniformBufferBase();

private:

	const size_t _baseSize;
	size_t _alignment = 0;
	size_t _offset = 0;
	char* _mappedData = nullptr;
};

/**
 \brief Represents a buffer containing arbitrary data, stored on the CPU and/or GPU.
 \ingroup Resources
 */
template<typename T>
class UniformBuffer : public UniformBufferBase {
public:

	/** Constructor.
	 \param count the number of elements
	 \param usage the update frequency of the buffer content
	 */
	UniformBuffer(size_t count, DataUse usage);

	/** Accessor.
	 \param i the location of the item to retrieve
	 \return a reference to the item
	 */
	T & operator[](size_t i){
		return data[i];
	}

	/** Accessor.
	 \param i the location of the item to retrieve
	 \return a reference to the item
	 */
	const T & operator[](size_t i) const {
		return data[i];
	}

	/** Accessor.
	 \param i the location of the item to retrieve
	 \return a reference to the item
	 */
	T & at(size_t i){
		return data[i];
	}

	/** Accessor.
	 \param i the location of the item to retrieve
	 \return a reference to the item
	 */
	const T & at(size_t i) const {
		return data[i];
	}

	/** \return the CPU size of the buffer. */
	size_t size() const {
		return data.size();
	}

	/*T* data(){
		return data.data();
	}*/

	/** Send the buffer data to the GPU.
	 Previously uploaded content will potentially be erased.
	 */
	void upload();

	/** Send part of the buffer data to the GPU.
	 \param count number of elements to upload
	 \param offset location of the first element to upload
	*/
	//void upload(size_t offset, size_t count);

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	UniformBuffer & operator=(const UniformBuffer &) = delete;
	
	/** Copy constructor (disabled). */
	UniformBuffer(const UniformBuffer &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	UniformBuffer & operator=(UniformBuffer &&) = default;
	
	/** Move constructor. */
	UniformBuffer(UniformBuffer &&) = default;

	/** Destructor. */
	~UniformBuffer() = default;

	std::vector<T> data; ///< The CPU data.
	
};

template <typename T>
UniformBuffer<T>::UniformBuffer(size_t count, DataUse usage) :
	UniformBufferBase(count * sizeof(T), usage) {
	data.resize(count);
}

template <typename T>
void UniformBuffer<T>::upload() {
	UniformBufferBase::upload(reinterpret_cast<unsigned char*>(data.data()));
}
/*
template <typename T>
void Buffer<T>::upload(size_t offset, size_t count) {
	BufferBase::upload(count * sizeof(T), reinterpret_cast<unsigned char*>(data.data()), offset);
}

template <typename T>
void Buffer<T>::download() {
	// Resize to make sure that we have enough room.
	data.resize(sizeMax/sizeof(T));
	BufferBase::download(data.size() * sizeof(T), reinterpret_cast<unsigned char*>(data.data()), 0);
}

template <typename T>
void Buffer<T>::clearCPU() {
	data.clear();
}
 */

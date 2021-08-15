#pragma once
#include "graphics/GPUTypes.hpp"
#include "Common.hpp"

class GPUBuffer;

/** \brief General purpose GPU buffer, capable of handling different update frequency.
 The internal GPU buffer might be large enough to contain multiple copies of the buffer while respecting alignment constraints.
 \ingroup Resources
 */
class BufferBase {
public:

	/** Constructor.
	\param sizeInBytes the size of the buffer in bytes
	\param atype the target of the buffer (uniform, index, vertex)
	\param ausage the update frequency of the buffer content
	 \note Depending on the update frequency, multiple copies of the buffer might be allocated internally.
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

/** \brief Buffer used to transfer data from the CPU to the GPU or vice-versa, and to store static data such as vertex/index buffers.
 \ingroup Resources
 */
class TransferBuffer : public BufferBase {
public:

	/**
	 \param sizeInBytes the size of the transfer buffer
	 \param atype how the buffer will be used (CPU to GPU, vertex buffer,...)
	 */
	TransferBuffer(size_t sizeInBytes, BufferType atype);

	/** Upload data to the buffer. You have to take care of synchronization when
	 updating a subregion of the buffer that is currently in use.
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

	/** Destructor. */
	~TransferBuffer() = default;

};

/** \brief Uniform buffer exposed to all shader stages, that can be updated at varying frequencies.
 Multiple instances of the GPU data will be maintained internally.
 \details This allows the class to be template-free (to avoid exposing GPU/GPUObjects details
 in headers), while preserving the "pass general Object to GPU for setup/upload &
 GPUObject only stores handle and enums" approach followed by Texture and Mesh.
 In practice you will want to use UniformBuffer<T> to benefit from CPU storage and simplified upload.
 \ingroup Resources
 */
class UniformBufferBase : public BufferBase {
public:

	/** Constructor.
	 \param sizeInBytes size of the uniform buffer
	 \param use the update frequency
	 */
	UniformBufferBase(size_t sizeInBytes, DataUse use);

	/** Upload data. The buffer will internally copy the data (using the internal size)
	 to a region of mapped GPU memory. Buffering will be handled based on the update frequency.
	 \param data the data to copy
	 */
	void upload(unsigned char * data);

	/** Clean the buffer. */
	void clean();

	/** \return the current offset in bytes in the internal GPU buffer.*/
	size_t currentOffset() const { return _offset; }

	/** \return the size of one instance of the buffer */
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

	/** Destructor. */
	virtual ~UniformBufferBase();

private:

	const size_t _baseSize; ///< The buffer base size (one instance).
	size_t _alignment = 0; ///< The alignment constraint to respect between successive instances.
	size_t _offset = 0; ///< The current offset in bytes in the array of instances.

};

/**
 \brief Represents a buffer containing uniform data, stored on the CPU and GPU.
 Depending on the update frequency of the CPU data, the buffer will maintain one or multiple copies of the data on the GPU.
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

	/** Send the buffer data to the GPU.
	 Previously uploaded content will potentially be erased.
	 */
	void upload();

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

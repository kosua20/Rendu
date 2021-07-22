#include "graphics/GPU.hpp"
#include "graphics/ShaderCompiler.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/Texture.hpp"
#include "resources/Image.hpp"
#include "system/TextUtilities.hpp"
#include "system/Window.hpp"

#include "graphics/GPUInternal.hpp"
#include "graphics/PipelineCache.hpp"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#include <vma/vk_mem_alloc.h>
#pragma clang diagnostic pop

#include <sstream>
#include <GLFW/glfw3.h>

#include <set>

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

GPUContext _context;

PipelineCache _pipelineCache;
VmaAllocator _allocator = VK_NULL_HANDLE;
VmaVulkanFunctions _vulkanFunctions;

GPUContext* GPU::getInternal(){
	return &_context;
}

bool GPU::setup(const std::string & appName) {

	if(volkInitialize() != VK_SUCCESS){
		Log::Error() << Log::GPU << "Could not load Vulkan" << std::endl;
		return false;
	}

	bool debugEnabled = false;
#if defined(DEBUG) || defined(FORCE_DEBUG_VULKAN)
	// Only enable if the layers are supported.
	debugEnabled = VkUtils::checkLayersSupport(validationLayers) && VkUtils::checkExtensionsSupport({ VK_EXT_DEBUG_UTILS_EXTENSION_NAME });
#endif

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Rendu";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;

	// We have to tell Vulkan the extensions we need.
	const std::vector<const char*> extensions = VkUtils::getRequiredInstanceExtensions(debugEnabled);
	if(!VkUtils::checkExtensionsSupport(extensions)){
		Log::Error() << Log::GPU << "Unsupported extensions." << std::endl;
		return false;
	}
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();

	// Validation layers.
	instanceInfo.enabledLayerCount = 0;
	if(debugEnabled){
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}

	// Debug callbacks if supported.
	VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
	if(debugEnabled){
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = vkDebugCallback;
		debugInfo.pUserData = nullptr;
		instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
	}

	if(vkCreateInstance(&instanceInfo, nullptr, &_context.instance) != VK_SUCCESS){
		Log::Info() << Log::GPU << "Unable to create a Vulkan instance." << std::endl;
		return false;
	}

	volkLoadInstance(_context.instance);

	if(debugEnabled){
		VK_RET(vkCreateDebugUtilsMessengerEXT(_context.instance, &debugInfo, nullptr, &_context.debugMessenger));
		//vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
	}

	// Pick a physical device.
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(_context.instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_context.instance, &deviceCount, devices.data());

	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
	// Check which one is ok for our requirements.
	for(const auto& device : devices) {
		bool hasPortability = false;
		// We want a device with swapchain support.
		const bool supportExtensions = VkUtils::checkDeviceExtensionsSupport(device, deviceExtensions, hasPortability);
		// Ask for anisotropy and tessellation.
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);
		const bool hasFeatures = features.samplerAnisotropy && features.tessellationShader;

		if(supportExtensions && hasFeatures){
			// Prefere a discrete GPU if possible.
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);
			const bool isDiscrete = properties.deviceType;

			if(selectedDevice == VK_NULL_HANDLE || isDiscrete){
				selectedDevice = device;
				_context.portability = hasPortability;
			}
		}
	}

	if(selectedDevice == VK_NULL_HANDLE){
		Log::Error() << Log::GPU << "Unable to find proper physical device." << std::endl;
		return false;
	}

	_context.physicalDevice = selectedDevice;

	// Query a few infos.
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(_context.physicalDevice, &properties);
	if(!properties.limits.timestampComputeAndGraphics){
		Log::Warning() << Log::GPU << "Timestamp queries are not supported on the selected device." << std::endl;
	}

	_context.timestep = double(properties.limits.timestampPeriod);
	_context.uniformAlignment = properties.limits.minUniformBufferOffsetAlignment;
	_context.mappingAlignment = properties.limits.nonCoherentAtomSize;
	// minImageTransferGranularity is guaranteed to be (1,1,1) on graphics/compute queues

	if(!ShaderCompiler::init()){
		Log::Error() << Log::GPU << "Unable to initialize shader compiler." << std::endl;
		return false;
	}
	return true;
}

bool GPU::setupWindow(Window * window){
	// Create a surface.
	if(glfwCreateWindowSurface(_context.instance, window->_window, nullptr, &_context.surface) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create surface." << std::endl;
		return false;
	}
	// Query the available queues.
	uint graphicsIndex, presentIndex;
	bool found = VkUtils::getQueueFamilies(_context.physicalDevice, _context.surface, graphicsIndex, presentIndex);
	if(!found){
		Log::Error() << Log::GPU << "Unable to find compatible queue families." << std::endl;
		return false;
	}

	// Select queues.
	std::set<uint> families;
	families.insert(graphicsIndex);
	families.insert(presentIndex);

	float queuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueInfos;
	for(int queueFamily : families) {
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueInfos.push_back(queueInfo);
	}

	// Device setup.
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
	deviceInfo.pQueueCreateInfos = queueInfos.data();
	// Features we want.
	VkPhysicalDeviceFeatures features = {};
	features.samplerAnisotropy = VK_TRUE;
	features.tessellationShader = VK_TRUE;
	deviceInfo.pEnabledFeatures = &features;
	// Extensions.
	auto extensions = deviceExtensions;
	// If portability is available, we have to enabled it.
	if(_context.portability){
		extensions.push_back("VK_KHR_portability_subset");
	}
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	deviceInfo.ppEnabledExtensionNames = extensions.data();

	if(vkCreateDevice(_context.physicalDevice, &deviceInfo, nullptr, &_context.device) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create logical device." << std::endl;
		return false;
	}
	_context.graphicsId = graphicsIndex;
	_context.presentId = presentIndex;
	vkGetDeviceQueue(_context.device, graphicsIndex, 0, &_context.graphicsQueue);
	vkGetDeviceQueue(_context.device, presentIndex, 0, &_context.presentQueue);

	// Setup allocator.
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
	allocatorInfo.physicalDevice = _context.physicalDevice;
	allocatorInfo.device = _context.device;
	allocatorInfo.instance = _context.instance;

	_vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
	_vulkanFunctions.vkMapMemory = vkMapMemory;
	_vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
	_vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	_vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	_vulkanFunctions.vkFreeMemory = vkFreeMemory;

	_vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
	_vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
	_vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
	_vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;

	_vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
	_vulkanFunctions.vkCreateImage = vkCreateImage;
	_vulkanFunctions.vkDestroyImage = vkDestroyImage;
	_vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;

	_vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
	_vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	_vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;

	_vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
	_vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
	_vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
	_vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
	_vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;


	allocatorInfo.pVulkanFunctions = &_vulkanFunctions;
	vmaCreateAllocator(&allocatorInfo, &_allocator);


	// Create the command pool.
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if(vkCreateCommandPool(_context.device, &poolInfo, nullptr, &_context.commandPool) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create command pool." << std::endl;
		return false;
	}

	// Create basic vertex array for screenquad.
	{
		_quad.positions = {
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(3.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, 3.0f, 0.0f),
		};
		_quad.indices = {0, 1, 2};
		_quad.upload();
	}

	// Finally setup the swapchain.
	window->_swapchain.init(_context, window->_config);
	_context.mainRenderPass = window->_swapchain.getMainPass();
	
	// Create a pipeline cache.
	_pipelineCache.init();

	_context.descriptorAllocator.init(&_context, 1024);
	return true;
}

int GPU::checkError(const char * file, int line, const std::string & infos) {
//	const GLenum glErr = glGetError();
//	if(glErr != GL_NO_ERROR) {
//		const std::string filePath(file);
//		size_t pos = std::min(filePath.find_last_of('/'), filePath.find_last_of('\\'));
//		if(pos == std::string::npos) {
//			pos = 0;
//		}
//		Log::Error() << Log::GPU << "Error " << getGLErrorString(glErr) << " in " << filePath.substr(pos + 1) << " (" << line << ").";
//		if(!infos.empty()) {
//			Log::Error() << " Infos: " << infos;
//		}
//		Log::Error() << std::endl;
//		return 1;
//	}
	return 0;
}

void GPU::createProgram(Program& program, const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent, const std::string & debugInfos) {

	Log::Verbose() << Log::GPU << "Compiling " << debugInfos << "." << std::endl;
	
	std::string compilationLog;
	// If vertex program code is given, compile it.
	if(!vertexContent.empty()) {
		ShaderCompiler::compile(vertexContent, ShaderType::VERTEX, program.stage(ShaderType::VERTEX), compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Vertex shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If fragment program code is given, compile it.
	if(!fragmentContent.empty()) {
		ShaderCompiler::compile(fragmentContent, ShaderType::FRAGMENT, program.stage(ShaderType::FRAGMENT), compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Fragment shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If geometry program code is given, compile it.
	if(!geometryContent.empty()) {
		ShaderCompiler::compile(geometryContent, ShaderType::GEOMETRY, program.stage(ShaderType::GEOMETRY), compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Geometry shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If tesselation control program code is given, compile it.
	if(!tessControlContent.empty()) {
		ShaderCompiler::compile(tessControlContent, ShaderType::TESSCONTROL, program.stage(ShaderType::TESSCONTROL), compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Tessellation control shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If tessellation evaluation program code is given, compile it.
	if(!tessEvalContent.empty()) {
		ShaderCompiler::compile(tessEvalContent, ShaderType::TESSEVAL, program.stage(ShaderType::TESSEVAL), compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Tessellation evaluation shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}

	//checkGPUError();
}

void GPU::bindProgram(const Program & program){
	_state.program = (Program*)&program;
//	if(_state.program != program._id){
//		_state.program = program._id;
//		glUseProgram(program._id);
//		_metrics.programBindings += 1;
//	}
}

void GPU::bindFramebuffer(const Framebuffer & framebuffer){
//	_state.framebuffer = &framebuffer;
//	if(_state.drawFramebuffer != framebuffer._id){
//		_state.drawFramebuffer = framebuffer._id;
//		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer._id);
	//		_metrics.framebufferBindings += 1;
//	}
}

//void GPU::bindFramebuffer(const Framebuffer & framebuffer, Framebuffer::Mode mode){
	//if(mode == Framebuffer::Mode::WRITE && _state.drawFramebuffer != framebuffer._id){
	//	_state.drawFramebuffer = framebuffer._id;
	//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer._id);
	//	_metrics.framebufferBindings += 1;
	//} else if(mode == Framebuffer::Mode::READ && _state.readFramebuffer != framebuffer._id){
	//	_state.readFramebuffer = framebuffer._id;
	//	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer._id);
	//	_metrics.framebufferBindings += 1;
	//}

//}

void GPU::saveFramebuffer(const Framebuffer & framebuffer, const std::string & path, bool flip, bool ignoreAlpha) {

	// Don't alter the GPU state, this is a temporary action.
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer._id);

	//const std::unique_ptr<GPUTexture> & gpu = framebuffer.texture()->gpu;
	//GPU::savePixels(gpu->type, gpu->format, framebuffer.width(), framebuffer.height(), gpu->channels, path, flip, ignoreAlpha);
	
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, _state.readFramebuffer);
	//_metrics.framebufferBindings += 2;
}


void GPU::setupTexture(Texture & texture, const Descriptor & descriptor) {

	if(texture.gpu) {
		texture.gpu->clean();
	}

	texture.gpu.reset(new GPUTexture(descriptor, texture.shape));

	const bool is3D = texture.shape & TextureShape::D3;
	const bool isCube = texture.shape & TextureShape::Cube;
	const bool isArray = texture.shape & TextureShape::Array;

	const Layout & layout = descriptor.typedFormat();
	const bool isDepth = layout == Layout::DEPTH_COMPONENT16 || layout == Layout::DEPTH_COMPONENT24 || layout == Layout::DEPTH_COMPONENT32F || layout == Layout::DEPTH24_STENCIL8 || layout == Layout::DEPTH32F_STENCIL8;
	const bool isStencil = layout == Layout::DEPTH24_STENCIL8 || layout == Layout::DEPTH32F_STENCIL8;

	VkImageUsageFlags usage = (isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) | VK_IMAGE_USAGE_SAMPLED_BIT;
	// Create image.
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = texture.gpu->type;
	imageInfo.extent.width = static_cast<uint32_t>(texture.width);
	imageInfo.extent.height = static_cast<uint32_t>(texture.height);
	imageInfo.extent.depth = is3D ? texture.depth : 1;
	imageInfo.mipLevels = texture.levels;
	imageInfo.arrayLayers = (isCube || isArray) ? texture.depth : 1;
	imageInfo.format = texture.gpu->format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = texture.gpu->layout;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;
	if(isCube){
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	} else if(texture.shape == TextureShape::Array2D){
		imageInfo.flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
	}

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaCreateImage(_allocator, &imageInfo, &allocInfo, &(texture.gpu->image), &(texture.gpu->data), nullptr);

	// Create view.
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = texture.gpu->image;
	viewInfo.viewType = texture.gpu->viewType;
	viewInfo.format = texture.gpu->format;
	viewInfo.subresourceRange.aspectMask = texture.gpu->aspect;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = texture.levels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;

	if (vkCreateImageView(_context.device, &viewInfo, nullptr, &(texture.gpu->view)) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create image view." << std::endl;
		return;
	}

	// Create associated sampler.
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = texture.gpu->imgFiltering;
	samplerInfo.minFilter = texture.gpu->imgFiltering;
	samplerInfo.addressModeU = texture.gpu->wrapping;
	samplerInfo.addressModeV = texture.gpu->wrapping;
	samplerInfo.addressModeW = texture.gpu->wrapping;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = texture.gpu->mipFiltering;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = float(texture.levels);
	if (vkCreateSampler(_context.device, &samplerInfo, nullptr, &(texture.gpu->sampler)) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create a sampler." << std::endl;
	}


}

void GPU::uploadTexture(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(texture.images.empty()) {
		Log::Warning() << Log::GPU << "No images to upload." << std::endl;
		return;
	}

	// Sanity check the texture destination format.
	const unsigned int destChannels = texture.gpu->channels;
	if(destChannels != texture.images[0].components) {
		Log::Error() << Log::GPU << "Not enough values in source data for texture upload." << std::endl;
		return;
	}

	// Compute total texture size on the CPU.
	size_t totalSize = 0;
	for(const auto & img: texture.images) {
		// \todo Handle conversion to other formats.
		const size_t imgSize = img.pixels.size() * sizeof(float);
		totalSize += imgSize;
	}

	// Transfer the complete CPU image data to a staging buffer.
	TransferBuffer transferBuffer(totalSize, BufferType::CPUTOGPU);
	size_t currentOffset = 0;
	for(const auto & img: texture.images) {
		const size_t imgSize = img.pixels.size() * sizeof(float);
		std::memcpy(transferBuffer.gpu->mapped + currentOffset, img.pixels.data(), imgSize);
		currentOffset += imgSize;
	}
	// flush?

	VkCommandBuffer commandBuffer = VkUtils::startOneTimeCommandBuffer(_context);

	VkUtils::transitionImageLayout(commandBuffer, texture.gpu->image, texture.gpu->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture.levels, texture.depth);

	// Copy operation for each mip level that is available on the CPU.
	size_t currentImg = 0;
	currentOffset = 0;

	for(size_t mid = 0; mid < texture.levels; ++mid) {
		// How deep is the image for 3D textures.
		const size_t depth = texture.shape == TextureShape::D3 ? (texture.depth / (1 << mid)) : 1;
		// How many images in the mip level (for arrays and cubes)
		const size_t layers = texture.shape == TextureShape::D3 ? 1 : texture.depth;

		// First image of the mip level (they all have the same size.
		const Image & image = texture.images[currentImg];
		const size_t imgSize = image.pixels.size() * sizeof(float);

		// Perform copy for this mip level.
		VkBufferImageCopy region = {};
		region.bufferOffset = currentOffset;
		region.bufferRowLength = 0; // Tightly packed.
		region.bufferImageHeight = 0; // Tightly packed.
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = mid;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layers;
		// Offset *in the subregion*
		region.imageOffset = {0, 0, 0};
		region.imageExtent = { image.width, image.height, (uint32_t)depth};

		vkCmdCopyBufferToImage(commandBuffer, transferBuffer.gpu->buffer, texture.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		currentImg += depth;
		currentOffset += depth * imgSize;
		// We might have more levels allocated on the GPU than we had available on the CPU.
		// Stop, these will be generated automatically.
		if(currentImg >= texture.images.size()){
			break;
		}

	}

	//VkUtils::transitionImageLayout(commandBuffer, texture.gpu->image, texture.gpu->format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.levels, texture.depth);

	VkUtils::endOneTimeCommandBuffer(commandBuffer, _context);

	transferBuffer.clean();

}

void GPU::downloadTexture(Texture & texture) {
	downloadTexture(texture, -1);
}

void GPU::downloadTexture(Texture & texture, int level) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(texture.shape != TextureShape::D2 && texture.shape != TextureShape::Cube) {
		Log::Error() << Log::GPU << "Unsupported download format." << std::endl;
		return;
	}
	if(!texture.images.empty()) {
		Log::Verbose() << Log::GPU << "Texture already contain CPU data, will be erased." << std::endl;
	}
	texture.images.resize(texture.depth * texture.levels);
//
//	const GLenum target			= texture.gpu->target;
//	const GLenum type			= GL_FLOAT;
//	const GLenum format			= texture.gpu->format;
//	const unsigned int channels = texture.gpu->channels;
//
//	// We enforce float type, we can use 4 alignment.
//	glPixelStorei(GL_PACK_ALIGNMENT, 4);
//	_metrics.stateChanges += 1;
//	glBindTexture(target, texture.gpu->id);
//	_metrics.textureBindings += 1;
//
//	// For each mip level.
//	for(size_t mid = 0; mid < texture.levels; ++mid) {
//		if(level >= 0 && int(mid) != level) {
//			continue;
//		}
//		const GLsizei w = GLsizei(std::max<uint>(1, texture.width / (1 << mid)));
//		const GLsizei h = GLsizei(std::max<uint>(1, texture.height / (1 << mid)));
//		const GLint mip = GLint(mid);
//
//		if(texture.shape == TextureShape::D2) {
//			texture.images[mid] = Image(w, h, channels);
//			Image & image		= texture.images[mid];
//			glGetTexImage(GL_TEXTURE_2D, mip, format, type, &image.pixels[0]);
//			_metrics.downloads += 1;
//		} else if(texture.shape == TextureShape::Cube) {
//			for(size_t lid = 0; lid < texture.depth; ++lid) {
//				const size_t id	   = mid * texture.levels + lid;
//				texture.images[id] = Image(w, h, channels);
//				Image & image	   = texture.images[id];
//				glGetTexImage(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lid), mip, format, type, &image.pixels[0]);
//				_metrics.downloads += 1;
//			}
//		}
//	}
//	GPU::restoreTexture(texture.shape);
}

void GPU::generateMipMaps(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}
	// Do we support blitting?
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(_context.physicalDevice, texture.gpu->format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		Log::Error() << Log::GPU << "Bliting not supported for this format." << std::endl;
		return;
	}

	const bool isCube = texture.shape & TextureShape::Cube;
	const bool isArray = texture.shape & TextureShape::Array;
	const size_t layers = (isCube || isArray) ? texture.depth : 1;
	size_t width = texture.width;
	size_t height = texture.height;
	size_t depth = texture.shape == TextureShape::D3 ? texture.depth : 1;

	// Prepare barrier that we will reuse at each level.
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = texture.gpu->image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layers;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;

	// Blit the texture to each mip level.
	VkCommandBuffer commandBuffer = VkUtils::startOneTimeCommandBuffer(_context);

	// For now, don't bother with existing mip data (potentially uploaded from the CPU).
	for (size_t mid = 1; mid < texture.levels; mid++) {

		// Transition level i-1 to transfer layout.
		barrier.subresourceRange.baseMipLevel = mid - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,  0, nullptr, 1, &barrier);
		// Then, blit to level i.
		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { (int32_t)width, (int32_t)height, (int32_t)depth };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = mid - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = layers;
		blit.dstOffsets[0] = { 0, 0, 0 };

		// Divide all dimensions by 2 if possible.
		width  = (width  > 1 ? (width/2)  : 1);
		height = (height > 1 ? (height/2) : 1);
		depth  = (depth  > 1 ? (depth/2)  : 1);

		blit.dstOffsets[1] = { (int32_t)width, (int32_t)height, (int32_t)depth };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = mid;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = layers;
		// Blit using linear filtering for smoother downscaling.
		vkCmdBlitImage(commandBuffer, texture.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
		// Force sync and move previous layer to shader readable format.
		// \todo Could be done for all levels at once at the end ? but it has a different old layout.
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}
	// Transition the last level.
	barrier.subresourceRange.baseMipLevel = int(texture.levels)-1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	// Submit the commands.
	VkUtils::endOneTimeCommandBuffer(commandBuffer, _context);
}

void GPU::setupBuffer(BufferBase & buffer) {
	if(buffer.gpu) {
		buffer.gpu->clean();
	}
	// Create.
	buffer.gpu.reset(new GPUBuffer(buffer.type, buffer.usage));

	static const std::map<BufferType, VkBufferUsageFlags> types = {
		{ BufferType::VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT },
		{ BufferType::INDEX, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT },
		{ BufferType::UNIFORM, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT },
		{ BufferType::CPUTOGPU, VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
		{ BufferType::GPUTOCPU, VK_BUFFER_USAGE_TRANSFER_DST_BIT }};
	const VkBufferUsageFlags type = types.at(buffer.type);

	static const std::map<BufferType, VmaMemoryUsage> usages = {
		{ BufferType::VERTEX, VMA_MEMORY_USAGE_GPU_ONLY },
		{ BufferType::INDEX, VMA_MEMORY_USAGE_GPU_ONLY },
		{ BufferType::UNIFORM, VMA_MEMORY_USAGE_CPU_TO_GPU  },
		{ BufferType::CPUTOGPU, VMA_MEMORY_USAGE_CPU_ONLY },
		{ BufferType::GPUTOCPU, VMA_MEMORY_USAGE_GPU_TO_CPU }};

	const VmaMemoryUsage usage = usages.at(buffer.type);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = buffer.sizeMax;
	bufferInfo.usage = type;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = usage;
	if(buffer.gpu->mappable){
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	VmaAllocationInfo resultInfos = {};

	vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &(buffer.gpu->buffer), &(buffer.gpu->data), &resultInfos);

	if(buffer.gpu->mappable){
		buffer.gpu->mapped = (char*)resultInfos.pMappedData;
	} else {
		buffer.gpu->mapped = nullptr;
	}
	
}

void GPU::uploadBuffer(const BufferBase & buffer, size_t size, uchar * data, size_t offset) {
	if(!buffer.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU buffer." << std::endl;
		return;
	}

	if(size == 0) {
		Log::Warning() << Log::GPU << "No data to upload." << std::endl;
		return;
	}
	if(offset + size > buffer.sizeMax) {
		Log::Warning() << Log::GPU << "Not enough allocated space to upload." << std::endl;
		return;
	}

	// If the buffer is visible from the CPU side, we don't need an intermediate staging buffer.
	if(buffer.gpu->mappable){
		if(!buffer.gpu->mapped){
			vmaMapMemory(_allocator, buffer.gpu->data, (void**)&buffer.gpu->mapped);
		}
		std::memcpy(buffer.gpu->mapped + offset, data, size);
		flushBuffer(buffer, size, offset);
		return;
	}

	// Otherwise, create a transfer buffer.
	TransferBuffer transferBuffer(size, BufferType::CPUTOGPU);
	transferBuffer.upload(size, data, 0);
	// Copy operation.
	VkCommandBuffer commandBuffer = VkUtils::startOneTimeCommandBuffer(_context);
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = offset;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, transferBuffer.gpu->buffer, buffer.gpu->buffer, 1, &copyRegion);
	VkUtils::endOneTimeCommandBuffer(commandBuffer, _context);
	transferBuffer.clean();
}

void GPU::downloadBuffer(const BufferBase & buffer, size_t size, uchar * data, size_t offset) {
	if(!buffer.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU buffer." << std::endl;
		return;
	}
	if(offset + size > buffer.sizeMax) {
		Log::Warning() << Log::GPU << "Not enough available data to download." << std::endl;
		return;
	}

	// If the buffer is visible from the CPU side, we don't need an intermediate staging buffer.
	if(buffer.gpu->mappable){

		if(!buffer.gpu->mapped){
			vmaMapMemory(_allocator, buffer.gpu->data, (void**)&buffer.gpu->mapped);
		}
		vmaInvalidateAllocation(_allocator, buffer.gpu->data, offset, size);
		std::memcpy(data, buffer.gpu->mapped + offset, size);
		return;
	}

	// Otherwise, create a transfer buffer.
	TransferBuffer transferBuffer(size, BufferType::GPUTOCPU);

	// Copy operation.
	VkCommandBuffer commandBuffer = VkUtils::startOneTimeCommandBuffer(_context);
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = offset;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, buffer.gpu->buffer, transferBuffer.gpu->buffer, 1, &copyRegion);
	VkUtils::endOneTimeCommandBuffer(commandBuffer, _context);
	transferBuffer.download(size, data, 0);
	transferBuffer.clean();
}

void GPU::flushBuffer(const BufferBase & buffer, size_t size, size_t offset){
	if(buffer.gpu->mapped == nullptr){
		Log::Error() << Log::GPU << "Buffer is not mapped." << std::endl;
		return;
	}
	vmaFlushAllocation(_allocator, buffer.gpu->data, offset, size);
}

void GPU::setupMesh(Mesh & mesh) {
	if(mesh.gpu) {
		mesh.gpu->clean();
	}
	mesh.gpu.reset(new GPUMesh());


	// Compute full allocation size.
	size_t totalSize = 0;
	totalSize += 3 * mesh.positions.size();
	totalSize += 3 * mesh.normals.size();
	totalSize += 2 * mesh.texcoords.size();
	totalSize += 3 * mesh.tangents.size();
	totalSize += 3 * mesh.binormals.size();
	totalSize += 3 * mesh.colors.size();
	totalSize *= sizeof(float);

	// Create a staging buffer to host the geometry data (to avoid creating a staging buffer for each sub-upload).
	std::vector<uchar> vertexBufferData(totalSize);

	GPUMesh::InputState& state = mesh.gpu->state;
	state.attributes.clear();
	state.bindings.clear();
	state.offsets.clear();

	// Fill in subregions.

	struct AttribInfos {
		uchar* data;
		size_t size;
		size_t components;
	};

	const std::vector<AttribInfos> attribs = {
		{ reinterpret_cast<uchar*>(mesh.positions.data()), mesh.positions.size(), 3},
		{ reinterpret_cast<uchar*>(mesh.normals.data()), mesh.normals.size(), 3},
		{ reinterpret_cast<uchar*>(mesh.texcoords.data()), mesh.texcoords.size(), 2},
		{ reinterpret_cast<uchar*>(mesh.tangents.data()), mesh.tangents.size(), 3},
		{ reinterpret_cast<uchar*>(mesh.binormals.data()), mesh.binormals.size(), 3},
		{ reinterpret_cast<uchar*>(mesh.colors.data()), mesh.colors.size(), 3},
	};

	size_t offset = 0;
	uint location = 0;
	uint bindingIndex = 0;

	for(const AttribInfos& attrib : attribs){
		if(attrib.size == 0){
			++location;
			continue;
		}
		// Setup attribute.
		state.attributes.emplace_back();
		state.attributes.back().binding = bindingIndex;
		state.attributes.back().location = location;
		state.attributes.back().offset = 0;
		state.attributes.back().format = attrib.components == 3 ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32_SFLOAT;
		// Setup binding.
		const size_t elementSize = sizeof(float) * attrib.components;
		state.bindings.emplace_back();
		state.bindings.back().binding = bindingIndex;
		state.bindings.back().stride = elementSize;
		state.bindings.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		state.offsets.emplace_back(offset);
		// Copy data.
		const size_t size = elementSize * attrib.size;
		std::memcpy(vertexBufferData.data() + offset, attrib.data, size);
		offset += size;
		++bindingIndex;
		++location;
	}

	const size_t inSize = sizeof(unsigned int) * mesh.indices.size();

	// Upload data to the buffers. Staging will be handled internally.
	TransferBuffer vertexBuffer(totalSize, BufferType::VERTEX);
	TransferBuffer indexBuffer(inSize, BufferType::INDEX);

	vertexBuffer.upload(totalSize, vertexBufferData.data(), 0);
	indexBuffer.upload(inSize, reinterpret_cast<unsigned char *>(mesh.indices.data()), 0);

	// Replicate the buffer as many times as needed for each attribute.
	state.buffers.resize(state.offsets.size(), vertexBuffer.gpu->buffer);

	mesh.gpu->count		   = mesh.indices.size();
	mesh.gpu->indexBuffer  = std::move(indexBuffer.gpu);
	mesh.gpu->vertexBuffer = std::move(vertexBuffer.gpu);
}

void GPU::bindPipelineIfNeeded(){

	// Possibilities:
	// state is outdated, create/retrieve new pipeline
	bool shouldBindPipeline = _context.newRenderPass;
	_context.newRenderPass = false;
	// \todo Use hash for equivalence.
	if(!_state.isEquivalent(_lastState)){
		_context.pipeline = _pipelineCache.getPipeline(_state);
		_lastState = _state;
		shouldBindPipeline = true;
	}

	// if new render pass begun, bind pipeline
	// if pipeline updated, bind pipeline
	if(shouldBindPipeline){
		vkCmdBindPipeline(_context.getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _context.pipeline);
	}
}

void GPU::drawMesh(const Mesh & mesh) {
	_state.mesh = mesh.gpu.get();

	bindPipelineIfNeeded();

	_state.program->update();

	vkCmdBindVertexBuffers(_context.getCurrentCommandBuffer(), 0, mesh.gpu->state.offsets.size(), mesh.gpu->state.buffers.data(), mesh.gpu->state.offsets.data());
	vkCmdBindIndexBuffer(_context.getCurrentCommandBuffer(), mesh.gpu->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(_context.getCurrentCommandBuffer(), static_cast<uint32_t>(mesh.gpu->count), 1, 0, 0, 0);
}

void GPU::drawTesselatedMesh(const Mesh & mesh, uint patchSize){
	// \todo check if we need to specify the patch size or if it's specified in the shader
	drawMesh(mesh);
}

void GPU::drawQuad(){
	_state.mesh = _quad.gpu.get();

	bindPipelineIfNeeded();

	_state.program->update();

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(_context.getCurrentCommandBuffer(), 0, 1, &(_quad.gpu->vertexBuffer->buffer), &offset);
	vkCmdDraw(_context.getCurrentCommandBuffer(), 3, 1, 0, 0);
}

void GPU::sync(){
	vkDeviceWaitIdle(_context.device);
}

void GPU::nextFrame(){
	_context.nextFrame();
	_pipelineCache.freeOutdatedPipelines();
	// Save and reset stats.
	_metricsPrevious = _metrics;
	_metrics = Metrics();
}

void GPU::deviceInfos(std::string & vendor, std::string & renderer, std::string & version, std::string & shaderVersion) {
	vendor = renderer = version = shaderVersion = "";

	const std::unordered_map<uint32_t, std::string> vendors = {
		{ 0x1002, "AMD" }, { 0x10DE, "NVIDIA" }, { 0x8086, "INTEL" }, { 0x13B5, "ARM" }
	};

	if(_context.physicalDevice != VK_NULL_HANDLE){
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(_context.physicalDevice, &properties);

		const uint32_t vendorId = properties.vendorID;
		vendor = vendors.count(vendorId) ? vendors.at(vendorId) : std::to_string(vendorId);

		renderer = std::string(properties.deviceName);
		version = std::to_string(properties.driverVersion);

		const uint32_t vMaj = VK_VERSION_MAJOR(properties.apiVersion);
		const uint32_t vMin = VK_VERSION_MINOR(properties.apiVersion);
		const uint32_t vPat = VK_VERSION_PATCH(properties.apiVersion);
		shaderVersion = std::to_string(vMaj) + "." + std::to_string(vMin) + "." + std::to_string(vPat);
	}
}

std::vector<std::string> GPU::supportedExtensions() {
	std::vector<std::string> names;
	names.emplace_back("-- Instance ------");
	// Get available extensions.
	uint32_t instanceExtsCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtsCount, nullptr);
	std::vector<VkExtensionProperties> instanceExts(instanceExtsCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtsCount, instanceExts.data());
	for(const auto& ext : instanceExts){
		names.emplace_back(ext.extensionName);
	}
	// Layers too.
	names.emplace_back("-- Layers --------");
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	for(const auto& layer : availableLayers){
		names.emplace_back(layer.layerName);
	}
	// Get available device extensions.
	if(_context.physicalDevice != VK_NULL_HANDLE){
		names.emplace_back("-- Device --------");
		uint32_t deviceExtsCount;
		vkEnumerateDeviceExtensionProperties(_context.physicalDevice, nullptr, &deviceExtsCount, nullptr);
		std::vector<VkExtensionProperties> deviceExts(deviceExtsCount);
		vkEnumerateDeviceExtensionProperties(_context.physicalDevice, nullptr, &deviceExtsCount, deviceExts.data());
		for(const auto& ext : deviceExts){
			names.emplace_back(ext.extensionName);
		}
	}
	return names;
}

void GPU::setViewport(int x, int y, int w, int h) {
	VkViewport vp;
	vp.x = x;
	vp.y = y;
	vp.width = w;
	vp.height = h;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;
	vkCmdSetViewport(_context.getCurrentCommandBuffer(), 0, 1, &vp);
	VkRect2D scissor;
	scissor.offset.x = x;
	scissor.offset.y = y;
	scissor.extent.width = w;
	scissor.extent.height = h;
	vkCmdSetScissor(_context.getCurrentCommandBuffer(), 0, 1, &scissor);
}

void GPU::clearColor(const glm::vec4 & color) {
//	if(_state.colorClearValue != color){
//		_state.colorClearValue = color;
//		glClearColor(color[0], color[1], color[2], color[3]);
//		_metrics.stateChanges += 1;
//	}
//	glClear(GL_COLOR_BUFFER_BIT);
//	_metrics.clearAndBlits += 1;
}

void GPU::clearDepth(float depth) {
//	if(_state.depthClearValue != depth){
//		_state.depthClearValue = depth;
//		glClearDepth(depth);
//		_metrics.stateChanges += 1;
//	}
//	glClear(GL_DEPTH_BUFFER_BIT);
//	_metrics.clearAndBlits += 1;
}

void GPU::clearStencil(uchar stencil) {
	// The stencil mask applies to clearing.
	// Disable it temporarily.
//	if(!_state.stencilWriteMask){
//		glStencilMask(0xFF);
//		_metrics.stateChanges += 1;
//	}
//
//	if(_state.stencilClearValue != stencil){
//		_state.stencilClearValue = stencil;
//		glClearStencil(GLint(stencil));
//		_metrics.stateChanges += 1;
//	}
//	glClear(GL_STENCIL_BUFFER_BIT);
//	_metrics.clearAndBlits += 1;
//
//	if(!_state.stencilWriteMask){
//		glStencilMask(0x00);
//		_metrics.stateChanges += 1;
//	}
}

void GPU::clearColorAndDepth(const glm::vec4 & color, float depth) {
//	if(_state.colorClearValue != color){
	//	_state.colorClearValue = color;
//		glClearColor(color[0], color[1], color[2], color[3]);
//		_metrics.stateChanges += 1;
//	}
//	if(_state.depthClearValue != depth){
//		_state.depthClearValue = depth;
//		glClearDepth(depth);
//		_metrics.stateChanges += 1;
//	}
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	_metrics.clearAndBlits += 1;
}

void GPU::clearColorDepthStencil(const glm::vec4 & color, float depth, uchar stencil) {
	// The stencil mask applies to clearing.
	// Disable it temporarily.
//	if(!_state.stencilWriteMask){
//		glStencilMask(0xFF);
//		_metrics.stateChanges += 1;
//	}
//	if(_state.colorClearValue != color){
//		_state.colorClearValue = color;
//		glClearColor(color[0], color[1], color[2], color[3]);
//		_metrics.stateChanges += 1;
//	}
//	if(_state.depthClearValue != depth){
//		_state.depthClearValue = depth;
//		glClearDepth(depth);
//		_metrics.stateChanges += 1;
//	}
//	if(_state.stencilClearValue != stencil){
//		_state.stencilClearValue = stencil;
//		glClearStencil(GLint(stencil));
//		_metrics.stateChanges += 1;
//	}
//
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
//	_metrics.clearAndBlits += 1;
//
//	if(!_state.stencilWriteMask){
//		glStencilMask(0x00);
//		_metrics.stateChanges += 1;
//	}
}

void GPU::setDepthState(bool test) {
	_state.depthTest = test;
}

void GPU::setDepthState(bool test, TestFunction equation, bool write) {
	_state.depthTest = test;
	_state.depthFunc = equation;
	_state.depthWriteMask = write;
}

void GPU::setStencilState(bool test, bool write){
	_state.stencilTest = test;
	_state.stencilWriteMask = write;
}

void GPU::setStencilState(bool test, TestFunction function, StencilOp fail, StencilOp pass, StencilOp depthFail, uchar value){
	_state.stencilTest = test;
	_state.stencilFunc = function;
	_state.stencilWriteMask = true;
	_state.stencilFail = fail;
	_state.stencilPass = depthFail;
	_state.stencilDepthPass = pass;
}

void GPU::setBlendState(bool test) {
	_state.blend = test;
}

void GPU::setBlendState(bool test, BlendEquation equation, BlendFunction src, BlendFunction dst) {
	_state.blend = test;
	_state.blendEquationRGB = _state.blendEquationAlpha = equation;
	_state.blendSrcRGB = _state.blendSrcAlpha = src;
	_state.blendDstRGB = _state.blendDstAlpha = dst;
}

void GPU::setCullState(bool cull) {
	_state.cullFace = cull;
}

void GPU::setCullState(bool cull, Faces culledFaces) {
	_state.cullFace = cull;
	_state.cullFaceMode = culledFaces;
}

void GPU::setPolygonState(PolygonMode mode) {
	_state.polygonMode = mode;
}

void GPU::setColorState(bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha){
	_state.colorWriteMask.r = writeRed;
	_state.colorWriteMask.g = writeGreen;
	_state.colorWriteMask.b = writeBlue;
	_state.colorWriteMask.a = writeAlpha;
}

void GPU::blitDepth(const Framebuffer & src, const Framebuffer & dst) {
//	src.bind(Framebuffer::Mode::READ);
//	dst.bind(Framebuffer::Mode::WRITE);
//	glBlitFramebuffer(0, 0, src.width(), src.height(), 0, 0, dst.width(), dst.height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
//	_metrics.clearAndBlits += 1;
}

void GPU::blit(const Framebuffer & src, const Framebuffer & dst, Filter filter) {
//	src.bind(Framebuffer::Mode::READ);
//	dst.bind(Framebuffer::Mode::WRITE);
//	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
//	glBlitFramebuffer(0, 0, src.width(), src.height(), 0, 0, dst.width(), dst.height(), GL_COLOR_BUFFER_BIT, filterGL);
//	_metrics.clearAndBlits += 1;
}

void GPU::blit(const Framebuffer & src, const Framebuffer & dst, size_t lSrc, size_t lDst, Filter filter) {
	//GPU::blit(src, dst, lSrc, lDst, 0, 0, filter);
}

void GPU::blit(const Framebuffer & src, const Framebuffer & dst, size_t lSrc, size_t lDst, size_t mipSrc, size_t mipDst, Filter filter) {
//	src.bind(lSrc, mipSrc, Framebuffer::Mode::READ);
//	dst.bind(lDst, mipDst, Framebuffer::Mode::WRITE);
//	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
//	glBlitFramebuffer(0, 0, src.width() / (1 << mipSrc), src.height() / (1 << mipSrc), 0, 0, dst.width() / (1 << mipDst), dst.height() / (1 << mipDst), GL_COLOR_BUFFER_BIT, filterGL);
//	_metrics.clearAndBlits += 1;
}

void GPU::blit(const Texture & src, Texture & dst, Filter filter) {
	// Prepare the destination.
//	dst.width  = src.width;
//	dst.height = src.height;
//	dst.depth  = src.depth;
//	dst.levels = 1;
//	dst.shape  = src.shape;
//	if(src.levels != 1) {
//		Log::Warning() << Log::GPU << "Only the first mipmap level will be used." << std::endl;
//	}
//	if(!src.images.empty()) {
//		Log::Warning() << Log::GPU << "CPU data won't be copied." << std::endl;
//	}
//	GPU::setupTexture(dst, src.gpu->descriptor());
//
//	// Create two framebuffers.
//	GLuint srcFb, dstFb;
//	glGenFramebuffers(1, &srcFb);
//	glGenFramebuffers(1, &dstFb);
//	// Because these two are temporary and will be unbound at the end of the call
//	// we do not update the cached GPU state.
//	glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFb);
//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFb);
//	_metrics.framebufferBindings += 2;
//
//	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
//
//	if(src.shape == TextureShape::Cube) {
//		for(size_t i = 0; i < 6; ++i) {
//			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), src.gpu->id, 0);
//			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), dst.gpu->id, 0);
//			GPU::checkFramebufferStatus();
//			glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width, dst.height, GL_COLOR_BUFFER_BIT, filterGL);
//			_metrics.clearAndBlits += 1;
//		}
//	} else {
//		if(src.shape == TextureShape::D1) {
//			glFramebufferTexture1D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);
//			glFramebufferTexture1D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst.gpu->target, dst.gpu->id, 0);
//
//		} else if(src.shape == TextureShape::D2) {
//			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);
//			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst.gpu->target, dst.gpu->id, 0);
//
//		} else {
//			Log::Error() << Log::GPU << "Unsupported texture shape for blitting." << std::endl;
//			return;
//		}
//		GPU::checkFramebufferStatus();
//		glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width, dst.height, GL_COLOR_BUFFER_BIT, filterGL);
//		_metrics.clearAndBlits += 1;
//	}
//	// Restore the proper framebuffers from the cache.
//	glBindFramebuffer(GL_READ_FRAMEBUFFER, _state.readFramebuffer);
//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _state.drawFramebuffer);
//	_metrics.framebufferBindings += 2;
//	glDeleteFramebuffers(1, &srcFb);
//	glDeleteFramebuffers(1, &dstFb);
}

void GPU::blit(const Texture & src, Framebuffer & dst, Filter filter) {
	// Prepare the destination.
//	if(src.levels != 1) {
//		Log::Warning() << Log::GPU << "Only the first mipmap level will be used." << std::endl;
//	}
//	if(src.shape != dst.shape()){
//		Log::Error() << Log::GPU << "The texture and framebuffer don't have the same shape." << std::endl;
//		return;
//	}
//
//	// Create one framebuffer.
//	GLuint srcFb;
//	glGenFramebuffers(1, &srcFb);
//	// Because it's temporary and will be unbound at the end of the call
//	// we do not update the cached GPU state.
//	glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFb);
//	_metrics.framebufferBindings += 1;
//	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
//
//	if(src.shape == TextureShape::Cube) {
//		for(size_t i = 0; i < 6; ++i) {
//			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), src.gpu->id, 0);
//			GPU::checkFramebufferStatus();
//			dst.bind(i, 0, Framebuffer::Mode::WRITE);
//			glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width(), dst.height(), GL_COLOR_BUFFER_BIT, filterGL);
//			_metrics.clearAndBlits += 1;
//		}
//	} else {
//		if(src.shape == TextureShape::D1) {
//			glFramebufferTexture1D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);
//
//		} else if(src.shape == TextureShape::D2) {
//			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);
//
//		} else {
//			Log::Error() << Log::GPU << "Unsupported texture shape for blitting." << std::endl;
//			return;
//		}
//		GPU::checkFramebufferStatus();
//		dst.bind(0, 0, Framebuffer::Mode::WRITE);
//		glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width(), dst.height(), GL_COLOR_BUFFER_BIT, filterGL);
//		_metrics.clearAndBlits += 1;
//	}
//	// Restore the proper framebuffer from the cache.
//	glBindFramebuffer(GL_READ_FRAMEBUFFER, _state.readFramebuffer);
//	_metrics.framebufferBindings += 1;
//	glDeleteFramebuffers(1, &srcFb);
}

//void GPU::savePixels(GLenum type, GLenum format, unsigned int width, unsigned int height, unsigned int components, const std::string & path, bool flip, bool ignoreAlpha) {

//	GPU::sync();
//
//	const bool hdr = type == GL_FLOAT;
//
//	Log::Info() << Log::GPU << "Saving framebuffer to file " << path << (hdr ? ".exr" : ".png") << "... " << std::flush;
//	int ret;
//	Image image(width, height, components);
//
//	glPixelStorei(GL_PACK_ALIGNMENT, 1);
//	_metrics.stateChanges += 1;
//	const size_t fullSize = image.width * image.height * image.components;
//	if(hdr) {
//		// Get back values.
//		glReadPixels(0, 0, GLsizei(image.width), GLsizei(image.height), format, type, &image.pixels[0]);
//		_metrics.downloads += 1;
//		// Save data.
//		ret = image.save(path + ".exr", flip, ignoreAlpha);
//
//	} else {
//		// Get back values.
//		GLubyte * data = new GLubyte[fullSize];
//		glReadPixels(0, 0, GLsizei(image.width), GLsizei(image.height), format, type, &data[0]);
//		_metrics.downloads += 1;
//		// Convert to image float format.
//		for(size_t pid = 0; pid < fullSize; ++pid) {
//			image.pixels[pid] = float(data[pid]) / 255.0f;
//		}
//		// Save data.
//		ret = image.save(path + ".png", flip, ignoreAlpha);
//		delete[] data;
//	}
//	glPixelStorei(GL_PACK_ALIGNMENT, 4);
//	_metrics.stateChanges += 1;
//
//	if(ret != 0) {
//		Log::Error() << "Error." << std::endl;
//	} else {
//		Log::Info() << "Done." << std::endl;
//	}
//}

void GPU::getState(GPUState& state) {
	state = _state;
}


const GPU::Metrics & GPU::getMetrics(){
	return _metricsPrevious;
}

void GPU::cleanup(){
	GPU::sync();

	_pipelineCache.clean();
	_context.descriptorAllocator.clean();
	
	vkDestroyCommandPool(_context.device, _context.commandPool, nullptr);

	_quad.clean();
	vmaDestroyAllocator(_allocator);

	//vkDestroyDevice(_context.device, nullptr);
	ShaderCompiler::cleanup();
}


void GPU::clean(GPUTexture & tex){
	vkDestroyImageView(_context.device, tex.view, nullptr);
	vkDestroySampler(_context.device, tex.sampler, nullptr);
	vmaDestroyImage(_allocator, tex.image, tex.data);
}

void GPU::clean(Framebuffer & framebuffer){

}

void GPU::clean(GPUMesh & mesh){
	// Nothing to do, buffers will all be cleaned up.
}

void GPU::clean(GPUBuffer & buffer){
	vmaDestroyBuffer(_allocator, buffer.buffer, buffer.data);
}

void GPU::clean(Program & program){
	
}

GPUState GPU::_state;
GPUState GPU::_lastState;
GPU::Metrics GPU::_metrics;
GPU::Metrics GPU::_metricsPrevious;
Mesh GPU::_quad("Quad");

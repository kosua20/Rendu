#include "graphics/GPU.hpp"
#include "graphics/ShaderCompiler.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/SamplerLibrary.hpp"
#include "resources/Texture.hpp"
#include "resources/Image.hpp"
#include "system/TextUtilities.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "graphics/GPUInternal.hpp"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION

#define FORCE_MARKERS_VULKAN

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#include <vma/vk_mem_alloc.h>
#pragma clang diagnostic pop

#include <sstream>
#include <GLFW/glfw3.h>
#include <set>

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" /*, "VK_LAYER_LUNARG_api_dump"*/ };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,  VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,  VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

GPUContext _context;

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

	bool wantsMarkers = false;
#if defined(DEBUG) || defined(FORCE_MARKERS_VULKAN)
	wantsMarkers	  = VkUtils::checkExtensionsSupport({VK_EXT_DEBUG_UTILS_EXTENSION_NAME});
#endif

	bool debugEnabled = false;
#if defined(DEBUG) || defined(FORCELAYERS_VULKAN)
	// Only enable if the layers are supported.
	debugEnabled	  = VkUtils::checkLayersSupport(validationLayers);
	debugEnabled &= wantsMarkers;
#endif

	bool wantsPortability = false;
#if defined(__APPLE__) || defined(FORCE_PORTABILITY)
	wantsPortability = true;
#endif

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Rendu";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	
	// We have to tell Vulkan the extensions we need.
	const std::vector<const char *> extensions = VkUtils::getRequiredInstanceExtensions(wantsMarkers, wantsPortability);
	if(!VkUtils::checkExtensionsSupport(extensions)){
		Log::Error() << Log::GPU << "Unsupported extensions." << std::endl;
		return false;
	}
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();
	// Allow portability drivers enumeration to support MoltenVK.
	if(wantsPortability) {
		instanceInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}

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
	}

	// Pick a physical device.
	uint32_t deviceCount = 0;
	VK_RET(vkEnumeratePhysicalDevices(_context.instance, &deviceCount, nullptr));
	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_RET(vkEnumeratePhysicalDevices(_context.instance, &deviceCount, devices.data()));

	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
	// Check which one is ok for our requirements.
	for(const auto& device : devices) {
		bool hasPortability = false;
		// We want a device with swapchain support.
		const bool supportExtensions = VkUtils::checkDeviceExtensionsSupport(device, deviceExtensions, hasPortability);
		// Ask for anisotropy and tessellation.
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);
		const bool hasFeatures = features.samplerAnisotropy && features.tessellationShader && features.imageCubeArray;

		if(supportExtensions && hasFeatures){
			// Prefere a discrete GPU if possible.
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);
			const bool isDiscrete = properties.deviceType;

			if(selectedDevice == VK_NULL_HANDLE || isDiscrete){
				selectedDevice = device;
				_context.portability = hasPortability;
				break;
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
	// minImageTransferGranularity is guaranteed to be (1,1,1) on graphics/compute queues
	_context.markersEnabled = wantsMarkers;

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
	int graphicsIndex = -1;
	int presentIndex = -1;
	bool found = VkUtils::getQueueFamilies(_context.physicalDevice, _context.surface, graphicsIndex, presentIndex);
	if(!found){
		Log::Error() << Log::GPU << "Unable to find compatible queue families." << std::endl;
		return false;
	}

	// Select queues.
	std::set<uint> families;
	families.insert(uint(graphicsIndex));
	families.insert(uint(presentIndex));

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
	features.imageCubeArray = VK_TRUE;
	features.fillModeNonSolid = VK_TRUE;
	deviceInfo.pEnabledFeatures = &features;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature { };
	dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamicRenderingFeature.dynamicRendering = VK_TRUE;
	deviceInfo.pNext = &dynamicRenderingFeature;
	
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
	_context.graphicsId = uint(graphicsIndex);
	_context.presentId = uint(presentIndex);
	vkGetDeviceQueue(_context.device, _context.graphicsId, 0, &_context.graphicsQueue);
	vkGetDeviceQueue(_context.device, _context.presentId, 0, &_context.presentQueue);

	VkUtils::setDebugName(_context, VK_OBJECT_TYPE_QUEUE, uint64_t(_context.graphicsQueue), "Graphics");
	VkUtils::setDebugName(_context, VK_OBJECT_TYPE_QUEUE, uint64_t(_context.presentQueue), "Present");

	// Setup allocator.
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
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

	_vulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
	_vulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

	allocatorInfo.pVulkanFunctions = &_vulkanFunctions;
	VK_RET(vmaCreateAllocator(&allocatorInfo, &_allocator));


	// Create the command pool.
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = _context.graphicsId;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if(vkCreateCommandPool(_context.device, &poolInfo, nullptr, &_context.commandPool) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create command pool." << std::endl;
		return false;
	}

	VkUtils::setDebugName(_context, VK_OBJECT_TYPE_COMMAND_POOL, uint64_t(_context.commandPool), "Main pool");

	// Create query pools.
	_context.queryAllocators[GPUQuery::Type::TIME_ELAPSED].init(GPUQuery::Type::TIME_ELAPSED, 1024);
	_context.queryAllocators[GPUQuery::Type::ANY_DRAWN].init(GPUQuery::Type::ANY_DRAWN, 1024);
	_context.queryAllocators[GPUQuery::Type::SAMPLES_DRAWN].init(GPUQuery::Type::SAMPLES_DRAWN, 1024);

	// Finally setup the swapchain.
	window->_swapchain.reset(new Swapchain(_context, window->_config));
	
	// Create a pipeline cache.
	_context.pipelineCache.init();

	_context.descriptorAllocator.init(&_context, 1024);

	// Create static samplers.
	_context.samplerLibrary.init();

	// Create basic vertex array for screenquad.
	{
		_quad.positions = {
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, 3.0f, 0.0f),
			glm::vec3(3.0f, -1.0f, 0.0f),
		};
		_quad.indices = {0, 1, 2};
		_quad.upload();
	}
	return true;
}

void GPU::createGraphicsProgram(Program& program, const std::string & vertexContent, const std::string & fragmentContent, const std::string & tessControlContent, const std::string & tessEvalContent, const std::string & debugInfos) {

	Log::Verbose() << Log::GPU << "Compiling " << debugInfos << "." << std::endl;
	
	std::string compilationLog;
	// If vertex program code is given, compile it.
	if(!vertexContent.empty()) {
		ShaderCompiler::compile(vertexContent, ShaderType::VERTEX, program.stage(ShaderType::VERTEX), true, compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Vertex shader (for " << program.name() << ") failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If fragment program code is given, compile it.
	if(!fragmentContent.empty()) {
		ShaderCompiler::compile(fragmentContent, ShaderType::FRAGMENT, program.stage(ShaderType::FRAGMENT), true, compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Fragment shader (for " << program.name() << ") failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If tessellation control program code is given, compile it.
	if(!tessControlContent.empty()) {
		ShaderCompiler::compile(tessControlContent, ShaderType::TESSCONTROL, program.stage(ShaderType::TESSCONTROL), true, compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Tessellation control shader (for " << program.name() << ") failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If tessellation evaluation program code is given, compile it.
	if(!tessEvalContent.empty()) {
		ShaderCompiler::compile(tessEvalContent, ShaderType::TESSEVAL, program.stage(ShaderType::TESSEVAL), true, compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Tessellation evaluation shader (for " << program.name() << ") failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	++_metrics.programs;
}

void GPU::createComputeProgram(Program& program, const std::string & computeContent, const std::string & debugInfos) {

	Log::Verbose() << Log::GPU << "Compiling " << debugInfos << "." << std::endl;

	std::string compilationLog;
	// If compute program code is given, compile it.
	if(!computeContent.empty()) {
		ShaderCompiler::compile(computeContent, ShaderType::COMPUTE, program.stage(ShaderType::COMPUTE), true, compilationLog);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Compute shader (for " << program.name() << ") failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	++_metrics.programs;
}

void GPU::bindProgram(const Program & program){
	if(program.type() == Program::Type::COMPUTE){
		_state.computeProgram = (Program*)&program;
	} else {
		_state.graphicsProgram = (Program*)&program;
	}
}

void GPU::beginRender(const Load & depthOp, const Load & stencilOp, const Texture * depthStencil, const Load & colorOp, const Texture * color0, const Texture * color1, const Texture * color2, const Texture * color3) {
	GPU::bindAttachments(0, 0, colorOp, depthOp, stencilOp, depthStencil, color0, color1, color2, color3);
}

void GPU::beginRender(const Load& colorOp, const Texture* color0, const Texture* color1, const Texture* color2, const Texture* color3){
	GPU::bindAttachments(0, 0, colorOp, Load::Operation::DONTCARE, Load::Operation::DONTCARE, nullptr, color0, color1, color2, color3);
}

void GPU::beginRender(const Load& depthOp, const Load& stencilOp, const Texture* depthStencil){
	GPU::bindAttachments(0, 0, Load::Operation::DONTCARE, depthOp, stencilOp, depthStencil, nullptr, nullptr, nullptr, nullptr);
}

void GPU::beginRender(uint layer, uint mip, const Load & depthOp, const Load & stencilOp, const Texture * depthStencil, const Load & colorOp, const Texture * color0, const Texture * color1, const Texture * color2, const Texture * color3) {
	GPU::bindAttachments(layer, mip, colorOp, depthOp, stencilOp, depthStencil, color0, color1, color2, color3);
}

void GPU::beginRender(uint layer, uint mip, const Load& colorOp, const Texture* color0, const Texture* color1, const Texture* color2, const Texture* color3){
	GPU::bindAttachments(layer, mip, colorOp, Load::Operation::DONTCARE, Load::Operation::DONTCARE, nullptr, color0, color1, color2, color3);
	
}

void GPU::beginRender(uint layer, uint mip, const Load& depthOp, const Load& stencilOp, const Texture* depthStencil){
	GPU::bindAttachments(layer, mip, Load::Operation::DONTCARE, depthOp, stencilOp, depthStencil, nullptr, nullptr, nullptr, nullptr);
}

void GPU::beginRender(const Window & window, const Load & depthOp, const Load & stencilOp, const Load & colorOp) {
	GPU::bindAttachments(0, 0, colorOp, depthOp, stencilOp, &window._swapchain->depth(), &window._swapchain->color(), nullptr, nullptr, nullptr);
}

void GPU::bindAttachments(uint layer, uint mip, const Load& colorOp, const Load& depthOp, const Load& stencilOp, const Texture* depthStencil, const Texture* color0, const Texture* color1, const Texture* color2, const Texture* color3){

	GPU::endRenderingIfNeeded();

	_state.depthStencil = nullptr;
	_state.colors.fill(nullptr);
	_state.pass = {};

	assert(depthStencil != nullptr || color0 != nullptr);
	if(depthStencil) {
		_state.pass.depthStencil = depthStencil->format;
		_state.depthStencil		 = depthStencil;
	}

	uint colorsCount = 0;
	if(color0){
		_state.pass.colors[0] = color0->format;
		_state.colors[0]	  = color0;
		++colorsCount;
	}

	if(color1){
		assert(color0 != nullptr);
		_state.pass.colors[1] = color1->format;
		_state.colors[1]	  = color1;
		++colorsCount;
	}
	if(color2){
		assert(color1 != nullptr);
		_state.pass.colors[2] = color2->format;
		_state.colors[2]	  = color2;
		++colorsCount;
	}
	if(color3){
		assert(color2 != nullptr);
		_state.pass.colors[3] = color3->format;
		_state.colors[3]	  = color3;
		++colorsCount;
	}

	_state.pass.mipStart = mip;
	_state.pass.mipCount = 1;
	_state.pass.layerStart = layer;
	_state.pass.layerCount = 1;

	GPUContext* context = GPU::getInternal();
	VkCommandBuffer& commandBuffer = context->getRenderCommandBuffer();

	const uint width  = depthStencil ? depthStencil->width  : color0->width;
	const uint height = depthStencil ? depthStencil->height : color0->height;
	const uint w = std::max<uint>(1u, width >> mip);
	const uint h = std::max<uint>(1u, height >> mip);

	static const std::array<VkAttachmentLoadOp, 3> ops = {VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_DONT_CARE};

	const VkAttachmentLoadOp colorLoad = ops[uint(colorOp.mode)];
	const VkAttachmentStoreOp colorStore = VK_ATTACHMENT_STORE_OP_STORE;
	const VkAttachmentLoadOp 	depthLoad = ops[uint(depthOp.mode)];
	const VkAttachmentStoreOp depthStore = VK_ATTACHMENT_STORE_OP_STORE;
	const VkAttachmentLoadOp 	stencilLoad = ops[uint(stencilOp.mode)];
	const VkAttachmentStoreOp stencilStore = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	info.renderArea.extent = {uint32_t(w), uint32_t(h)};
	info.renderArea.offset = {0u, 0u};
	info.layerCount = 1;
	info.colorAttachmentCount = 0;
	info.pColorAttachments = nullptr;
	info.pDepthAttachment = nullptr;
	info.pStencilAttachment = nullptr;
	info.viewMask = 0;
	info.flags = 0;

	std::vector<VkRenderingAttachmentInfoKHR> colorInfos(colorsCount);
	VkRenderingAttachmentInfoKHR depthInfo{};
	VkRenderingAttachmentInfoKHR stencilInfo{};

	for(uint cid = 0; cid < colorsCount; ++cid){
		VkRenderingAttachmentInfoKHR& colorInfo = colorInfos[cid];
		colorInfo.imageView = _state.colors[cid]->gpu->views[mip].views[layer];
		colorInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorInfo.clearValue.color.float32[0] = colorOp.value[0];
		colorInfo.clearValue.color.float32[1] = colorOp.value[1];
		colorInfo.clearValue.color.float32[2] = colorOp.value[2];
		colorInfo.clearValue.color.float32[3] = colorOp.value[3];
		colorInfo.loadOp = colorLoad;
		colorInfo.storeOp = colorStore;
		colorInfo.resolveMode = VK_RESOLVE_MODE_NONE;

		VkUtils::imageLayoutBarrier(commandBuffer, *_state.colors[cid]->gpu, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, mip, 1, layer, 1);
	}

	if(colorsCount != 0){
		info.pColorAttachments = colorInfos.data();
		info.colorAttachmentCount = colorsCount;
	}

	if(depthStencil){
		info.pDepthAttachment = &depthInfo;
		depthInfo.imageView	= depthStencil->gpu->views[mip].views[layer];
		depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthInfo.clearValue.depthStencil.depth = depthOp.value[0];
		depthInfo.loadOp = depthLoad;
		depthInfo.storeOp = depthStore;
		depthInfo.resolveMode = VK_RESOLVE_MODE_NONE;

		if(depthStencil->format == Layout::DEPTH24_STENCIL8 || depthStencil->format == Layout::DEPTH32F_STENCIL8) {
			info.pStencilAttachment = &stencilInfo;
			stencilInfo.imageView = depthInfo.imageView;
			stencilInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
			stencilInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			stencilInfo.clearValue.depthStencil.stencil = uint32_t(stencilOp.value[0]);
			stencilInfo.loadOp = stencilLoad;
			stencilInfo.storeOp = stencilStore;
			stencilInfo.resolveMode = VK_RESOLVE_MODE_NONE;
		}

		VkUtils::imageLayoutBarrier(commandBuffer, *_state.depthStencil->gpu, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, mip, 1, layer, 1);
	}

	vkCmdBeginRenderingKHR(commandBuffer, &info);
	context->newRenderPass = true;
	context->inRenderPass = true;
}

void GPU::saveTexture(Texture & texture, const std::string & path, Image::Save options) {
	static const std::vector<Layout> hdrLayouts = {
		Layout::R16F, Layout::RG16F, Layout::RGBA16F, Layout::R32F, Layout::RG32F, Layout::RGBA32F, Layout::A2_BGR10, Layout::A2_RGB10,
		Layout::DEPTH_COMPONENT32F, Layout::DEPTH24_STENCIL8, Layout::DEPTH_COMPONENT16, Layout::DEPTH_COMPONENT24, Layout::DEPTH32F_STENCIL8,
	};

	GPU::endRenderingIfNeeded();

	const bool isHDR = std::find(hdrLayouts.begin(), hdrLayouts.end(), texture.format) != hdrLayouts.end();

	GPU::downloadTextureAsync(texture, glm::uvec2(0), glm::uvec2(texture.width, texture.height), 1, [isHDR, path, options](const Texture& result){
		// Save the image to the disk.
		const std::string ext = isHDR ? ".exr" : ".png";
		const std::string finalPath = path + ext;
		Log::Info() << Log::GPU << "Saving texture to file " << finalPath << "... " << std::endl;
		// Gamma correction if the output is LDR.
		const int ret = result.images[0].save(finalPath, options | Image::Save::SRGB_LDR);
		if(ret != 0) {
			Log::Error() << "Error when saving image at path " << finalPath << "." << std::endl;
		}
	});

}

void GPU::setupTexture(Texture & texture) {

	if(texture.gpu) {
		texture.gpu->clean();
	}

	texture.gpu.reset(new GPUTexture(texture.format));
	const std::string& name = texture.name();

	const bool is3D = texture.shape & TextureShape::D3;
	const bool isCube = texture.shape & TextureShape::Cube;
	const bool isArray = texture.shape & TextureShape::Array;
	const bool isDepth = texture.gpu->aspect & VK_IMAGE_ASPECT_DEPTH_BIT;

	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if(texture.drawable){
		usage |= isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	// Check feature support for compute access
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(_context.physicalDevice, texture.gpu->format, &formatProperties);
	if((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0) {
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	const uint layers = (isCube || isArray) ? texture.depth : 1;

	// Set the layout on all subresources.
	texture.gpu->defaultLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texture.gpu->layouts.resize(texture.levels);
	for(uint mipId = 0; mipId < texture.levels; ++mipId){
		texture.gpu->layouts[mipId].resize(layers, VK_IMAGE_LAYOUT_UNDEFINED);
	}
	VkImageType imgType;
	VkImageViewType viewType;
	VkUtils::typesFromShape(texture.shape, imgType, viewType);

	// Create image.
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = imgType;
	imageInfo.extent.width = static_cast<uint32_t>(texture.width);
	imageInfo.extent.height = static_cast<uint32_t>(texture.height);
	imageInfo.extent.depth = is3D ? texture.depth : 1;
	imageInfo.mipLevels = texture.levels;
	imageInfo.arrayLayers = layers;
	imageInfo.format = texture.gpu->format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;
	if(isCube){
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VK_RET(vmaCreateImage(_allocator, &imageInfo, &allocInfo, &(texture.gpu->image), &(texture.gpu->data), nullptr));

	VkUtils::setDebugName(_context, VK_OBJECT_TYPE_IMAGE, uint64_t(texture.gpu->image), "%s", name.c_str());

	// Create view.
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = texture.gpu->image;
	viewInfo.viewType = viewType;
	viewInfo.format = texture.gpu->format;
	// Remove the stencil bit when reading from the texture via the view.
	viewInfo.subresourceRange.aspectMask = (texture.gpu->aspect & ~VK_IMAGE_ASPECT_STENCIL_BIT);
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = texture.levels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;

	if(vkCreateImageView(_context.device, &viewInfo, nullptr, &(texture.gpu->view)) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create image view." << std::endl;
		return;
	}

	VkUtils::setDebugName(_context, VK_OBJECT_TYPE_IMAGE_VIEW, uint64_t(texture.gpu->view), "%s-global", name.c_str());

	// Slice format
	VkImageViewType viewTypeSlice = VK_IMAGE_VIEW_TYPE_2D;
	if((texture.shape & TextureShape::D3) != 0){
		viewTypeSlice = VK_IMAGE_VIEW_TYPE_3D;
	} else if((texture.shape & TextureShape::D1) != 0){
		viewTypeSlice = VK_IMAGE_VIEW_TYPE_1D;
	}

	// Create per mip view. \todo Create only for writable/drawable?
	texture.gpu->views.resize(texture.levels);
	for(uint mid = 0; mid < texture.levels; ++mid){

		// Create per layer per mip view.
		texture.gpu->views[mid].views.resize(layers);
		for(uint lid = 0; lid < layers; ++lid){
			VkImageViewCreateInfo viewInfoMip = {};
			viewInfoMip.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfoMip.image = texture.gpu->image;
			viewInfoMip.viewType = viewTypeSlice;
			viewInfoMip.format = texture.gpu->format;
			// This will mainly be used as an attachment, so don't remove the stencil aspect.
			viewInfoMip.subresourceRange.aspectMask = texture.gpu->aspect;
			viewInfoMip.subresourceRange.baseMipLevel = mid;
			viewInfoMip.subresourceRange.levelCount = 1;
			viewInfoMip.subresourceRange.baseArrayLayer = lid;
			viewInfoMip.subresourceRange.layerCount = 1;

			if(vkCreateImageView(_context.device, &viewInfoMip, nullptr, &(texture.gpu->views[mid].views[lid])) != VK_SUCCESS) {
				Log::Error() << "GPU: Unable to create image view." << std::endl;
				return;
			}

			VkUtils::setDebugName(_context, VK_OBJECT_TYPE_IMAGE_VIEW, uint64_t(texture.gpu->views[mid].views[lid]), "%s-mip %u-level %u", name.c_str(), mid, lid);
		}

		// Create global mip view.
		VkImageViewCreateInfo viewInfoMip = {};
		viewInfoMip.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfoMip.image = texture.gpu->image;
		viewInfoMip.viewType = viewType;
		viewInfoMip.format = texture.gpu->format;
		// Remove the stencil bit when reading from the texture via the view.
		viewInfoMip.subresourceRange.aspectMask = (texture.gpu->aspect & ~VK_IMAGE_ASPECT_STENCIL_BIT);
		viewInfoMip.subresourceRange.baseMipLevel = mid;
		viewInfoMip.subresourceRange.levelCount = 1;
		viewInfoMip.subresourceRange.baseArrayLayer = 0;
		viewInfoMip.subresourceRange.layerCount = imageInfo.arrayLayers;

		if(vkCreateImageView(_context.device, &viewInfoMip, nullptr, &(texture.gpu->views[mid].mipView)) != VK_SUCCESS) {
			Log::Error() << Log::GPU << "Unable to create image view." << std::endl;
			return;
		}

		VkUtils::setDebugName(_context, VK_OBJECT_TYPE_IMAGE_VIEW, uint64_t(texture.gpu->views[mid].mipView), "%s-mip %u", name.c_str(), mid);

	}

	if(texture.drawable) {
		VkCommandBuffer commandBuffer = _context.getUploadCommandBuffer();
		VkUtils::textureLayoutBarrier(commandBuffer, texture, texture.gpu->defaultLayout);
	}

	++_metrics.textures;
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

	// Determine if we can do the transfer without an intermediate texture.
	const Layout format = texture.format;
	const bool is8UB = format == Layout::R8 || format == Layout::RG8 || format == Layout::RGBA8 || format == Layout::BGRA8 || format == Layout::SRGB8_ALPHA8 || format == Layout::SBGR8_ALPHA8;
	const bool is32F = format == Layout::R32F || format == Layout::RG32F || format == Layout::RGBA32F;

	// Compute total texture size.
	size_t totalComponentCount = 0;
	for(const auto & img: texture.images) {
		const size_t imgSize = img.pixels.size();
		totalComponentCount += imgSize;
	}

	const size_t compSize = is8UB ? sizeof(unsigned char) : sizeof(float);
	const size_t totalSize = totalComponentCount * compSize;

	Texture transferTexture("tmpTexture");
	const Texture* dstTexture = &texture;
	size_t currentOffset = 0;

	// Transfer the complete CPU image data to a staging buffer, handling conversion..
	Buffer transferBuffer(totalSize, BufferType::CPUTOGPU, "ImageStaging");

	if(is8UB){
		// Convert to uchar on the CPU.
		for(const auto & img: texture.images) {
			// Number of floats in the image.
			const size_t compCount = img.pixels.size();
			// Ideally parallelism should be moved higher up.
			System::forParallel(0, compCount, [&img, currentOffset, &transferBuffer](size_t cid){
				const float val = glm::clamp(img.pixels[cid], 0.0f, 1.0f);
				*(transferBuffer.gpu->mapped + currentOffset + cid) = (unsigned char)(255.0f * val);
			});
			currentOffset += compCount;
		}
	} else {
		// Copy float arrays.
		size_t currentOffset = 0;
		for(const auto & img: texture.images) {
			const size_t compCount = img.pixels.size() * compSize;
			std::memcpy(transferBuffer.gpu->mapped + currentOffset, img.pixels.data(), compCount);
			currentOffset += compCount;
		}
		// If destination is not 32F, we need to use an intermediate 32F texture and convert
		// to destination format using blit.
		if(!is32F){
			// Prepare the intermediate texture.
			transferTexture.width = texture.width;
			transferTexture.height = texture.height;
			transferTexture.depth = texture.depth;
			transferTexture.levels = texture.levels;
			transferTexture.shape = texture.shape;
			const Layout floatFormats[5] = {Layout(0), Layout::R32F, Layout::RG32F, Layout::RGBA32F /* no 3 channels format */, Layout::RGBA32F};
			transferTexture.format = floatFormats[destChannels];
			GPU::setupTexture(transferTexture);
			// Useful to avoid a useless transition at the very end.
			transferTexture.gpu->defaultLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			// Change floatcopy destination.
			dstTexture = &transferTexture;
		}
	}
	// Prepare copy destination.
	VkCommandBuffer commandBuffer = _context.getUploadCommandBuffer();
	VkUtils::textureLayoutBarrier(commandBuffer, *dstTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// How many images in the mip level (for arrays and cubes)
	const uint layers = texture.shape == TextureShape::D3 ? 1u : texture.depth;
	// Copy operation for each mip level that is available on the CPU.
	size_t currentImg = 0;
	currentOffset = 0;

	for(uint mid = 0; mid < texture.levels; ++mid) {
		// How deep is the image for 3D textures.
		const uint d = texture.shape == TextureShape::D3 ? std::max<uint>(texture.depth >> mid, 1u) : 1u;
		const uint w = std::max<uint>(texture.width >> mid, 1u);
		const uint h = std::max<uint>(texture.height >> mid, 1u);

		// Perform copy for this mip level.
		VkBufferImageCopy region = {};
		region.bufferOffset = currentOffset;
		region.bufferRowLength = 0; // Tightly packed.
		region.bufferImageHeight = 0; // Tightly packed.
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = uint32_t(mid);
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = uint32_t(layers);
		// Offset *in the subregion*
		region.imageOffset = {0, 0, 0};
		region.imageExtent = { (uint32_t)w, (uint32_t)h, (uint32_t)d};

		// Copy to the intermediate texture.
		vkCmdCopyBufferToImage(commandBuffer, transferBuffer.gpu->buffer, dstTexture->gpu->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		const uint imageCount = texture.shape == TextureShape::D3 ? d : layers;
		// Support both 8-bits and 32-bits cases.
		currentOffset += imageCount * w * h * destChannels * compSize;
		currentImg += imageCount;
		// We might have more levels allocated on the GPU than we had available on the CPU.
		// Stop, these will be generated automatically.
		if(currentImg >= texture.images.size()){
			break;
		}

	}
	// If we used an intermediate texture, blit from it to the destination. This will handle format conversion.
	if(dstTexture != &texture){
		const glm::uvec2 size(texture.width, texture.height);
		VkUtils::blitTexture(commandBuffer, *dstTexture, texture, 0, 0, texture.levels, 0, 0, layers, glm::uvec2(0), size, glm::uvec2(0), size, Filter::NEAREST);
	} else {
		// Else we just have to transition the destination to its default layout.
		VkUtils::imageLayoutBarrier(commandBuffer, *(texture.gpu), texture.gpu->defaultLayout, 0, texture.levels, 0, layers);
	}

	++_metrics.uploads;
}


void GPU::downloadTextureSync(Texture & texture, int level) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(!(texture.gpu->aspect & VK_IMAGE_ASPECT_COLOR_BIT)){
		Log::Error() << Log::GPU << "Unsupported download format." << std::endl;
		return;
	}

	if(!texture.images.empty()) {
		Log::Verbose() << Log::GPU << "Texture already contain CPU data, will be erased." << std::endl;
	}


	const uint firstLevel = level >= 0 ? uint(level) : 0u;
	const uint lastLevel = level >= 0 ? uint(level) : (texture.levels - 1u);
	const uint levelCount = lastLevel - firstLevel + 1u;

	// Download will take place on an auxiliary command buffer, so we have to make sure that the texture is in
	// its default layout state (the one at the end of a command buffer). We will restore it to the same state afterwards.
	for(uint lid = firstLevel; lid <= lastLevel; ++lid){
		for(const VkImageLayout& lay : texture.gpu->layouts[lid]){
			if(lay != texture.gpu->defaultLayout){
				Log::Error() << Log::GPU << "Texture should be in its default layout state." << std::endl;
				return;
			}
		}
	}

	VkCommandBuffer commandBuffer = VkUtils::beginSyncOperations(_context);
	const uint layersCount = texture.shape == TextureShape::D3? 1 : texture.depth;
	const glm::uvec2 size(texture.width, texture.height);
	
	std::shared_ptr<Buffer> dstBuffer;
	const glm::uvec2 imgRange = VkUtils::copyTextureRegionToBuffer(commandBuffer, texture, dstBuffer, firstLevel, levelCount, 0, layersCount, glm::uvec2(0), size);
	VkUtils::endSyncOperations(commandBuffer, _context);
	GPU::flushBuffer(*dstBuffer, dstBuffer->size, 0);

	// Prepare images.
	texture.allocateImages(texture.gpu->channels, firstLevel, levelCount);

	// Finally copy from the buffer to each image.
	size_t currentOffset = 0;
	for(uint iid = 0; iid < imgRange[1]; ++iid){
		Image& img = texture.images[imgRange[0] + iid];
		const size_t imgSize = img.pixels.size() * sizeof(float);
		std::memcpy(img.pixels.data(), dstBuffer->gpu->mapped + currentOffset, imgSize);
		currentOffset += imgSize;
	}

	++_metrics.downloads;

}

GPUAsyncTask GPU::downloadTextureAsync(const Texture& texture, const glm::uvec2& offset, const glm::uvec2& size, uint layerCount, std::function<void(const Texture&)> callback){
	GPU::endRenderingIfNeeded();
	
	const uint texLayerCount = texture.shape == TextureShape::D3 ? 1 : texture.depth;
	const uint effectiveLayerCount = layerCount == 0 ? texLayerCount : std::min(texLayerCount, layerCount);

	_context.textureTasks.emplace_back();
	++_context.tasksCount;

	AsyncTextureTask& request = _context.textureTasks.back();
	request.frame = _context.frameIndex;
	request.dstImageRange = VkUtils::copyTextureRegionToBuffer(_context.getRenderCommandBuffer(), texture, request.dstBuffer, 0, 1, 0, effectiveLayerCount, offset, size);
	request.dstImageRange[0] = 0;
	request.dstTexture.reset(new Texture("DstTexture"));
	request.dstTexture->width = size[0];
	request.dstTexture->height = size[1];
	request.dstTexture->depth = effectiveLayerCount;
	request.dstTexture->levels = 1;
	request.dstTexture->shape = texture.shape;
	request.dstTexture->allocateImages(texture.gpu->channels, 0, 1);
	request.callback = callback;
	request.id = GPUAsyncTask(_context.tasksCount);
	return request.id;
}

void GPU::cancelAsyncOperation(const GPUAsyncTask& id){
	for(auto op = _context.textureTasks.begin(); op != _context.textureTasks.end(); ++op){
		if(op->id == id){
			_context.textureTasks.erase(op);
			break;
		}
	}
}

void GPU::downloadTextureSync(Texture & texture) {
	downloadTextureSync(texture, -1);
}

void GPU::generateMipMaps(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}
	// Do we support blitting?
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(_context.physicalDevice, texture.gpu->format, &formatProperties);
	if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		Log::Warning() << Log::GPU << "Filtered mipmapping not supported for this format." << std::endl;
		return;
	}

	const bool isCube = texture.shape & TextureShape::Cube;
	const bool isArray = texture.shape & TextureShape::Array;
	const uint layers = (isCube || isArray) ? texture.depth : 1u;
	const uint baseWidth = texture.width;
	const uint baseHeight = texture.height;
	const uint baseDepth = texture.shape == TextureShape::D3 ? texture.depth : 1u;

	// Blit the texture to each mip level.
	VkCommandBuffer commandBuffer = _context.getUploadCommandBuffer();

	VkUtils::textureLayoutBarrier(commandBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Respect existing (already uploaded) mip levels.
	const uint presentCount = uint(texture.images.size());
	uint totalCount = 0;
	uint firstLevelToGenerate = 0xFFFFFFFF;
	for(uint lid = 0; lid < texture.levels; ++lid){
		// Number of images at this level:
		const uint imageCount = texture.shape == TextureShape::D3 ? std::max<uint>(texture.depth >> lid, 1u) : layers;
		totalCount += imageCount;
		if(totalCount > presentCount){
			firstLevelToGenerate = lid;
			break;
		}
	}
	if(firstLevelToGenerate == 0xFFFFFFFF){
		Log::Info() << "No mip level left to generate, all are already present." << std::endl;
		return;
	}
	const uint srcMip = firstLevelToGenerate - 1;
	uint width = baseWidth > 1 ? (baseWidth >> srcMip) : 1;
	uint height = baseHeight > 1 ? (baseHeight >> srcMip) : 1;
	uint depth = baseDepth > 1 ? (baseDepth >> srcMip) : 1;

	for(uint mid = firstLevelToGenerate; mid < texture.levels; mid++) {

		// Transition level i-1 to transfer source layout.
		VkUtils::imageLayoutBarrier(commandBuffer, *texture.gpu, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mid - 1, 1, 0, layers);

		// Then, prepare blit to level i.
		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { (int32_t)width, (int32_t)height, (int32_t)depth };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = uint32_t(mid - 1);
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = uint32_t(layers);
		blit.dstOffsets[0] = { 0, 0, 0 };

		// Divide all dimensions by 2 if possible.
		width  = (width  > 1 ? (width/2)  : 1);
		height = (height > 1 ? (height/2) : 1);
		depth  = (depth  > 1 ? (depth/2)  : 1);

		blit.dstOffsets[1] = { (int32_t)width, (int32_t)height, (int32_t)depth };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = uint32_t(mid);
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = uint32_t(layers);

		// Blit using linear filtering for smoother downscaling.
		vkCmdBlitImage(commandBuffer, texture.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

	}

	// Transition to shader read.
	VkUtils::textureLayoutBarrier(commandBuffer, texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

}

void GPU::clearTexture(const Texture & texture, const glm::vec4& color){
	// See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#clears
	// for possibilities.
	// End current render pass.
	GPU::endRenderingIfNeeded();

	GPUContext* context = GPU::getInternal();
	VkCommandBuffer& commandBuffer = context->getRenderCommandBuffer();

	VkClearColorValue clearCol = {};
	clearCol.float32[0] = color[0];
	clearCol.float32[1] = color[1];
	clearCol.float32[2] = color[2];
	clearCol.float32[3] = color[3];

	VkImageSubresourceRange rangeCol = {};
	rangeCol.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	rangeCol.baseArrayLayer = 0;
	rangeCol.layerCount = (texture.shape == TextureShape::D3) ? 1u : texture.depth;
	rangeCol.baseMipLevel = 0;
	rangeCol.levelCount = texture.levels;

	VkUtils::textureLayoutBarrier(commandBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkCmdClearColorImage(commandBuffer, texture.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearCol, 1, &rangeCol);
	VkUtils::textureLayoutBarrier(commandBuffer, texture, texture.gpu->defaultLayout);
}

void GPU::clearDepth(const Texture & texture, float depth){
	// See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#clears
	// for possibilities.
	// End current render pass.
	GPU::endRenderingIfNeeded();

	GPUContext* context = GPU::getInternal();
	VkCommandBuffer& commandBuffer = context->getRenderCommandBuffer();

	VkClearDepthStencilValue clearZ = {};
	clearZ.depth = depth;
	clearZ.stencil = 0u;

	VkImageSubresourceRange rangeZ = {};
	rangeZ.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	rangeZ.baseArrayLayer = 0;
	rangeZ.layerCount = (texture.shape == TextureShape::D3) ? 1u : texture.depth;
	rangeZ.baseMipLevel = 0;
	rangeZ.levelCount = texture.levels;

	VkUtils::textureLayoutBarrier(commandBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkCmdClearDepthStencilImage(commandBuffer, texture.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearZ, 1, &rangeZ);
	VkUtils::textureLayoutBarrier(commandBuffer, texture, texture.gpu->defaultLayout);
}


void GPU::setupBuffer(Buffer & buffer) {
	if(buffer.gpu) {
		buffer.gpu->clean();
	}
	// Create.
	buffer.gpu.reset(new GPUBuffer(buffer.type));
	const std::string& name = buffer.name();

	static const std::unordered_map<BufferType, VkBufferUsageFlags> types = {
		{ BufferType::VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
		{ BufferType::INDEX, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
		{ BufferType::UNIFORM, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT },
		{ BufferType::CPUTOGPU, VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
		{ BufferType::GPUTOCPU, VK_BUFFER_USAGE_TRANSFER_DST_BIT },
		{ BufferType::STORAGE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT}
	};
	const VkBufferUsageFlags type = types.at(buffer.type);

	static const std::unordered_map<BufferType, VmaMemoryUsage> usages = {
		{ BufferType::VERTEX, VMA_MEMORY_USAGE_GPU_ONLY },
		{ BufferType::INDEX, VMA_MEMORY_USAGE_GPU_ONLY },
		{ BufferType::UNIFORM, VMA_MEMORY_USAGE_CPU_TO_GPU  },
		{ BufferType::CPUTOGPU, VMA_MEMORY_USAGE_CPU_ONLY },
		{ BufferType::GPUTOCPU, VMA_MEMORY_USAGE_GPU_TO_CPU },
		{ BufferType::STORAGE, VMA_MEMORY_USAGE_GPU_ONLY }
	};

	const VmaMemoryUsage usage = usages.at(buffer.type);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = buffer.size;
	bufferInfo.usage = type;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = usage;
	if(buffer.gpu->mappable){
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	VmaAllocationInfo resultInfos = {};

	VK_RET(vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &(buffer.gpu->buffer), &(buffer.gpu->data), &resultInfos));

	VkUtils::setDebugName(_context, VK_OBJECT_TYPE_BUFFER, uint64_t(buffer.gpu->buffer), "%s", name.c_str());

	if(buffer.gpu->mappable){
		buffer.gpu->mapped = (char*)resultInfos.pMappedData;
	} else {
		buffer.gpu->mapped = nullptr;
	}

	++_metrics.buffers;
}

void GPU::uploadBuffer(const Buffer & buffer, size_t size, uchar * data, size_t offset) {
	if(!buffer.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU buffer." << std::endl;
		return;
	}

	if(size == 0) {
		Log::Warning() << Log::GPU << "No data to upload." << std::endl;
		return;
	}
	if(offset + size > buffer.size) {
		Log::Warning() << Log::GPU << "Not enough allocated space to upload." << std::endl;
		return;
	}

	// If the buffer is visible from the CPU side, we don't need an intermediate staging buffer.
	if(buffer.gpu->mappable){
		if(!buffer.gpu->mapped){
			VK_RET(vmaMapMemory(_allocator, buffer.gpu->data, (void**)&buffer.gpu->mapped));
		}
		std::memcpy(buffer.gpu->mapped + offset, data, size);
		flushBuffer(buffer, size, offset);
		++_metrics.uploads;
		return;
	}

	// Otherwise, create a transfer buffer.
	Buffer transferBuffer(size, BufferType::CPUTOGPU, "BufferStaging");
	transferBuffer.upload(size, data, 0);
	// Copy operation.
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = offset;
	copyRegion.size = size;
	vkCmdCopyBuffer(_context.getUploadCommandBuffer(), transferBuffer.gpu->buffer, buffer.gpu->buffer, 1, &copyRegion);

}

void GPU::downloadBufferSync(const Buffer & buffer, size_t size, uchar * data, size_t offset) {
	if(!buffer.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU buffer." << std::endl;
		return;
	}
	if(offset + size > buffer.size) {
		Log::Warning() << Log::GPU << "Not enough available data to download." << std::endl;
		return;
	}

	// If the buffer is visible from the CPU side, we don't need an intermediate staging buffer.
	if(buffer.gpu->mappable){

		if(!buffer.gpu->mapped){
			VK_RET(vmaMapMemory(_allocator, buffer.gpu->data, (void**)&buffer.gpu->mapped));
		}
		vmaInvalidateAllocation(_allocator, buffer.gpu->data, offset, size);
		std::memcpy(data, buffer.gpu->mapped + offset, size);
		++_metrics.downloads;
		return;
	}

	// Otherwise, create a transfer buffer.
	Buffer transferBuffer(size, BufferType::GPUTOCPU, "Download");

	// Copy operation.
	VkCommandBuffer commandBuffer = VkUtils::beginSyncOperations(_context);
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = offset;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, buffer.gpu->buffer, transferBuffer.gpu->buffer, 1, &copyRegion);
	VkUtils::endSyncOperations(commandBuffer, _context);
	transferBuffer.download(size, data, 0);

}

void GPU::flushBuffer(const Buffer & buffer, size_t size, size_t offset){
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
	totalSize += 3 * mesh.bitangents.size();
	totalSize += 3 * mesh.colors.size();
	totalSize *= sizeof(float);

	// Create a staging buffer to host the geometry data (to avoid creating a staging buffer for each sub-upload).
	std::vector<uchar> vertexBufferData(totalSize);

	GPUMesh::State& state = mesh.gpu->state;
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
		{ reinterpret_cast<uchar*>(mesh.bitangents.data()), mesh.bitangents.size(), 3},
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
		state.bindings.back().stride = uint32_t(elementSize);
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
	mesh.gpu->count = mesh.indices.size();
	mesh.gpu->vertexBuffer.reset(new Buffer(totalSize, BufferType::VERTEX, "Vertices " + mesh.name()));
	mesh.gpu->indexBuffer.reset(new Buffer(inSize, BufferType::INDEX, "Indices " + mesh.name()));

	mesh.gpu->vertexBuffer->upload(totalSize, vertexBufferData.data(), 0);
	mesh.gpu->indexBuffer->upload(inSize, reinterpret_cast<unsigned char *>(mesh.indices.data()), 0);

	// Replicate the buffer as many times as needed for each attribute.
	state.buffers.resize(state.offsets.size(), mesh.gpu->vertexBuffer->gpu->buffer);

}


void GPU::bindGraphicsPipelineIfNeeded(){
	if(_state.pass.depthStencil == Layout::NONE && _state.pass.colors[0] == Layout::NONE){
		Log::Error() << "GPU: We are not in a render pass." << std::endl;
		return;
	}
	// Possibilities:
	// * we have started a new render pass
	bool shouldBindPipeline = _context.newRenderPass;
	_context.newRenderPass = false;
	// * state is outdated, create/retrieve new pipeline
	if(!_state.isGraphicsEquivalent(_lastState)){
		_context.graphicsPipeline = _context.pipelineCache.getGraphicsPipeline(_state);
		// Preserve the current compute program.
		Program* lastComputeUsed = _lastState.computeProgram;
		_lastState = _state;
		_lastState.computeProgram = lastComputeUsed;
		shouldBindPipeline = true;
	}

	if(shouldBindPipeline){
		vkCmdBindPipeline(_context.getRenderCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _context.graphicsPipeline);
		++_metrics.pipelineBindings;
	}
}


void GPU::bindComputePipelineIfNeeded(){
	if(_state.pass.depthStencil != Layout::NONE || _state.pass.colors[0] != Layout::NONE){
		Log::Error() << "GPU: We are in a render pass." << std::endl;
		return;
	}

	// Possibilities:
	// * we have just finished a render pass
	bool shouldBindPipeline = _context.hadRenderPass;
	_context.hadRenderPass = false;

	// * state is outdated, create/retrieve new pipeline
	if(!_state.isComputeEquivalent(_lastState)){
		_context.computePipeline = _context.pipelineCache.getComputePipeline(_state);
		// Save the current compute program.
		_lastState.computeProgram = _state.computeProgram;
		shouldBindPipeline = true;
	}

	if(shouldBindPipeline){
		vkCmdBindPipeline(_context.getRenderCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, _context.computePipeline);
		++_metrics.pipelineBindings;
	}
}

void GPU::drawMesh(const Mesh & mesh) {
	_state.mesh = mesh.gpu.get();

	bindGraphicsPipelineIfNeeded();
	_state.graphicsProgram->update();

	vkCmdBindVertexBuffers(_context.getRenderCommandBuffer(), 0, uint32_t(mesh.gpu->state.offsets.size()), mesh.gpu->state.buffers.data(), mesh.gpu->state.offsets.data());
	vkCmdBindIndexBuffer(_context.getRenderCommandBuffer(), mesh.gpu->indexBuffer->gpu->buffer, 0, VK_INDEX_TYPE_UINT32);
	++_metrics.meshBindings;

	vkCmdDrawIndexed(_context.getRenderCommandBuffer(), static_cast<uint32_t>(mesh.gpu->count), 1, 0, 0, 0);
	++_metrics.drawCalls;
}

void GPU::drawTesselatedMesh(const Mesh & mesh, uint patchSize){
	_state.patchSize = patchSize;
	
	drawMesh(mesh);
}

void GPU::drawQuad(){
	_state.mesh = _quad.gpu.get();

	bindGraphicsPipelineIfNeeded();

	_state.graphicsProgram->update();

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(_context.getRenderCommandBuffer(), 0, 1, &(_quad.gpu->vertexBuffer->gpu->buffer), &offset);
	vkCmdDraw(_context.getRenderCommandBuffer(), 3, 1, 0, 0);
	++_metrics.quadCalls;
}

void GPU::dispatch(uint width, uint height, uint depth){

	endRenderingIfNeeded();
	bindComputePipelineIfNeeded();
	// Ensure all resources are in the proper state for compute use (including
	// synchronization with a previous compute shader).
	_state.computeProgram->transitionResourcesTo(Program::Type::COMPUTE);
	// Update and bind descriptors (after layout transitions)
	_state.computeProgram->update();
	// Dispatch.
	const glm::uvec3& localSize = _state.computeProgram->size();
	const glm::uvec3 paddedSize = glm::uvec3(width, height, depth) + localSize - glm::uvec3(1u);
	const glm::uvec3 groupSize = paddedSize / localSize;
	vkCmdDispatch(_context.getRenderCommandBuffer(), groupSize[0], groupSize[1], groupSize[2]);

	// Restore all resources to their graphic state.
	// This is quite wasteful if resources are then used by another compute shader.
	// But we can't restore resources just before a draw call because we can't have
	// arbitrary layout barriers in a render pass...
	// A solution would be to keep track of unrestored resources on the context
	// and transition them before starting the next render pass.
	_state.computeProgram->transitionResourcesTo(Program::Type::GRAPHICS);
}

void GPU::beginFrameCommandBuffers() {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	VkCommandBuffer& commandBuffer = _context.getRenderCommandBuffer();
	VkCommandBuffer& commandBufferUpload = _context.getUploadCommandBuffer();
	VK_RET(vkBeginCommandBuffer(commandBuffer, &beginInfo));
	VK_RET(vkBeginCommandBuffer(commandBufferUpload, &beginInfo));
}

void GPU::submitFrameCommandBuffers() {
	VkCommandBuffer& commandBuffer = _context.getRenderCommandBuffer();
	VkCommandBuffer& commandBufferUpload = _context.getUploadCommandBuffer();
	VK_RET(vkEndCommandBuffer(commandBuffer));
	VK_RET(vkEndCommandBuffer(commandBufferUpload));

	// Submit them.
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	// Start with the uploads.
	submitInfo.pCommandBuffers = &commandBufferUpload;
	VK_RET(vkQueueSubmit(_context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_RET(vkQueueWaitIdle(_context.graphicsQueue));
	// Then the rendering (as it might use uploaded data).
	submitInfo.pCommandBuffers = &commandBuffer;
	VK_RET(vkQueueSubmit(_context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_RET(vkQueueWaitIdle(_context.graphicsQueue));
}

void GPU::flush(){
	GPU::endRenderingIfNeeded();

	// End both command buffers.
	GPU::submitFrameCommandBuffers();

	// Perform copies and destructions.
	processAsyncTasks(true);
	processDestructionRequests();

	// Re-open command buffers.
	GPU::beginFrameCommandBuffers();
}

void GPU::nextFrame(){
	processAsyncTasks();
	processDestructionRequests();

	_context.nextFrame();
	_context.pipelineCache.freeOutdatedPipelines();

	// Save and reset stats.
	_metricsPrevious = _metrics;
	_metrics.resetPerFrameMetrics();
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
	VK_RET(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtsCount, nullptr));
	std::vector<VkExtensionProperties> instanceExts(instanceExtsCount);
	VK_RET(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtsCount, instanceExts.data()));
	for(const auto& ext : instanceExts){
		names.emplace_back(ext.extensionName);
	}
	// Layers too.
	names.emplace_back("-- Layers --------");
	uint32_t layerCount;
	VK_RET(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
	std::vector<VkLayerProperties> availableLayers(layerCount);
	VK_RET(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));
	for(const auto& layer : availableLayers){
		names.emplace_back(layer.layerName);
	}
	// Get available device extensions.
	if(_context.physicalDevice != VK_NULL_HANDLE){
		names.emplace_back("-- Device --------");
		uint32_t deviceExtsCount;
		VK_RET(vkEnumerateDeviceExtensionProperties(_context.physicalDevice, nullptr, &deviceExtsCount, nullptr));
		std::vector<VkExtensionProperties> deviceExts(deviceExtsCount);
		VK_RET(vkEnumerateDeviceExtensionProperties(_context.physicalDevice, nullptr, &deviceExtsCount, deviceExts.data()));
		for(const auto& ext : deviceExts){
			names.emplace_back(ext.extensionName);
		}
	}
	return names;
}

void GPU::setViewport(int x, int y, int w, int h) {
	VkViewport vp;
	vp.x = float(x);
	vp.y = float(y);
	vp.width = float(w);
	vp.height = float(h);
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;
	vkCmdSetViewport(_context.getRenderCommandBuffer(), 0, 1, &vp);
	VkRect2D scissor;
	scissor.offset.x = int32_t(x);
	scissor.offset.y = int32_t(y);
	scissor.extent.width = uint32_t(w);
	scissor.extent.height = uint32_t(h);
	vkCmdSetScissor(_context.getRenderCommandBuffer(), 0, 1, &scissor);
}

void GPU::setViewport(const Texture& tex) {
	GPU::setViewport(0, 0, tex.width, tex.height);
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
	_state.stencilValue = value;
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

void GPU::blitDepth(const Texture & src, const Texture & dst) {
	blit(src, dst, Filter::NEAREST);
}

void GPU::blit(const Texture & src, const Texture & dst, Filter filter) {
	GPU::endRenderingIfNeeded();
	VkCommandBuffer& commandBuffer = _context.getRenderCommandBuffer();

	const glm::uvec2 srcSize(src.width, src.height);
	const glm::uvec2 dstSize(dst.width, dst.height);
	const uint mipCount = src.levels;
	const uint layerCount = src.shape == TextureShape::D3 ? 1 : src.depth;
	assert(mipCount == dst.levels);
	assert(layerCount == (dst.shape == TextureShape::D3 ? 1 : dst.depth));

	VkUtils::blitTexture(commandBuffer, src, dst, 0, 0, mipCount, 0, 0, layerCount, glm::uvec2(0), srcSize, glm::uvec2(0), dstSize, filter);
	_metrics.blitCount += 1;
}

void GPU::blit(const Texture & src, const Texture & dst, size_t lSrc, size_t lDst, Filter filter) {
	GPU::endRenderingIfNeeded();
	VkCommandBuffer& commandBuffer = _context.getRenderCommandBuffer();

	const glm::uvec2 srcSize(src.width, src.height);
	const glm::uvec2 dstSize(dst.width, dst.height);
	const uint mipCount = src.levels;
	assert(mipCount == dst.levels);

	VkUtils::blitTexture(commandBuffer, src, dst, 0, 0, mipCount, uint(lSrc), uint(lDst), 1, glm::uvec2(0), srcSize, glm::uvec2(0), dstSize, filter);
	_metrics.blitCount += 1;
}

void GPU::blit(const Texture & src, const Texture & dst, size_t lSrc, size_t lDst, size_t mipSrc, size_t mipDst, Filter filter) {
	GPU::endRenderingIfNeeded();
	VkCommandBuffer& commandBuffer = _context.getRenderCommandBuffer();

	const glm::uvec2 srcSize(src.width, src.height);
	const glm::uvec2 dstSize(dst.width, dst.height);

	VkUtils::blitTexture(commandBuffer, src, dst, uint(mipSrc), uint(mipDst), 1, uint(lSrc), uint(lDst), 1, glm::uvec2(0), srcSize, glm::uvec2(0), dstSize, filter);
	_metrics.blitCount += 1;
}

void GPU::endRenderingIfNeeded(){
	// No active attachments.
	if(_state.pass.depthStencil == Layout::NONE && _state.pass.colors[0] == Layout::NONE){
		assert(!_context.inRenderPass);
		return;
	}
	assert(false);
	GPU::endRender();
}

void GPU::endRender(){
	assert(_context.inRenderPass);

	VkCommandBuffer& commandBuffer = _context.getRenderCommandBuffer();
	vkCmdEndRenderingKHR(commandBuffer);
	++_metrics.renderPasses;
	_context.inRenderPass = false;
	_context.hadRenderPass = true;

	const uint attachCount = _state.colors.size();
	for(uint cid = 0; cid < attachCount; ++cid ){
		const Texture* color = _state.colors[cid];
		if(!color) {
			break;
		}
		VkUtils::imageLayoutBarrier(commandBuffer, *(color->gpu), color->gpu->defaultLayout, _state.pass.mipStart, _state.pass.mipCount, _state.pass.layerStart, _state.pass.layerCount);
	}
	const Texture* depth = _state.depthStencil;
	if(depth != nullptr){
		VkUtils::imageLayoutBarrier(commandBuffer, *(depth->gpu), depth->gpu->defaultLayout, _state.pass.mipStart, _state.pass.mipCount, _state.pass.layerStart, _state.pass.layerCount);
	}
	_state.depthStencil = nullptr;
	_state.colors.fill(nullptr);
	_state.pass = {};
}

void GPU::getState(GPUState& state) {
	state = _state;
}


const GPU::Metrics & GPU::getMetrics(){
	return _metricsPrevious;
}

void GPU::cleanup(){
	VK_RET(vkDeviceWaitIdle(_context.device));

	// Clean all remaining resources.
	_quad.clean();

	for(auto& alloc : _context.queryAllocators){
		alloc.second.clean();
	}

	_context.textureTasks.clear();

	_context.frameIndex += 100;
	processDestructionRequests();
	
	_context.pipelineCache.clean();
	_context.descriptorAllocator.clean();
	_context.samplerLibrary.clean();

	assert(_context.resourcesToDelete.empty());

	vkDestroyCommandPool(_context.device, _context.commandPool, nullptr);

	vmaDestroyAllocator(_allocator);

	vkDestroyDevice(_context.device, nullptr);
	ShaderCompiler::cleanup();
}

void GPU::clean(GPUTexture & tex){
	_context.resourcesToDelete.emplace_back();
	ResourceToDelete& rsc = _context.resourcesToDelete.back();
	rsc.view = tex.view;
	rsc.image = tex.image;
	rsc.data = tex.data;
	rsc.frame = _context.frameIndex;

	const uint mipCount = uint(tex.views.size());
	const uint layerCount = tex.views.empty() ? 0u : uint(tex.views[0].views.size());

	for(uint mid = 0; mid < mipCount; ++mid){
		_context.resourcesToDelete.emplace_back();
		ResourceToDelete& rsc = _context.resourcesToDelete.back();
		rsc.view = tex.views[mid].mipView;
		rsc.frame = _context.frameIndex;

		assert(layerCount == tex.views[mid].views.size());
		for(uint lid = 0; lid < layerCount; ++lid){
			_context.resourcesToDelete.emplace_back();
			ResourceToDelete& rsc = _context.resourcesToDelete.back();
			rsc.view = tex.views[mid].views[lid];
			rsc.frame = _context.frameIndex;
		}
	}
	tex.views.clear();
	--_metrics.textures;
}

void GPU::clean(GPUMesh & mesh){
	// Nothing to do, buffers will all be cleaned up.
	(void)mesh;
}

void GPU::clean(GPUBuffer & buffer){
	_context.resourcesToDelete.emplace_back();
	ResourceToDelete& rsc = _context.resourcesToDelete.back();
	rsc.buffer = buffer.buffer;
	rsc.data = buffer.data;
	rsc.frame = _context.frameIndex;
	--_metrics.buffers;
}

void GPU::clean(Program & program){
	
	vkDestroyPipelineLayout(_context.device, program._state.layout, nullptr);
	// Skip the static samplers.
	const size_t layoutCount = program._state.setLayouts.size();
	if(layoutCount > 0){
		// Skip the static samplers.
		for(size_t lid = 0; lid < layoutCount; ++lid){
			if(lid == SAMPLERS_SET){
				continue;
			}
			VkDescriptorSetLayout& setLayout = program._state.setLayouts[lid];
			vkDestroyDescriptorSetLayout(_context.device, setLayout, nullptr);
		}
	}
	for(Program::Stage& stage : program._stages){
		vkDestroyShaderModule(_context.device, stage.module, nullptr);
		stage.reset();
	}
}


void GPU::processDestructionRequests(){
	const uint64_t currentFrame = _context.frameIndex;

	if(_context.resourcesToDelete.empty() || (currentFrame < 2)){
		return;
	}

	while(!_context.resourcesToDelete.empty()){
		ResourceToDelete& rsc = _context.resourcesToDelete.front();
		// If the following resources are too recent, they might still be used by in flight frames.
		if(rsc.frame >= currentFrame - 2){
			break;
		}
		if(rsc.view != VK_NULL_HANDLE){
			vkDestroyImageView(_context.device, rsc.view, nullptr);
		}
		if(rsc.sampler != VK_NULL_HANDLE){
			vkDestroySampler(_context.device, rsc.sampler, nullptr);
		}
		if(rsc.image != VK_NULL_HANDLE){
			vmaDestroyImage(_allocator, rsc.image, rsc.data);
		}
		if(rsc.buffer != VK_NULL_HANDLE){
			vmaDestroyBuffer(_allocator, rsc.buffer, rsc.data);
		}
		_context.resourcesToDelete.pop_front();
	}
}

void GPU::processAsyncTasks(bool forceAll){
	const uint64_t currentFrame = _context.frameIndex;

	if(_context.textureTasks.empty() || (!forceAll && (currentFrame < 2))){
		return;
	}

	while(!_context.textureTasks.empty()){
		AsyncTextureTask& tsk = _context.textureTasks.front();
		// If the following requests are too recent, they might not have completed yet.
		if(!forceAll && (tsk.frame + 2 >= currentFrame)){
			break;
		}

		// Make sure the buffer is flushed.
		GPU::flushBuffer(*tsk.dstBuffer, tsk.dstBuffer->size, 0);

		// Copy from the buffer to each image of the destination.
		size_t currentOffset = 0;
		const uint firstImage = tsk.dstImageRange[0];
		const uint imageCount = tsk.dstImageRange[1];

		for(uint iid = firstImage; iid < firstImage + imageCount; ++iid){
			Image& img = tsk.dstTexture->images[iid];

			// Copy the data to the image.
			const size_t imgSize = img.pixels.size() * sizeof(float);
			std::memcpy(img.pixels.data(), tsk.dstBuffer->gpu->mapped + currentOffset, imgSize);
			currentOffset += imgSize;

		}

		// User-defined callback.
		tsk.callback(*tsk.dstTexture);
		_context.textureTasks.pop_front();
		++_metrics.downloads;
	}
}

GPUState GPU::_state;
GPUState GPU::_lastState;
GPU::Metrics GPU::_metrics;
GPU::Metrics GPU::_metricsPrevious;
Mesh GPU::_quad("Quad");

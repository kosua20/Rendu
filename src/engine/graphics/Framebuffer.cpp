#include "graphics/Framebuffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "renderers/DebugViewer.hpp"


Framebuffer::Framebuffer(uint width, uint height, const Descriptor & descriptor, bool depthBuffer, const std::string & name) :
	Framebuffer(TextureShape::D2, width, height, 1, 1, std::vector<Descriptor>(1, descriptor), depthBuffer, name) {
}

Framebuffer::Framebuffer(uint width, uint height, const std::vector<Descriptor> & descriptors, bool depthBuffer, const std::string & name) :
	Framebuffer(TextureShape::D2, width, height, 1, 1, descriptors, depthBuffer, name) {
}

VkRenderPass Framebuffer::createRenderpass(Operation colorOp, Operation depthOp, Operation stencilOp, bool presentable){

	const size_t attachCount = _colors.size() + (_hasDepth ? 1 : 0);
	std::vector<VkAttachmentDescription> attachDescs(attachCount);
	std::vector<VkAttachmentReference> attachRefs(attachCount);

	static const std::array<VkAttachmentLoadOp, 3> ops = {VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_DONT_CARE};

	const VkAttachmentLoadOp colorLoad = ops[uint(colorOp)];
	const VkAttachmentStoreOp colorStore = VK_ATTACHMENT_STORE_OP_STORE;
	const VkAttachmentLoadOp 	depthLoad = ops[uint(depthOp)];
	const VkAttachmentStoreOp depthStore = VK_ATTACHMENT_STORE_OP_STORE; // could be DONT_CARE when depth is internal ?
	const VkAttachmentLoadOp 	stencilLoad = ops[uint(stencilOp)];
	const VkAttachmentStoreOp stencilStore = VK_ATTACHMENT_STORE_OP_STORE;  // could be DONT_CARE when depth is internal ?

	for(size_t cid = 0; cid < _colors.size(); ++cid){
		VkAttachmentDescription& desc = attachDescs[cid];
		desc.format = _colors[cid].gpu->format;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.loadOp = colorLoad;
		desc.storeOp = colorStore;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		desc.finalLayout = presentable ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference& ref = attachRefs[cid];
		ref.attachment = cid;
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	// Depth is the last attachment.
	if(_hasDepth){
		VkAttachmentDescription& desc = attachDescs.back();
		desc.format = _depth.gpu->format;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.loadOp = depthLoad;
		desc.storeOp = depthStore;
		desc.stencilLoadOp = stencilLoad;
		desc.stencilStoreOp = stencilStore;
		desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference& ref = attachRefs.back();
		ref.attachment = static_cast<uint32_t>(_colors.size());
		ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	// Subpass.
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(_colors.size());
	subpass.pColorAttachments = attachRefs.data();
	subpass.pDepthStencilAttachment = _hasDepth ? &attachRefs.back() : nullptr;

	// Dependencies.
	std::array<VkSubpassDependency, 1> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = 0;

	// Render pass.
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachDescs.size());
	renderPassInfo.pAttachments = attachDescs.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	GPUContext* context = GPU::getInternal();
	VkRenderPass pass = VK_NULL_HANDLE;
	if(vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &pass) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create render pass." << std::endl;
	}
	return pass;
}


Framebuffer::Framebuffer(TextureShape shape, uint width, uint height, uint depth, uint mips, const std::vector<Descriptor> & descriptors, bool depthBuffer, const std::string & name) : _name(name), 
	_width(width), _height(height), _mips(mips) {

	// Check that the shape is supported.
	_shape = shape;
	if(_shape != TextureShape::D2 && _shape != TextureShape::Array2D && _shape != TextureShape::Cube && _shape != TextureShape::ArrayCube){
		Log::Error() << Log::GPU << "Unsupported framebuffer shape." << std::endl;
		return;
	}
	if(shape == TextureShape::D2){
		_layers = 1;
	} else if(shape == TextureShape::Cube){
		_layers = 6;
	} else if(shape == TextureShape::ArrayCube){
		_layers = 6 * depth;
	} else {
		_layers = depth;
	}

	uint cid = 0;
	for(const auto & descriptor : descriptors) {
		// Create the color texture to store the result.
		const Layout & format		  = descriptor.typedFormat();
		const bool isDepthComp		  = format == Layout::DEPTH_COMPONENT16 || format == Layout::DEPTH_COMPONENT24 || format == Layout::DEPTH_COMPONENT32F;
		const bool hasStencil = format == Layout::DEPTH24_STENCIL8 || format == Layout::DEPTH32F_STENCIL8;

		if(isDepthComp || hasStencil) {
			_hasDepth	  = true;
			_depth.width  = _width;
			_depth.height = _height;
			_depth.depth  = _layers;
			_depth.levels = _mips;
			_depth.shape  = shape;
			GPU::setupTexture(_depth, descriptor, true);

		} else {
			_colors.emplace_back("Color " + std::to_string(cid++));
			Texture & tex = _colors.back();
			tex.width     = _width;
			tex.height	  = _height;
			tex.depth	  = _layers;
			tex.levels	  = _mips;
			tex.shape	  = shape;
			GPU::setupTexture(tex, descriptor, true);

		}
	}

	if(!_hasDepth && depthBuffer) {
		_depth.width  = _width;
		_depth.height = _height;
		_depth.levels = _mips;
		_depth.depth  = 1;
		_depth.shape  = TextureShape::D2;
		GPU::setupTexture(_depth, {Layout::DEPTH_COMPONENT32F, Filter::NEAREST, Wrap::CLAMP}, true);
		_hasDepth = true;
	}

	// Populate all render passes. If this is too wasteful (27 render passes), we could create them on request and cache them.
	populateRenderPasses(false);
	populateLayoutState();

	// Create the framebuffer.
	finalizeFramebuffer();

	DebugViewer::trackDefault(this);
}

void Framebuffer::populateRenderPasses(bool isBackbuffer){
	const uint operationCount = _renderPasses.size();
	for(uint cid = 0; cid < operationCount; ++cid){
		for(uint did = 0; did < operationCount; ++did){
			for(uint sid = 0; sid < operationCount; ++sid){
				_renderPasses[cid][did][sid] = createRenderpass(Operation(cid), Operation(did), Operation(sid), isBackbuffer);
			}
		}
	}
}

void Framebuffer::populateLayoutState(){
	for(uint cid = 0; cid < _colors.size(); ++cid){
		_state.colors.push_back(_colors[cid].gpu->descriptor().typedFormat());
	}
	if(_hasDepth){
		_state.hasDepth = true;
		_state.depth = _depth.gpu->descriptor().typedFormat();
	}
}

void Framebuffer::finalizeFramebuffer(){

	// Finalize the texture layouts.
	// By not using the default (shader read only) layout, we ensure that we don't try to read in a framebuffer
	// texture before having filled it with some data (clear or draw).
	GPUContext* context = GPU::getInternal();
	VkCommandBuffer commandBuffer = VkUtils::startOneTimeCommandBuffer(*context);
	for(size_t cid = 0; cid < _colors.size(); ++cid){
		VkUtils::textureLayoutBarrier(commandBuffer, _colors[cid], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	if(_hasDepth){
		VkUtils::textureLayoutBarrier(commandBuffer, _depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	VkUtils::endOneTimeCommandBuffer(commandBuffer, *context);

	const uint attachCount = _colors.size() + (_hasDepth ? 1 : 0);

	_framebuffers.resize(_mips);
	// Generate per-mip per-layer framebuffers.
	for(uint mid = 0; mid < _mips; ++mid){
		_framebuffers[mid].resize(_layers);

		for(uint lid = 0; lid < _layers; ++lid){
			Slice& slice = _framebuffers[mid][lid];
			slice.attachments.resize(attachCount);

			//Register which attachments to draw to.
			for(size_t cid = 0; cid < _colors.size(); ++cid){
				
					// Create a custom one-level one-layer view.
				VkImageViewCreateInfo viewInfo = {};
				viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewInfo.image = _colors[cid].gpu->image;
				viewInfo.viewType =  VK_IMAGE_VIEW_TYPE_2D;
				viewInfo.format =  _colors[cid].gpu->format;
				viewInfo.subresourceRange.aspectMask = _colors[cid].gpu->aspect;
				viewInfo.subresourceRange.baseMipLevel = mid;
				viewInfo.subresourceRange.levelCount = 1;
				viewInfo.subresourceRange.baseArrayLayer = lid;
				viewInfo.subresourceRange.layerCount = 1;

				if (vkCreateImageView(context->device, &viewInfo, nullptr, &(slice.attachments[cid])) != VK_SUCCESS) {
					Log::Error() << Log::GPU << "Unable to create image view for framebuffer." << std::endl;
					return;
				}
			}

			// Depth attachment is last.
			if(_hasDepth){
				VkImageViewCreateInfo viewInfo = {};
				viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewInfo.image = _depth.gpu->image;
				viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewInfo.format =  _depth.gpu->format;
				viewInfo.subresourceRange.aspectMask =  _depth.gpu->aspect;
				viewInfo.subresourceRange.baseMipLevel = mid;
				viewInfo.subresourceRange.levelCount = 1;
				viewInfo.subresourceRange.baseArrayLayer = lid;
				viewInfo.subresourceRange.layerCount = 1;
				if (vkCreateImageView(context->device, &viewInfo, nullptr, &(slice.attachments.back())) != VK_SUCCESS) {
					Log::Error() << Log::GPU << "Unable to create image view for framebuffer." << std::endl;
					return;
				}
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// We can use any operations for the pass, they will all be compatible no matter the operations.
			framebufferInfo.renderPass = _renderPasses[0][0][0];
			framebufferInfo.attachmentCount = static_cast<uint32_t>(slice.attachments.size());
			framebufferInfo.pAttachments = slice.attachments.data();
			framebufferInfo.width = _width;
			framebufferInfo.height = _height;
			framebufferInfo.layers = 1; // We don't support multi-layered rendering for now.

			if(vkCreateFramebuffer(context->device, &framebufferInfo, nullptr, &slice.framebuffer) != VK_SUCCESS) {
				Log::Error() << Log::GPU << "Unable to create framebuffer." << std::endl;
			}

		}
	}

	// Create full framebuffer.
	{
		Slice& slice = _fullFramebuffer;
		slice.attachments.resize(attachCount);

		//Register which attachments to draw to.
		for(size_t cid = 0; cid < _colors.size(); ++cid){
			// Use the full view.
			slice.attachments[cid] = _colors[cid].gpu->view;
		}

		// Depth attachment is last.
		if(_hasDepth){
			slice.attachments[_colors.size()] = _depth.gpu->view;
		}

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// We can use any operations for the pass, they will all be compatible no matter the operations.
		framebufferInfo.renderPass = _renderPasses[0][0][0];
		framebufferInfo.attachmentCount = static_cast<uint32_t>(slice.attachments.size());
		framebufferInfo.pAttachments = slice.attachments.data();
		framebufferInfo.width = _width;
		framebufferInfo.height = _height;
		framebufferInfo.layers = _layers;

		if(vkCreateFramebuffer(context->device, &framebufferInfo, nullptr, &slice.framebuffer) != VK_SUCCESS) {
			Log::Error() << Log::GPU << "Unable to create framebuffer." << std::endl;
		}
	}

}

void Framebuffer::cleanRenderPasses(){
	GPUContext* context = GPU::getInternal();
	for(const auto& passes2 : _renderPasses){
		for(const auto& passes1 : passes2){
			for(const auto& pass : passes1){
				vkDestroyRenderPass(context->device, pass, nullptr);
			}
		}
	}
}

void Framebuffer::bind(const LoadOperation& colorOp, const LoadOperation& depthOp, const LoadOperation& stencilOp) const {
	bind(0, 0, colorOp, depthOp, stencilOp);
}

void Framebuffer::bind(size_t layer, size_t mip, const LoadOperation& colorOp, const LoadOperation& depthOp, const LoadOperation& stencilOp) const {

	const Slice& slice = _framebuffers[mip][layer];
	bind(slice, layer, 1, mip, 1, colorOp, depthOp, stencilOp);

}

void Framebuffer::bind(const Framebuffer::Slice& slice, size_t layer, size_t layerCount, size_t mip, size_t mipCount, const LoadOperation& colorOp, const LoadOperation& depthOp, const LoadOperation& stencilOp) const {
	const VkRenderPass& pass = _renderPasses[uint(colorOp.mode)][uint(depthOp.mode)][uint(stencilOp.mode)];


	GPU::unbindFramebufferIfNeeded();

	GPU::bindFramebuffer(*this, layer, layerCount, mip, mipCount);

	GPUContext* context = GPU::getInternal();
	VkCommandBuffer& commandBuffer = context->getCurrentCommandBuffer();

	// Retrieve clear colors and transition the regions of the resources we need.
	const uint attachCount = _colors.size() + (_hasDepth ? 1 : 0);
	std::vector<VkClearValue> clearVals(attachCount);

	for(uint cid = 0; cid < _colors.size(); ++cid){
		clearVals[cid].color.float32[0] = colorOp.value[0];
		clearVals[cid].color.float32[1] = colorOp.value[1];
		clearVals[cid].color.float32[2] = colorOp.value[2];
		clearVals[cid].color.float32[3] = colorOp.value[3];
		VkUtils::imageLayoutBarrier(commandBuffer, *_colors[cid].gpu, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, mip, mipCount, layer, layerCount);
	}
	if(_hasDepth){
		clearVals.back().depthStencil.depth = depthOp.value[0];
		clearVals.back().depthStencil.stencil = uint32_t(stencilOp.value[0]);
		VkUtils::imageLayoutBarrier(commandBuffer, *_depth.gpu, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, mip, mipCount, layer, layerCount);
	}

	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.pNext = nullptr;
	info.framebuffer = slice.framebuffer;
	info.renderPass = pass;
	info.clearValueCount = clearVals.size();
	info.pClearValues = clearVals.data();
	info.renderArea.extent = {uint32_t(_width), uint32_t(_height)};
	info.renderArea.offset = {0u, 0u};

	vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	context->newRenderPass = true;
	
}

void Framebuffer::setViewport() const {
	GPU::setViewport(0, 0, int(_width), int(_height));
}

void Framebuffer::resize(uint width, uint height) {
	_width  = width;
	_height = height;

	GPU::clean(*this);

	// Resize the renderbuffer.
	if(_hasDepth) {
		_depth.width  = _width;
		_depth.height = _height;
		GPU::setupTexture(_depth, _depth.gpu->descriptor(), true);
	}

	// Resize the textures.
	for(Texture & color : _colors) {
		color.width  = _width;
		color.height = _height;
		GPU::setupTexture(color, color.gpu->descriptor(), true);
	}

	finalizeFramebuffer();
	
}

void Framebuffer::resize(const glm::ivec2 & size) {
	resize(uint(size[0]), uint(size[1]));
}

void Framebuffer::clear(const glm::vec4 & color, float depth){
	// See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#clears
	// for possibilities.
	
	// Start a new pass with clearing instructions
	bind(_fullFramebuffer, 0, _layers, 0, _mips, color, depth, Operation::LOAD);
	// Finish it.
	GPU::unbindFramebufferIfNeeded();

}

bool Framebuffer::isEquivalent(const Framebuffer& other) const {
	return _state.isEquivalent(other.getLayoutState());
}

glm::vec3 Framebuffer::read(const glm::ivec2 & pos) const {
	glm::vec3 rgb(0.0f);
	//bind(0, 0, Mode::READ);
	//glReadPixels(pos.x, pos.y, 1, 1, GL_RGB, GL_FLOAT, &rgb[0]);
	//GPU::_metrics.downloads += 1;
	return rgb;
}

uint Framebuffer::attachments() const {
	return uint(_colors.size());
}

const Framebuffer::LayoutState& Framebuffer::getLayoutState() const {
	return _state;
}

bool Framebuffer::LayoutState::isEquivalent(const Framebuffer::LayoutState& other) const {
	if(other.colors.size() != colors.size()){
		return false;
	}

	if(hasDepth != other.hasDepth){
		return false;
	}

	// Two attachment references are compatible if they have matching format and sample count.
	// We can ignore: resolve, image layouts, load/store operations.

	if(hasDepth){
		// We can safely compare depths.
		if(depth != other.depth){
			return false;
		}
	}

	// We can safely compare color attachments.
	for(uint cid = 0; cid < colors.size(); ++cid){
		if(colors[cid] != other.colors[cid]){
			return false;
		}
	}
	return true;
}

Framebuffer::~Framebuffer() {
	if(_isBackbuffer){
		return;
	}

	DebugViewer::untrackDefault(this);

	GPU::clean(*this);

	// \todo Should this be move in GPU::clean? or not because these objects live longer than
	// the framebuffer object(s) (when resizing for instance).
	// Check swapchain too, to mutualize of possible.
	cleanRenderPasses();
	
	for(Texture& texture : _colors){
		texture.clean();
	}
	_colors.clear();
	if(_hasDepth){
		_depth.clean();
	}

}

Framebuffer * Framebuffer::_backbuffer = nullptr;

Framebuffer * Framebuffer::backbuffer() {
	return _backbuffer;
}

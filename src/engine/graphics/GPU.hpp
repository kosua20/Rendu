#pragma once

#include "resources/Mesh.hpp"
#include "resources/Texture.hpp"
#include "resources/Buffer.hpp"
#include "graphics/GPUTypes.hpp"
#include "graphics/Program.hpp"
#include "Common.hpp"

#include <functional>

// Forward declarations.
class Window;
struct GPUContext;

/**
 \brief Provide utility functions to communicate with the driver and GPU.
 \ingroup Graphics
 */
class GPU {

	friend class GPUTexture; ///< Access to deletion notifier for cached state update.
	friend class GPUBuffer; ///< Access to deletion notifier for cached state update.
	friend class GPUMesh; ///< Access to deletion notifier for cached state update.
	friend class Program; ///< Access to metrics.
	friend class Swapchain; ///< Access to command buffers.
	friend class PipelineCache; ///< Access to metrics.

public:

	/** Internal operation metrics. */
	struct Metrics {
		// Global statistics.
		unsigned long long uploads = 0; ///< Data upload to the GPU.
		unsigned long long downloads = 0; ///< Data download from the GPU.
		unsigned long long textures = 0; ///< Textures created.
		unsigned long long buffers = 0; ///< Buffers created.
		unsigned long long programs = 0; ///< Programs created.
		unsigned long long pipelines = 0; ///< Pipelines created.

		// Per-frame statistics.
		unsigned long long drawCalls = 0; ///< Mesh draw call.
		unsigned long long quadCalls = 0; ///< Full screen quad.
		unsigned long long pipelineBindings = 0; ///< Number of pipeline set operations.
		unsigned long long renderPasses = 0; ///< Number of render passes.
		unsigned long long meshBindings = 0; ///< Number of mesh bindings.
		unsigned long long blitCount = 0; ///< Texture blitting operations.

		/// Reset metrics that are measured over one frame.
		void resetPerFrameMetrics(){
			drawCalls = 0;
			quadCalls = 0;
			pipelineBindings = 0;
			renderPasses = 0;
			meshBindings = 0;
			blitCount = 0;
		}
	};
	
	/** Setup the GPU device in its initial state.
	 \param appName the name of the current executable
	 \return true if the setup was successful
	 */
	static bool setup(const std::string & appName);

	/** Setup a window swapchain, creating backbuffers and resources.
	 * \param window the window to create a swapchain for
	 * \return true if the setup was successful
	 */
	static bool setupWindow(Window * window);

	/** Create and link a graphics program using the shader code contained in the given strings.
	 \param program the program to compile
	 \param vertexContent the vertex shader string
	 \param fragmentContent the fragment shader string
	 \param tessControlContent the optional tessellation control shader string
	 \param tessEvalContent the optional tessellation evaluation shader string
	 \param debugInfos the name of the program, or any custom debug infos that will be logged.
	 */
	static void createGraphicsProgram(Program & program, const std::string & vertexContent, const std::string & fragmentContent, const std::string & tessControlContent, const std::string & tessEvalContent, const std::string & debugInfos);

	/** Create and link a compute program using the shader code contained in the given strings.
	 \param program the program to compile
	 \param computeContent the compute shader string
	 \param debugInfos the name of the program, or any custom debug infos that will be logged.
	 */
	static void createComputeProgram(Program & program, const std::string & computeContent, const std::string & debugInfos);

	/** Bind a program to use for rendering
	 \param program the program to use
	 */
	static void bindProgram(const Program & program);

	/** Begin rendering to a set of textures set as attachments.
	 \param colorOp the operation to perform on the color attachments
	 \param depthOp the operation to perform on the depth attachment
	 \param stencilOp the operation to perform on the stencil attachment
	 \param depthStencil the depth/stencil attachment (optional)
	 \param color0 the first color attachment (optional)
	 \param color1 the second color attachment (optional)
	 \param color2 the third color attachment (optional)
	 \param color3 the fourth color attachment (optional)
	 */
	static void bind(const Load& colorOp, const Load& depthOp, const Load& stencilOp, const Texture* depthStencil, const Texture* color0 = nullptr, const Texture* color1 = nullptr, const Texture* color2 = nullptr, const Texture* color3 = nullptr);

	/** Begin rendering to a set of textures set as attachments.
	 \param colorOp the operation to perform on the color attachments
	 \param color0 the first color attachment
	 \param color1 the second color attachment (optional)
	 \param color2 the third color attachment (optional)
	 \param color3 the fourth color attachment (optional)
	 */
	static void bind(const Load& colorOp, const Texture* color0, const Texture* color1 = nullptr, const Texture* color2 = nullptr, const Texture* color3 = nullptr);

	/** Begin rendering to a set of textures set as attachments.
	 \param depthOp the operation to perform on the depth attachment
	 \param stencilOp the operation to perform on the stencil attachment
	 \param depthStencil the depth/stencil attachment
	 */
	static void bind(const Load& depthOp, const Load& stencilOp, const Texture* depthStencil);

	/** Begin rendering to a set of textures set as attachments.
	 \param layer the texture layer to bind
	 \param mip the mip level to bind
	 \param colorOp the operation to perform on the color attachments
	 \param depthOp the operation to perform on the depth attachment
	 \param stencilOp the operation to perform on the stencil attachment
	 \param depthStencil the depth/stencil attachment (optional)
	 \param color0 the first color attachment (optional)
	 \param color1 the second color attachment (optional)
	 \param color2 the third color attachment (optional)
	 \param color3 the fourth color attachment (optional)
	 */
	static void bind(uint layer, uint mip, const Load& colorOp, const Load& depthOp, const Load& stencilOp, const Texture* depthStencil, const Texture* color0 = nullptr, const Texture* color1 = nullptr, const Texture* color2 = nullptr, const Texture* color3 = nullptr);

	/** Begin rendering to a set of textures set as attachments.
	 \param layer the texture layer to bind
	 \param mip the mip level to bind
	 \param colorOp the operation to perform on the color attachments
	 \param color0 the first color attachment
	 \param color1 the second color attachment (optional)
	 \param color2 the third color attachment (optional)
	 \param color3 the fourth color attachment (optional)
	 */
	static void bind(uint layer, uint mip, const Load& colorOp, const Texture* color0, const Texture* color1 = nullptr, const Texture* color2 = nullptr, const Texture* color3 = nullptr);

	/** Begin rendering to a set of textures set as attachments.
	 \param layer the texture layer to bind
	 \param mip the mip level to bind
	 \param depthOp the operation to perform on the depth attachment
	 \param stencilOp the operation to perform on the stencil attachment
	 \param depthStencil the depth/stencil attachment
	 */
	static void bind(uint layer, uint mip, const Load& depthOp, const Load& stencilOp, const Texture* depthStencil);

	/** Save a given texture content to the disk.
	 \param texture the texture to save
	 \param path the output image path
	 \param options output options (flip,...)
	 \note The output image extension will be automatically added based on the texture type and format.
	 \warning Export of small size float texture can create artifacts.
	 */
	static void saveTexture(Texture & texture, const std::string & path, Image::Save options);

	/** Create a GPU texture with a given layout and allocate it.
	 \param texture the texture to setup on the GPU
	 */
	static void setupTexture(Texture & texture);

	/** Upload a texture images data to the GPU.
	 \param texture the texture to upload
	 */
	static void uploadTexture(const Texture & texture);

	/** Download a texture images current data from the GPU.
	 \param texture the texture to download
	 \warning The CPU images of the texture will be overwritten.
	 \note The download will be performed when this function is called, so the data might be coming from a previous frame.
	 */
	static void downloadTextureSync(Texture & texture);

	/** Download a texture images current data from the GPU.
	 \param texture the texture to download
	 \param level the specific mip level to download
	 \warning The CPU images of the texture will be overwritten.
	 \note The download will be performed when this function is called, so the data might be coming from a previous frame.
	 */
	static void downloadTextureSync(Texture & texture, int level);

	/** Copy a texture GPU data at this point of the frame command buffer, and download the copied data once the frame is complete.
	 * \param texture the texture to download
	 * \param offset (x,y) offset of the region in the texture to download, from the top-left corner
	 * \param size dimensions of the region to download
	 * \param layerCount number of layers to download from
	 * \param callback function that will be called once the data has been download and stored in the images of the Texture parameter
	 * \return an async task ID
	 */ 
	static GPUAsyncTask downloadTextureAsync(const Texture& texture, const glm::uvec2& offset, const glm::uvec2& size, uint layerCount, std::function<void(const Texture&)> callback);

	/** Cancel an asynchronous download operation.
	 * \param id the ID of the task to cancel
	 */
	static void cancelAsyncOperation(const GPUAsyncTask& id);
	
	/** Generate a texture mipmaps on the GPU.
	 \param texture the texture to use
	 */
	static void generateMipMaps(const Texture & texture);

	/** Clear a texture content with a given color.
	 \param texture the texture to clear
	 \param color the color to use
	 */
	static void clearTexture(const Texture & texture, const glm::vec4& color);

	/** Clear a depth texture with a given depth.
	 \param texture the texture to clear/
	 \param depth the depth to use
	 */
	static void clearDepth(const Texture & texture, float depth);

	/** Create and allocate a GPU buffer.
	 \param buffer the buffer to setup on the GPU
	 */
	static void setupBuffer(Buffer & buffer);

	/** Upload data to a buffer on the GPU. It's possible to upload a subrange of the buffer data store.
	 \param buffer the buffer to upload to
	 \param size the amount of data to upload, in bytes
	 \param data pointer to the data to upload
	 \param offset optional offset in the buffer store
	 */
	static void uploadBuffer(const Buffer & buffer, size_t size, uchar * data, size_t offset = 0);

	/** Download data from a buffer on the GPU. It's possible to download a subrange of the buffer data store.
	 \param buffer the buffer to download from
	 \param size the amount of data to download, in bytes
	 \param data pointer to the storage destination
	 \param offset optional offset in the buffer store
	 */
	static void downloadBufferSync(const Buffer & buffer, size_t size, uchar * data, size_t offset = 0);

	/** Ensure that a buffer region is visible from the CPU and up-to-date.
	 * \param buffer the buffer to flush
	 * \param size size of the region to flush
	 * \param offset start of the region in the buffer
	 */
	static void flushBuffer(const Buffer & buffer, size_t size, size_t offset);

	/** Mesh loading: send a mesh data to the GPU and set the input mesh GPU infos accordingly.
	 \param mesh the mesh to upload
	 \note The order of attribute locations is: position, normal, uvs, tangents, bitangents.
	 */
	static void setupMesh(Mesh & mesh);

	/** Draw indexed geometry.
	 \param mesh the mesh to draw
	 */
	static void drawMesh(const Mesh & mesh);

	/** Draw tessellated geometry.
	 \param mesh the mesh to tessellate and render
	 \param patchSize number of vertices to use in a patch
	 */
	static void drawTesselatedMesh(const Mesh & mesh, uint patchSize);

	/** Helper used to draw a fullscreen quad for texture processing.
	 \details Instead of story two-triangles geometry, it uses a single triangle covering the whole screen. For instance:
	 \verbatim
	  2: (-1,3),(0,2)
		  *
		  | \
		  |   \
		  |     \
		  |       \
		  |         \
		  *-----------*  1: (3,-1), (2,0)
	  0: (-1,-1), (0,0)
	  \endverbatim

	  \see GPU::Vert::Passthrough
	  \see GPU::Frag::Passthrough, GPU::Frag::Passthrough_pixelperfect
	*/
	static void drawQuad();

	/** Run a compute program by spawning computation threads.
	 \param width the number of threads to spawn on the X axis
	 \param height the number of threads to spawn on the Y axis
	 \param depth the number of threads to spawn on the Z axis
	 \note The work group count will automatically be computed based on the program local size.
	 */
	static void dispatch(uint width, uint height, uint depth);

	/** Flush current GPU commands and wait for all processing to be done.
	 */
	static void flush();

	/** Prepare GPU for next frame.
	 */
	static void nextFrame();

	/** Query the GPU driver and API infos.
	 \param vendor will contain the vendor name
	 \param renderer will contain the renderer name
	 \param version will contain the driver API version
	 \param shaderVersion will contain the shading language version
	 */
	static void deviceInfos(std::string & vendor, std::string & renderer, std::string & version, std::string & shaderVersion);

	/** Query the GPU supported extensions.
	 \return a list of extensions names
	 */
	static std::vector<std::string> supportedExtensions();

	/** Set the current viewport.
	 \param x horizontal coordinate
	 \param y vertical coordinate
	 \param w width
	 \param h height
	 */
	static void setViewport(int x, int y, int w, int h);

	/** Set the current viewport based on a texture dimensions.
	 \param tex the texture to use the dimensions
	 */
	static void setViewport(const Texture& tex);

	/** Enable or disable the depth test.
	 \param test should depth test be performed
	 */
	static void setDepthState(bool test);

	/** Configure depth testing.
	 \param test should depth test be performed
	 \param function the test function
	 \param write should the depth be written to the depth buffer
	 */
	static void setDepthState(bool test, TestFunction function, bool write);

	/** Enable or disable the stencil test.
	 \param test should stencil test be performed
	 \param write should the stencil be written to the stencil buffer
	 */
	static void setStencilState(bool test, bool write);

	/** Configure stencil testing.
	 \param test should stencil test be performed
	 \param function the test function
	 \param fail operation to perform if the stencil test fails
	 \param pass operation to perform if the stencil and depth tests succeed, or if the stencil test is disabled
	 \param depthFail operation to perform if the stencil test succeeds but not the depth test
	 \param value reference value used for comparison
	 \warning Stencil writing still happens even if testing is disabled.
	 */
	static void setStencilState(bool test, TestFunction function, StencilOp fail, StencilOp pass, StencilOp depthFail, uchar value);

	/** Enable or disable blending.
	\param test should blending be enabled
	*/
	static void setBlendState(bool test);

	/** Configure blending.
	\param test should blending be enabled
	\param equation the blending equation
	\param src the blending value to use for the source
	\param dst the blending value to use for the destination
	*/
	static void setBlendState(bool test, BlendEquation equation, BlendFunction src, BlendFunction dst);

	/** Enable or disable backface culling.
	\param cull should backfaces be culled
	*/
	static void setCullState(bool cull);

	/** Configure backface culling.
	 \param cull should backfaces be culled
	 \param culledFaces the set of faces to cull
	 */
	static void setCullState(bool cull, Faces culledFaces);

	/** Set the polygon rasterization mode.
	 \param mode the mode (filled, lines, points)
	 */
	static void setPolygonState(PolygonMode mode);

	/** Set the color write mask.
	 \param writeRed allow writes to the red channel
	 \param writeGreen allow writes to the green channel
	 \param writeBlue allow writes to the blue channel
	 \param writeAlpha allow writes to the alpha channel
	 */
	static void setColorState(bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha);

	/** Query the current GPU state.
	 \param state will be populated with the current GPU settings
	 */
	static void getState(GPUState & state);

	/** \return internal metrics for the current frame */
	static const Metrics & getMetrics();

	/** Blit the content of a depth texture into another one.
	 \param src the source texture
	 \param dst the destination texture
	 \note Depth is necessarily filtered using nearest neighbour.
	 */
	static void blitDepth(const Texture & src, const Texture & dst);

	/** Blit the content of a texture into another one, resizing the content accordingly.
	 \param src the source texture
	 \param dst the destination texture
	 \param filter the filtering to use for resizing
	 */
	static void blit(const Texture & src, const Texture & dst, Filter filter);

	/** Blit the content of a texture into another one, resizing the content accordingly.
	 \param src the source texture
	 \param dst the destination texture
	 \param lSrc the src layer to copy
	 \param lDst the dst layer to copy to
	 \param filter the filtering to use for resizing
	 */
	static void blit(const Texture & src, const Texture & dst, size_t lSrc, size_t lDst, Filter filter);

	/** Blit the content of a texture into another one, resizing the content accordingly.
	 \param src the source texture
	 \param dst the destination texture
	 \param lSrc the src layer to copy
	 \param lDst the dst layer to copy to
	 \param mipSrc the src mip level to copy
	 \param mipDst the dst mip level to copy to
	 \param filter the filtering to use for resizing
	 */
	static void blit(const Texture & src, const Texture & dst, size_t lSrc, size_t lDst, size_t mipSrc, size_t mipDst, Filter filter);

	/** \return the opaque internal GPU context. */
	static GPUContext* getInternal();

	/** Clean all created GPU structures. */
	static void cleanup();

private:

	/** Bind textures as a draw destination.
	  \param layer the texture layer to bind
	  \param mip the mip level to bind
	  \param colorOp the operation to perform on the color attachments
	  \param depthOp the operation to perform on the depth attachment
	  \param stencilOp the operation to perform on the stencil attachment
	  \param depthStencil the depth/stencil attachment (optional)
	  \param color0 the first color attachment (optional)
	  \param color1 the second color attachment (optional)
	  \param color2 the third color attachment (optional)
	  \param color3 the fourth color attachment (optional)
	  */
	static void bindAttachments(uint layer, uint mip, const Load& colorOp, const Load& depthOp, const Load& stencilOp, const Texture* depthStencil, const Texture* color0, const Texture* color1, const Texture* color2, const Texture* color3);
	
	/** If the GPU graphics state has changed, retrieve and bind the pipeline corresponding to the new state. */
	static void bindGraphicsPipelineIfNeeded();

	/** If the GPU compute state has changed, retrieve and bind the pipeline corresponding to the new state. */
	static void bindComputePipelineIfNeeded();

	/** End the current render pass if one is started. */
	static void endRenderingIfNeeded();

	/** Begin render and upload command buffers for this frame */
	static void beginFrameCommandBuffers();

	/** End and submit upload and render command buffers for this frame, guaranteeing sequential execution.
	 \note This will wait for all queue work to be complete before returning.
	 */
	static void submitFrameCommandBuffers();

	/** Clean a texture GPU object. 
	 * \param tex the object to delete
	 */
	static void clean(GPUTexture & tex);

	/** Clean a mesh GPU object. 
	 * \param mesh the object to delete
	 */
	static void clean(GPUMesh & mesh);

	/** Clean a buffer GPU object. 
	 * \param buffer the object to delete
	 */
	static void clean(GPUBuffer & buffer);

	/** Clean a shader program object. 
	 * \param program the object to delete
	 */
	static void clean(Program & program);

	/** Process destruction requests for which we are sure that the resources are not used anymore. */
	static void processDestructionRequests();

	/** Process async download tasks for frames that are complete.
	 \param forceAll process all pending requests, including for the current frame.
	 \warning When forcing all requests to be processed, you have to guarantee that all commands have been completed on the GPU. */
	static void processAsyncTasks(bool forceAll = false);

	static GPUState _state; ///< Current GPU state.
	static GPUState _lastState; ///< Previous draw call GPU state for caching.
	static Metrics _metrics; ///< Internal metrics (draw count, state changes,...).
	static Metrics _metricsPrevious; ///< Internal metrics for the last completed frame.
	static Mesh _quad; ///< Screen-covering triangle.

};

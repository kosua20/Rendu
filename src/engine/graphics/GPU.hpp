#pragma once

#include "resources/Mesh.hpp"
#include "resources/Texture.hpp"
#include "resources/Buffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Program.hpp"
#include "Common.hpp"
#include <map>


/**
 \addtogroup Graphics
 @{
 */

/// This macro is used to check for GPU errors with access to the file and line number where the error is detected.
#define checkGPUError() GPU::checkError(__FILE__, __LINE__, "");
/// This macro is used to check for GPU errors with access to the file and line number where the error is detected, along with additional user informations.
#define checkGPUErrorInfos(infos) GPU::checkError(__FILE__, __LINE__, infos);

/**@}*/

class Framebuffer;
class ScreenQuad;
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
	friend class Framebuffer; ///< Access to deletion notifier for cached state update.
	friend class Program; ///< Access to metrics.

public:

	/** Internal operation metrics. */
	struct Metrics {
		unsigned long drawCalls = 0; ///< Mesh draw call.
		unsigned long quadCalls = 0; ///< Full screen quad.
		unsigned long stateChanges = 0; ///< State changes.
		unsigned long textureBindings = 0; ///< Number of texture bindings.
		unsigned long framebufferBindings = 0; ///< Number of framebuffer bindings.
		unsigned long bufferBindings = 0; ///< Number of data buffer bindings.
		unsigned long vertexBindings = 0; ///< Number of vertex array bindings.
		unsigned long programBindings = 0; ///< Number of shade program bindings.
		unsigned long clearAndBlits = 0; ///< Framebuffer clearing and blitting operations.
		unsigned long uploads = 0; ///< Data upload to the GPU.
		unsigned long downloads = 0; ///< Data download from the GPU.
		unsigned long uniforms = 0; ///< Uniform update.
	};

	/** Check if any GPU error has been detected and log it.
	 \param file the current file
	 \param line the current line
	 \param infos additional user info
	 \return non zero if an error was encountered
	 */
	static int checkError(const char * file, int line, const std::string & infos);
	
	/** Setup the GPU device in its initial state.
	 \param appName the name of the current executable
	 \return true if the setup was successful
	 */
	static bool setup(const std::string & appName);

	static bool setupWindow(Window * window);

	/** Create and link a GLProgram using the shader code contained in the given strings.
	 \param vertexContent the vertex shader string
	 \param fragmentContent the fragment shader string
	 \param geometryContent the optional geometry shader string
	 \param tessControlContent the optional tesselation control shader string
	 \param tessEvalContent the optional tesselation evaluation shader string
	 \param debugInfos the name of the program, or any custom debug infos that will be logged.
	 \return the GPU ID of the program
	 */
	static void createProgram(Program & program, const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent,const std::string & debugInfos);

	/** Bind a program to use for rendering
	 \param program the program to use
	 */
	static void bindProgram(const Program & program);

	/** Bind a framebuffer as a draw destination.
	 \param framebuffer the framebuffer to bind as draw destination
	 */
	static void bindFramebuffer(const Framebuffer & framebuffer);

	/** Save a given framebuffer content to the disk.
	 \param framebuffer the framebuffer to save
	 \param path the output image path
	 \param flip should the image be vertically fliped before saving
	 \param ignoreAlpha should the alpha channel be ignored if it exists
	 \note The output image extension will be automatically added based on the framebuffer type and format.
	 \warning Export of small size float framebuffers can create artifacts.
	 */
	static void saveFramebuffer(const Framebuffer & framebuffer, const std::string & path, bool flip = true, bool ignoreAlpha = false);

	/** Create a GPU texture with a given layout and allocate it.
	 \param texture the texture to setup on the GPU
	 \param descriptor type and format information
	 */
	static void setupTexture(Texture & texture, const Descriptor & descriptor, bool drawable);

	/** Upload a texture images data to the GPU.
	 \param texture the texture to upload
	 */
	static void uploadTexture(const Texture & texture);

	/** Download a texture images data from the GPU.
	 \param texture the texture to download
	 \warning The CPU images of the texture will be overwritten.
	 */
	static void downloadTexture(Texture & texture);

	/** Download a texture images data from the GPU.
	 \param texture the texture to download
	 \param level the specific mip level to download
	 \warning The CPU images of the texture will be overwritten.
	 */
	static void downloadTexture(Texture & texture, int level);

	/** Generate a texture mipmaps on the GPU.
	 \param texture the texture to use
	 \note This will set the number of levels to 1000.
	 */
	static void generateMipMaps(const Texture & texture);

	/** Create and allocate a GPU buffer.
	 \param buffer the buffer to setup on the GPU
	 */
	static void setupBuffer(BufferBase & buffer);

	/** Upload data to a buffer on the GPU. It's possible to upload a subrange of the buffer data store.
	 \param buffer the buffer to upload to
	 \param size the amount of data to upload, in bytes
	 \param data pointer to the data to upload
	 \param offset optional offset in the buffer store
	 */
	static void uploadBuffer(const BufferBase & buffer, size_t size, uchar * data, size_t offset = 0);

	/** Download data from a buffer on the GPU. It's possible to download a subrange of the buffer data store.
	 \param buffer the buffer to download from
	 \param size the amount of data to download, in bytes
	 \param data pointer to the storage destination
	 \param offset optional offset in the buffer store
	 */
	static void downloadBuffer(const BufferBase & buffer, size_t size, uchar * data, size_t offset = 0);

	static void flushBuffer(const BufferBase & buffer, size_t size, size_t offset);

	/** Mesh loading: send a mesh data to the GPU and set the input mesh GPU infos accordingly.
	 \param mesh the mesh to upload
	 \note The order of attribute locations is: position, normal, uvs, tangents, binormals.
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

	/** Draw a fullscreen quad.*/
	static void drawQuad();

	/** Flush the GPU command pipelines and wait for all processing to be done.
	 */
	static void sync();

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

	/** Blit the content of a depthbuffer into another one.
	 \param src the source framebuffer
	 \param dst the destination framebuffer
	 \note Depth is necessarily filtered using nearest neighbour.
	 \warning This treat the current depth buffer setup as a 2D texture.
	 */
	static void blitDepth(const Framebuffer & src, const Framebuffer & dst);

	/** Blit the content of a framebuffer into another one, resizing the content accordingly.
	 \param src the source framebuffer
	 \param dst the destination framebuffer
	 \param filter the filtering to use for resizing
	 \warning Only the first color attachment will be blit.
	 */
	static void blit(const Framebuffer & src, const Framebuffer & dst, Filter filter);

	/** Blit the content of a framebuffer into another one, resizing the content accordingly.
	 \param src the source framebuffer
	 \param dst the destination framebuffer
	 \param lSrc the src layer to copy
	 \param lDst the dst layer to copy to
	 \param filter the filtering to use for resizing
	 */
	static void blit(const Framebuffer & src, const Framebuffer & dst, size_t lSrc, size_t lDst, Filter filter);

	/** Blit the content of a framebuffer into another one, resizing the content accordingly.
	 \param src the source framebuffer
	 \param dst the destination framebuffer
	 \param lSrc the src layer to copy
	 \param lDst the dst layer to copy to
	 \param mipSrc the src mip level to copy
	 \param mipDst the dst mip level to copy to
	 \param filter the filtering to use for resizing
	 */
	static void blit(const Framebuffer & src, const Framebuffer & dst, size_t lSrc, size_t lDst, size_t mipSrc, size_t mipDst, Filter filter);
	
	/** Blit the content of a texture into another one, resizing the content accordingly.
	 \param src the source texture
	 \param dst the destination texture
	 \param filter the filtering to use for resizing
	 */
	static void blit(const Texture & src, Texture & dst, Filter filter);

	/** Blit the content of a texture into a framebuffer first attachment, resizing the content accordingly.
	 \param src the source texture
	 \param dst the destination framebuffer
	 \param filter the filtering to use for resizing
	 \note This can be used to easily blit a color attachment that is not the first one.
	 */
	static void blit(const Texture & src, Framebuffer & dst, Filter filter);

	static GPUContext* getInternal();

	static void cleanup();
	
private:
	/** Read back the currently bound framebuffer to the CPU and save it in the best possible format on disk.
	 \param type the type of the framebuffer
	 \param format the format of the framebuffer
	 \param width the width of the region to save
	 \param height the height of the region to save
	 \param components the number of components of the framebuffer
	 \param path the output image path
	 \param flip should the image be vertically fliped before saving
	 \param ignoreAlpha should the alpha channel be ignored if it exists
	 \note The output image extension will be automatically added based on the framebuffer type and format.
	 */
	//static void savePixels(GLenum type, GLenum format, unsigned int width, unsigned int height, unsigned int components, const std::string & path, bool flip, bool ignoreAlpha);

	static void bindPipelineIfNeeded();

	static void blitTexture(VkCommandBuffer& commandBuffer, const Texture& src, const Texture& dst, uint mipStartSrc, uint mipStartDst, uint mipCount, uint layerStartSrc, uint layerStartDst, uint layerCount, Filter filter);

	static void clean(GPUTexture & tex);

	static void clean(GPUMesh & mesh);

	static void clean(GPUBuffer & buffer);

	static void clean(Framebuffer & framebuffer);

	static void clean(Program & program);

	static void cleanFrame();

	static GPUState _state; ///< Current GPU state for caching.
	static GPUState _lastState; ///< Current GPU state for caching.
	static Metrics _metrics; ///< Internal metrics (draw count, state changes,...).
	static Metrics _metricsPrevious; ///< Internal metrics for the last completed frame.
	static Mesh _quad; ///< The unique empty screenquad VAO.

};

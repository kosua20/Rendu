#pragma once

#include "resources/Mesh.hpp"
#include "resources/Texture.hpp"
#include "resources/Buffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"
#include <map>


/**
 \addtogroup Graphics
 @{
 */

/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected.
#define checkGLError() _checkGLError(__FILE__, __LINE__, "")
/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected, along with additional user informations.
#define checkGLErrorInfos(infos) _checkGLError(__FILE__, __LINE__, infos)

/** Check if any OpenGL error has been detected and log it.
 \param file the current file
 \param line the current line
 \param infos additional user info
 \return non zero if an error was encountered
 */
int _checkGLError(const char * file, int line, const std::string & infos);

/** Check if any OpenGL has been detected after setting up a framebuffer.
 \return non zero if an error was encountered
 */
int checkGLFramebufferError();

/**@}*/

class Framebuffer;

/**
 \brief Provide utility functions to communicate with the driver and GPU.
 \ingroup Graphics
 */
class GLUtilities {

public:
	/** Setup the GPU in its initial state.
	 */
	static void setup();

	/** Create a shader of a given type from a string. Extract additional informations from the shader.
	 \param prog the content of the shader
	 \param type the type of shader (GL_VERTEX_SHADER,...)
	 \param bindings will be filled with the samplers present in the shader and their user-defined locations
	 \param finalLog will contain the compilation log of the shader
	 \return the OpenGL ID of the shader object
	 */
	static GLuint loadShader(const std::string & prog, GLuint type, std::map<std::string, int> & bindings, std::string & finalLog);

	/** Create and link a GLProgram using the shader code contained in the given strings.
	 \param vertexContent the vertex shader string
	 \param fragmentContent the fragment shader string
	 \param geometryContent the optional geometry shader string
	 \param bindings will be filled with the samplers present in the shaders and their user-defined locations
	 \param debugInfos the name of the program, or any custom debug infos that will be logged.
	 \return the OpenGL ID of the program
	 */
	static GLuint createProgram(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, std::map<std::string, int> & bindings, const std::string & debugInfos);

	/** Save a given framebuffer content to the disk.
	 \param framebuffer the framebuffer to save
	 \param width the width of the region to save
	 \param height the height of the region to save
	 \param path the output image path
	 \param flip should the image be vertically fliped before saving
	 \param ignoreAlpha should the alpha channel be ignored if it exists
	 \note The output image extension will be automatically added based on the framebuffer type and format.
	 \warning Export of small size GL_FLOAT framebuffers can create artifacts.
	 */
	static void saveFramebuffer(const Framebuffer & framebuffer, unsigned int width, unsigned int height, const std::string & path, bool flip = true, bool ignoreAlpha = false);


	/** Bind a texture to some texture slot.
	 \param texture the infos of the texture to bind
	 \param slot the binding slot
	 */
	static void bindTexture(const Texture * texture, size_t slot);
	
	/** Bind a series of textures to some texture slots, in order.
	 \param textures the infos of the textures to bind
	 \param startingSlot the optional index of the first binding slot
	 */
	static void bindTextures(const std::vector<const Texture *> & textures, size_t startingSlot = 0);

	/** Create a GPU texture with a given layout and allocate it.
	 \param texture the texture to setup on the GPU
	 \param descriptor type and format information
	 */
	static void setupTexture(Texture & texture, const Descriptor & descriptor);

	/** Allocate GPU memory for an existing texture.
	 \param texture the texture to allocate memory for
	 */
	static void allocateTexture(const Texture & texture);

	/** Upload a texture images data to the GPU.
	 \param texture the texture to upload
	 */
	static void uploadTexture(const Texture & texture);

	/** Download a texture images data from the GPU.
	 \param texture the texture to download
	 \warning The CPU images of the texture will be overwritten.
	 */
	static void downloadTexture(Texture & texture);

	/** Generate a texture mipmaps on the GPU.
	 \param texture the texture to use
	 \note This will set the number of levels to 1000.
	 */
	static void generateMipMaps(const Texture & texture);

	/** Convert a texture shape to an OpenGL texture format enum.
	 \param shape the texture shape
	 \return the corresponding target
	 */
	static GLenum targetFromShape(const TextureShape & shape);

	/** Bind a uniform buffer to a shader slot.
	 \param buffer the infos of the buffer to bind
	 \param slot the binding slot
	 \note This will bind the buffer as a uniform buffer.
	 */
	static void bindBuffer(const BufferBase & buffer, size_t slot);

	/** Create and allocate a GPU buffer.
	 \param buffer the buffer to setup on the GPU
	 */
	static void setupBuffer(BufferBase & buffer);

	/** Allocate GPU memory for an existing buffer.
	 \param buffer the buffer to allocate memory for
	 */
	static void allocateBuffer(const BufferBase & buffer);

	/** Upload data to a buffer on the GPU. It's possible to upload a subrange of the buffer data store.
	 \param buffer the buffer to upload to
	 \param size the amount of data to upload, in bytes
	 \param data pointer to the data to upload
	 \param offset optional offset in the buffer store
	 */
	static void uploadBuffer(const BufferBase & buffer, size_t size, unsigned char * data, size_t offset = 0);

	/** Mesh loading: send a mesh data to the GPU and set the input mesh GPU infos accordingly.
	 \param mesh the mesh to upload
	 \note The order of attribute locations is: position, normal, uvs, tangents, binormals.
	 */
	static void setupMesh(Mesh & mesh);

	/** Draw indexed geometry.
	 \param mesh the mesh to draw
	 */
	static void drawMesh(const Mesh & mesh);

	/** Flush the GPU command pipelines and wait for all processing to be done.
	 */
	static void sync();

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
	static std::vector<std::string> deviceExtensions();

	/** Set the current viewport.
	 \param x horizontal coordinate
	 \param y vertical coordinate
	 \param w width
	 \param h height
	 */
	static void setViewport(int x, int y, int w, int h);
	
	/** Clear color for the current framebuffer.
	 \param color the RGBA float clear color
	 */
	static void clearColor(const glm::vec4 & color);
	
	/** Clear depth for the current framebuffer.
	 \param depth the depth clear
	 */
	static void clearDepth(float depth);

	/** Clear color and depth for the current framebuffer.
	 \param color the RGBA float clear color
	 \param depth the depth clear
	 */
	static void clearColorAndDepth(const glm::vec4 & color, float depth);

	/** Blit the content of a framebuffer into another one, resizing the content accordingly.
	 \param src the source framebuffer
	 \param dst the destination framebuffer
	 \param filter the filtering to use for resizing
	 */
	static void blit(const Framebuffer & src, const Framebuffer & dst, Filter filter);
	
	/** Blit the content of a texture into another one, resizing the content accordingly.
	 \param src the source texture
	 \param dst the destination texture
	 \param filter the filtering to use for resizing
	 */
	static void blit(const Texture & src, Texture & dst, Filter filter);

private:
	/** Read back the currently bound framebuffer to the CPU and save it in the best possible format on disk.
	 \param type the type of the framebuffer
	 \param format the format of the framebuffer
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param components the number of components of the framebuffer
	 \param path the output image path
	 \param flip should the image be vertically fliped before saving
	 \param ignoreAlpha should the alpha channel be ignored if it exists
	 \note The output image extension will be automatically added based on the framebuffer type and format.
	 */
	static void savePixels(GLenum type, GLenum format, unsigned int width, unsigned int height, unsigned int components, const std::string & path, bool flip, bool ignoreAlpha);
};

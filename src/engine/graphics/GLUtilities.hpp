#pragma once

#include "resources/Mesh.hpp"
#include "resources/Image.hpp"
#include "resources/Texture.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

/**
 \addtogroup Graphics
 @{
 */

/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected.
#define checkGLError() _checkGLError(__FILE__, __LINE__, "")
/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected, along with additional user informations.
#define checkGLErrorInfos(infos) _checkGLError(__FILE__ , __LINE__, infos)

/** Check if any OpenGL error has been detected and log it.
 \param file the current file
 \param line the current line
 \param infos additional user info
 \return non zero if an error was encountered
 */
int _checkGLError(const char *file, int line, const std::string & infos);

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
	
	/** Mesh loading: send a mesh data to the GPU and set the input mesh GPU infos accordingly.
	 \param mesh the mesh to upload
	 \note The order of attribute locations is: position, normal, uvs, tangents, binormals.
	 */
	static void setupBuffers(Mesh & mesh);
	
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
	static void saveFramebuffer(const Framebuffer & framebuffer, const unsigned int width, const unsigned int height, const std::string & path, const bool flip = true, const bool ignoreAlpha = false);
	
	
	/** Draw indexed geometry.
	 \param mesh the mesh to draw
	 */
	static void drawMesh(const Mesh & mesh);
	
	static void bindTexture(const Texture * texture, unsigned int slot);
	/** Bind a series of textures to some texture slots, in order.
	 \param textures the infos of the textures to bind
	 \param startingSlot the optional index of the first binding slot
	 */
	static void bindTextures(const std::vector<const Texture*> & textures, unsigned int startingSlot = 0);
	
	/** Create a GPU texture with a given layout.
	 \param texture the texture to setup on the GPU
	 \param descriptor type and format information
	 */
	static void setupTexture(Texture & texture, const Descriptor & descriptor);
	
	static void allocateTexture(const Texture & texture);
	
	/** Upload a texture images data to the GPU.
	 \param texture the texture to upload
	 */
	static void uploadTexture(const Texture & texture);
	
	static void downloadTexture(Texture & texture);
	
	/** Generate a texture mipmaps on the GPU.
	 \param texture the texture to use
	 \note This will set the number of levels to 1000.
	 */
	static void generateMipMaps(const Texture & texture);
	
	/** Convert a texture shape to an openGL texture format enum.
	 \param shape the texture shape
	 \return the corresponding target
	 */
	static GLenum targetFromShape(const TextureShape & shape);
	
	static void sync();
	
	static void deviceInfos(std::string & vendor, std::string & renderer, std::string & version, std::string & shaderVersionk);
	
	static std::vector<std::string> deviceExtensions();
	
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
	static void savePixels(const GLenum type, const GLenum format, const unsigned int width, const unsigned int height, const unsigned int components, const std::string & path, const bool flip, const bool ignoreAlpha);
	
};


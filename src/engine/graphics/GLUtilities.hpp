#ifndef GLUtilities_h
#define GLUtilities_h

#include "resources/MeshUtilities.hpp"
#include "Common.hpp"

/**
 \addtogroup Graphics
 @{
 */

/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected.
#define checkGLError() _checkGLError(__FILE__, __LINE__, "")
/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected, along with additional user informations.
#define checkGLErrorInfos(infos) _checkGLError(__FILE__ , __LINE__, infos)

/** Converts a GLenum error number into a human-readable string.
 \param error the OpenGl error value
 \return the corresponding string
 */
std::string getGLErrorString(GLenum error);

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

/** \brief Regroups format, type, filtering and wrapping informations for a color buffer.
  \ingroup Graphics
 */
struct Descriptor {
	
	GLuint typedFormat; ///< The precise typed format.
	GLuint filtering; ///< Filtering mode.
	GLuint wrapping; ///< Wrapping mode.
	
	/** Default constructor. RGB8, linear, clamp. */
	Descriptor();
	
	/** Convenience constructor. Custom typed format, linear, clamp.
	 \param typedFormat_ the precise typed format to use
	 */
	Descriptor(const GLuint typedFormat_);
	
	/** Constructor.
	 \param typedFormat_ the precise typed format to use
	 \param filtering_ the texture filtering (GL_LINEAR,...) to use
	 \param wrapping_ the texture wrapping mode (GL_CLAMP_TO_EDGE) to use
	 */
	Descriptor(const GLuint typedFormat_, const GLuint filtering_, const GLuint wrapping_);
	
};

/**
 \brief Store texture informations.
 \ingroup Graphics
 */
struct TextureInfos {
	Descriptor descriptor; ///< The texture format, type, filtering.
	GLuint id; ///< The OpenGL texture ID.
	unsigned int width; ///< The texture width.
	unsigned int height; ///< The texture height.
	unsigned int mipmap; ///< The number of mipmaps.
	bool cubemap; ///< Denote if the texture is a cubemap.
	
	
	/** Default constructor. */
	TextureInfos() : descriptor(), id(0), width(0), height(0), mipmap(0), cubemap(false) {}

};

/**
 \brief Store geometry informations.
 \ingroup Graphics
 */
struct MeshInfos {
	GLuint vId; ///< The vertex array OpenGL ID.
	GLuint eId; ///< The element buffer OpenGL ID.
	GLsizei count; ///< The number of vertices.
	BoundingBox bbox; ///< The mesh bounding box in model space.
	
	/** Default constructor. */
	MeshInfos() : vId(0), eId(0), count(0), bbox() {}

};


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
	
	// Texture loading.
	/** Send a 2D texture to the GPU.
	 \param path a list of paths, one for each mipmap level of the texture
	 \param descriptor
	 \return the texture informations, including the OpenGL ID
	 \note If only one path is present, the mipmaps will be generated automatically.
	 */
	static TextureInfos loadTexture(const std::vector<std::string>& path, const Descriptor & descriptor);
	 
	/** Send a cubemap texture to the GPU.
	 \param paths a list of lists of paths, six (one per face) for each mipmap level of the texture
	 \param descriptor
	 \return the texture informations, including the OpenGL ID
	 \note If only one list of paths is present, the mipmaps will be generated automatically.
	 */
	static TextureInfos loadTextureCubemap(const std::vector<std::vector<std::string>> & paths, const Descriptor & descriptor);
	
	/** Mesh loading: send a mesh data to the GPU.
	 \param mesh the mesh to upload
	 \return the mesh infos, including OpenGL array/buffer IDs
	 \note The order of attribute locations is: position, normal, uvs, tangents, binormals.
	 */
	static MeshInfos setupBuffers(const Mesh & mesh);
	
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
	static void saveFramebuffer(const std::shared_ptr<Framebuffer> & framebuffer, const unsigned int width, const unsigned int height, const std::string & path, const bool flip = true, const bool ignoreAlpha = false);
	
	/** Save the window framebuffer content to the disk.
	 \param width the width of the region to save
	 \param height the height of the region to save
	 \param path the output image path
	 \note The output image extension will be automatically added based on the framebuffer type and format.
	 */
	static void saveDefaultFramebuffer(const unsigned int width, const unsigned int height, const std::string & path);
	
	/** Obtain the separate type, format and channel count of a texture typed format.
	 \param typedFormat the precise format to detail
	 \param type will contain the type (GL_FLOAT,...)
	 \param format will contain the general layout (GL_RG,...)
	 \return the number of channels
	 */
	static unsigned int getTypeAndFormat(const GLuint typedFormat, GLuint & type, GLuint & format);
	
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
	
	/** Create a GPU texture with a given layout and mip map count.
	 \param destination the kind of texture to create: 2D, cubemap,...
	 \param descriptor type and format information
	 \param mipmapCount the number of mipmap levels.
	 \return the handle of the created texture.
	 */
	static GLuint createTexture(const GLenum destination, const Descriptor & descriptor, const int mipmapCount);
	
	/** Upload data to a GPU texture.
	 \param destination the kind of texture to target: 2D, cubemap...
	 \param texId the handle of the texture
	 \param sourceType the OpenGL type of the input data
	 \param destTypedFormat the detailed format of the texture
	 \param mipid the mipmap level to populate
	 \param mipWidth the width of the targeted mipmap level
	 \param mipHeight the height of the targeted mipmap level
	 \param sourceChannels the number of input channels
	 \param data the raw data to upload to the GPU
	 */
	static void uploadTexture(const GLenum destination, const GLuint texId, const GLenum sourceType, const GLenum destTypedFormat, const unsigned int mipid, const unsigned int mipWidth, const unsigned int mipHeight, const unsigned int sourceChannels, void * data);
};


#endif

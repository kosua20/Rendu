#ifndef GLUtilities_h
#define GLUtilities_h
#include <map>

#include "../Common.hpp"
#include "../resources/MeshUtilities.hpp"
#include "Framebuffer.hpp"

/**
 \addtogroup Helpers
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


/**
 \brief Store texture informations.
 \ingroup Helpers
 */
struct TextureInfos {
	GLuint id; ///< The OpenGL texture ID.
	unsigned int width; ///< The texture width.
	unsigned int height; ///< The texture height.
	unsigned int mipmap; ///< The number of mipmaps.
	bool cubemap; ///< Denote if the texture is a cubemap.
	bool hdr; ///< Denote if the texture is HDR (float values).
	
	/** Default constructor. */
	TextureInfos() : id(0), width(0), height(0), mipmap(0), cubemap(false), hdr(false) {}

};

/**
 \brief Store geometry informations.
 \ingroup Helpers
 */
struct MeshInfos {
	GLuint vId; ///< The vertex array OpenGL ID.
	GLuint eId; ///< The element buffer OpenGL ID.
	GLsizei count; ///< The number of vertices.
	BoundingBox bbox; ///< The mesh bounding box in model space.
	
	/** Default constructor. */
	MeshInfos() : vId(0), eId(0), count(0), bbox() {}

};


/**
 \brief Provide utility functions to communicate with the driver and GPU.
 \ingroup Helpers
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
	 \param sRGB denotes if gamma conversion should be applied to the texture when used
	 \return the texture informations, including the OpenGL ID
	 \note If only one path is present, the mipmaps will be generated automatically.
	 */
	static TextureInfos loadTexture(const std::vector<std::string>& path, bool sRGB);
	 
	/** Send a cubemap texture to the GPU.
	 \param paths a list of lists of paths, six (one per face) for each mipmap level of the texture
	 \param sRGB denotes if gamma conversion should be applied to the texture when used
	 \return the texture informations, including the OpenGL ID
	 \note If only one list of paths is present, the mipmaps will be generated automatically.
	 */
	static TextureInfos loadTextureCubemap(const std::vector<std::vector<std::string>> & paths, bool sRGB);
	
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
	 \param flip should the image be vertically fliped before saving.
	 \param ignoreAlpha should the alpha channel be ignored if it exists.
	 \note The output image extension will be automatically added based on the framebuffer type and format.
	 */
	static void saveFramebuffer(const std::shared_ptr<Framebuffer> & framebuffer, const unsigned int width, const unsigned int height, const std::string & path, const bool flip = true, const bool ignoreAlpha = false);
	
	/** Save the window framebuffer content to the disk.
	 \param width the width of the region to save
	 \param height the height of the region to save
	 \param path the output image path
	 \note The output image extension will be automatically added based on the framebuffer type and format.
	 */
	static void saveDefaultFramebuffer(const unsigned int width, const unsigned int height, const std::string & path);
	
	static void getTypeAndFormat(const GLuint typedFormat, GLuint & type, GLuint & format);
	
private:
	
	/** Read back the currently bound framebuffer to the CPU and save it in the best possible format on disk.
	 \param type the type of the framebuffer
	 \param format the format of the framebuffer
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param components the number of components of the framebuffer
	 \param path the output image path
	 \param flip should the image be vertically fliped before saving.
	 \param ignoreAlpha should the alpha channel be ignored if it exists.
	 \note The output image extension will be automatically added based on the framebuffer type and format.
	 */
	static void savePixels(const GLenum type, const GLenum format, const unsigned int width, const unsigned int height, const unsigned int components, const std::string & path, const bool flip, const bool ignoreAlpha);
	
};


#endif

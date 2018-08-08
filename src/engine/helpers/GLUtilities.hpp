#ifndef GLUtilities_h
#define GLUtilities_h
#include "../Common.hpp"
#include "../resources/MeshUtilities.hpp"
#include "../Framebuffer.hpp"

/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected.

#define checkGLError() _checkGLError(__FILE__, __LINE__, "")
#define checkGLErrorInfos(infos) _checkGLError(__FILE__ , __LINE__, infos)

/// Converts a GLenum error number into a human-readable string.
std::string getGLErrorString(GLenum error);

/// Check if any OpenGL error has been detected and log it.
int _checkGLError(const char *file, int line, const std::string & infos);

/// Check if any OpenGL has been detected after setting up a framebuffer.
int checkGLFramebufferError();

struct TextureInfos {
	GLuint id;
	unsigned int width;
	unsigned int height;
	unsigned int mipmap;
	bool cubemap;
	bool hdr;
	TextureInfos() : id(0), width(0), height(0), mipmap(0), cubemap(false), hdr(false) {}

};

struct MeshInfos {
	GLuint vId;
	GLuint eId;
	GLsizei count;
	BoundingBox bbox;
	
	MeshInfos() : vId(0), eId(0), count(0), bbox() {}

};


class GLUtilities {
	
private:
	/// Load a shader of the given type from a string
	static GLuint loadShader(const std::string & prog, GLuint type);
	
	static void savePixels(const GLenum type, const GLenum format, const unsigned int width, const unsigned int height, const unsigned int components, const std::string & path, const bool flip, const bool ignoreAlpha);
	
public:
	
	// Program setup.
	/// Create a GLProgram using the shader code contained in the given strings.
	static GLuint createProgram(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent);
	
	// Texture loading.
	/// 2D texture.
	static TextureInfos loadTexture(const std::vector<std::string>& path, bool sRGB);
	
	/// Cubemap texture.
	static TextureInfos loadTextureCubemap(const std::vector<std::vector<std::string>> & paths, bool sRGB);
	
	// Mesh loading.
	static MeshInfos setupBuffers(const Mesh & mesh);
	
	// Framebuffer saving to disk.
	static void saveFramebuffer(const std::shared_ptr<Framebuffer> & framebuffer, const unsigned int width, const unsigned int height, const std::string & path, const bool flip = true, const bool ignoreAlpha = false);
	
	static void saveDefaultFramebuffer(const unsigned int width, const unsigned int height, const std::string & path);
	
};


#endif

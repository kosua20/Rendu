#ifndef ProgramUtilities_h
#define ProgramUtilities_h

#include <gl3w/gl3w.h>
#include <string>
#include <vector>

/// This macro is used to check for OpenGL errors with access to the file and line number where the error is detected.
#define checkGLError() _checkGLError(__FILE__, __LINE__)

/// Converts a GLenum error number into a human-readable string.
std::string getGLErrorString(GLenum error);

/// Check if any OpenGL error has been detected and log it.
int _checkGLError(const char *file, int line);

struct TextureInfos {
	GLuint id;
	int width;
	int height;
	bool cubemap;
};

class ProgramUtilities {
public:
	
	/// Return the content of a text file at the given path, as a string.
	static std::string loadStringFromFile(const std::string & path);
	
	/// Load a shader of the given type from a string
	static GLuint loadShader(const std::string & prog, GLuint type);
	
	/// Create a GLProgram using the hader code contained in the given files.
	static GLuint createGLProgram(const std::string & vertexPath, const std::string & fragmentPath, const std::string & geometryPath = "");
	
	// Texture loading.
	
	// 2D texture.
	static TextureInfos loadTexture(const std::string& path, bool sRGB);
	
	// Cubemap texture.
	static TextureInfos loadTextureCubemap(const std::vector<std::string> & paths, bool sRGB);
};


#endif

#ifndef ProgramUtilities_h
#define ProgramUtilities_h

#include <GL/glew.h>
#include <string>

/// Return the content of a text file at the given path, as a string.
std::string loadStringFromFile(const std::string & path);

/// Load a shader of the given type from a string
GLuint loadShader(const std::string & prog, GLuint type);

/// create a GLProgram using the hader code contained in the given files.
GLuint createGLProgram(const std::string & vertexPath, const std::string & fragmentPath, const std::string & geometryPath = "");

#endif

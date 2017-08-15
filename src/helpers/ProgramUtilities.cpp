#include "ProgramUtilities.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

std::string getGLErrorString(GLenum error) {
	std::string msg;
	switch (error) {
		case GL_INVALID_ENUM:
			msg = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			msg = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			msg = "GL_INVALID_OPERATION";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			msg = "GL_INVALID_FRAMEBUFFER_OPERATION";
			break;
		case GL_NO_ERROR:
			msg = "GL_NO_ERROR";
			break;
		case GL_OUT_OF_MEMORY:
			msg = "GL_OUT_OF_MEMORY";
			break;
		default:
			msg = "UNKNOWN_GL_ERROR";
	}
	return msg;
}

int _checkGLError(const char *file, int line){
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR){
		std::cerr << "glError in " << file << " (" << line << ") : " << getGLErrorString(glErr) << std::endl;
		return 1;
	}
	return 0;
}

std::string ProgramUtilities::loadStringFromFile(const std::string & filename) {
	std::ifstream in;
	// Open a stream to the file.
	in.open(filename.c_str());
	if (!in) {
		std::cerr << filename + " is not a valid file." << std::endl;
		return "";
	}
	std::stringstream buffer;
	// Read the stream in a buffer.
	buffer << in.rdbuf();
	// Create a string based on the content of the buffer.
	std::string line = buffer.str();
	in.close();
	return line;
}

GLuint ProgramUtilities::loadShader(const std::string & prog, GLuint type){
	GLuint id;
	// Create shader object.
	id = glCreateShader(type);
	checkGLError();
	// Setup string as source.
	const char * shaderProg = prog.c_str();
	glShaderSource(id,1,&shaderProg,(const GLint*)NULL);
	// Compile the shader on the GPU.
	glCompileShader(id);
	checkGLError();

	GLint success;
	glGetShaderiv(id,GL_COMPILE_STATUS, &success);
	
	// If compilation failed, get information and display it.
	if (success != GL_TRUE) {
		GLint infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<char> infoLog(std::max(infoLogLength, int(1)));
		glGetShaderInfoLog(id, infoLogLength, NULL, &infoLog[0]);

		std::cerr << std::endl 
					<< "*--- " 
					<< (type == GL_VERTEX_SHADER ? "Vertex" : (type == GL_FRAGMENT_SHADER ? "Fragment" : "Geometry (or tess.)")) 
					<< " shader failed to compile ---*" 
					<< std::endl
					<< &infoLog[0]
					<< "*---------------------------------*" 
					<< std::endl << std::endl;
	}
	// Return the id to the successfuly compiled  shader program.
	return id;
}

GLuint ProgramUtilities::createGLProgram(const std::string & vertexPath, const std::string & fragmentPath, const std::string & geometryPath){
	GLuint vp(0), fp(0), gp(0), id(0);
	id = glCreateProgram();
	checkGLError();
	std::string vertexCode = loadStringFromFile(vertexPath);
	std::string fragmentCode = loadStringFromFile(fragmentPath);

	// If vertex program code is given, compile it.
	if (!vertexCode.empty()) {
		vp = loadShader(vertexCode,GL_VERTEX_SHADER);
		glAttachShader(id,vp);
	}
	// If fragment program code is given, compile it.
	if (!fragmentCode.empty()) {
		fp = loadShader(fragmentCode,GL_FRAGMENT_SHADER);
		glAttachShader(id,fp);
	}
	// If geometry program filepath exists, load it and compile it.
	if(!geometryPath.empty()) {
		std::string geometryCode = loadStringFromFile(geometryPath);
		if (!geometryCode.empty()) {
			gp = loadShader(geometryCode,GL_GEOMETRY_SHADER);
			glAttachShader(id,gp);
		}
	}

	// Link everything
	glLinkProgram(id);
	checkGLError();
	//Check linking status.
	GLint success = GL_FALSE;
	glGetProgramiv(id, GL_LINK_STATUS, &success);

	// If linking failed, query info and display it.
	if(!success) {
		GLint infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<char> infoLog(std::max(infoLogLength, int(1)));
		glGetProgramInfoLog(id, infoLogLength, NULL, &infoLog[0]);

		std::cerr << "Failed loading program: " << &infoLog[0] << std::endl;
		return 0;
	}
	// We can now clean the shaders objects, by first detaching them
	if (vp != 0) {
		glDetachShader(id,vp);
	}
	if (fp != 0) {
		glDetachShader(id,fp);
	}
	if (gp != 0) {
		glDetachShader(id,gp);
	}
	checkGLError();
	//And deleting them
	glDeleteShader(vp);
	glDeleteShader(fp);
	glDeleteShader(gp);

	glUseProgram(id);
	checkGLError();
	// Return the id to the succesfuly linked GLProgram.
	return id;
}

void ProgramUtilities::flipImage(unsigned char * image, const int width, const int height, const int components){
	// The width in bytes.
	int widthInBytes = width * components;
	int halfHeight = height/2;

	// For each line of the first half, we swap it with the mirroring line, starting from the end of the image.
	for(int h=0; h < halfHeight; ++h){
		std::swap_ranges(image + h * widthInBytes, image + (h+1) * widthInBytes , image + (height - h - 1) * widthInBytes);
	}
}


GLuint ProgramUtilities::loadTexture(const std::string& path, int & width, int & height, bool sRGB){
	
	// Load and upload the texture.
	int components = 4;
	// We need to flip the texture.
	stbi_set_flip_vertically_on_load(true);
	unsigned char *image = stbi_load(path.c_str(), &width, &height, NULL, components);
	if(image == NULL){
		std::cerr << "Unable to load the texture at path " << path << "." << std::endl;
		return 0;
	}
	
	//flipImage(image, imwidth, imheight, components);
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA, width , height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(image[0]));
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	free(image);
	return textureId;
}




GLuint ProgramUtilities::loadTextureCubemap(const std::vector<std::string> & paths, int & width, int & height, bool sRGB){
	
	// If not enough images, return empty texture.
	if(paths.size() != 6){
		return 0;
	}
	
	// Create and bind texture.
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
	
	// Texture settings.
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	std::vector<unsigned char> image;
	int components = 4;
	// For each side, load the image and upload it in the right slot.
	// We don't need to flip them.
	stbi_set_flip_vertically_on_load(false);
	
	for(size_t side = 0; side < 6; ++side){
		unsigned char *image = stbi_load(paths[side].c_str(), &width, &height, NULL, components);
		if(image == NULL){
			std::cerr << "Unable to load the texture at path " << paths[side] << "." << std::endl;
			return 0;
		}
		glTexImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side), 0, sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(image[0]));
		free(image);
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	
	return textureId;
}












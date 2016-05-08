#include "ProgramUtilities.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

std::string loadStringFromFile(const std::string & filename) {
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

GLuint loadShader(const std::string & prog, GLuint type){
	GLuint id;
	// Create shader object.
	id = glCreateShader(type);

	// Setup string as source.
	const char * shaderProg = prog.c_str();
	glShaderSource(id,1,&shaderProg,(const GLint*)NULL);
	// Compile the shader on the GPU.
	glCompileShader(id);
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

GLuint createGLProgram(const std::string & vertexPath, const std::string & fragmentPath, const std::string & geometryPath){
	GLuint vp, fp, gp, id;
	id = glCreateProgram();
	
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

	glUseProgram(id);
	// Return the id to the succesfuly linked GLProgram.
	return id;
}

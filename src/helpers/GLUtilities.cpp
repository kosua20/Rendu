#include "GLUtilities.hpp"
#include "../resources/ImageUtilities.hpp"
#include "Logger.hpp"
#include <vector>
#include <algorithm>


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

int _checkGLError(const char *file, int line, const std::string & infos){
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR){
		std::string filePath(file);
		size_t pos = (std::min)(filePath.find_last_of("/"), filePath.find_last_of("\\"));
		if(pos == std::string::npos){
			pos = 0;
		}
		Log::Error() << Log::OpenGL << "Error " << getGLErrorString(glErr) << " in " << filePath.substr(pos+1) << " (" << line << ").";
		if(infos.size() > 0){
			Log::Error() << " Infos: " << infos;
		}
		Log::Error() << std::endl;
		return 1;
	}
	return 0;
}


GLuint GLUtilities::loadShader(const std::string & prog, GLuint type){
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
		std::vector<char> infoLog((std::max)(infoLogLength, int(1)));
		glGetShaderInfoLog(id, infoLogLength, NULL, &infoLog[0]);

		Log::Error() << std::endl 
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

GLuint GLUtilities::createProgram(const std::string & vertexContent, const std::string & fragmentContent){
	GLuint vp(0), fp(0), id(0);
	id = glCreateProgram();
	checkGLError();

	// If vertex program code is given, compile it.
	if (!vertexContent.empty()) {
		vp = loadShader(vertexContent,GL_VERTEX_SHADER);
		glAttachShader(id,vp);
	}
	// If fragment program code is given, compile it.
	if (!fragmentContent.empty()) {
		fp = loadShader(fragmentContent,GL_FRAGMENT_SHADER);
		glAttachShader(id,fp);
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
		std::vector<char> infoLog((std::max)(infoLogLength, int(1)));
		glGetProgramInfoLog(id, infoLogLength, NULL, &infoLog[0]);

		Log::Error() << Log::OpenGL << "Failed loading program: " << &infoLog[0] << std::endl;
		return 0;
	}
	// We can now clean the shaders objects, by first detaching them
	if (vp != 0) {
		glDetachShader(id,vp);
	}
	if (fp != 0) {
		glDetachShader(id,fp);
	}
	checkGLError();
	//And deleting them
	glDeleteShader(vp);
	glDeleteShader(fp);

	checkGLError();
	// Return the id to the succesfuly linked GLProgram.
	return id;
}



TextureInfos GLUtilities::loadTexture(const std::vector<std::string>& paths, bool sRGB){
	TextureInfos infos;
	infos.cubemap = false;
	if(paths.empty()){
		return infos;
	}
	
	// Create 2D texture.
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	
	// Set proper max mipmap level.
	if(paths.size()>1){
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (int)(paths.size())-1);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1000);
	}
	// Texture settings.
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	
	// Image infos.
	infos.hdr = ImageUtilities::isHDR(paths[0]);
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int channels = 4;
	
	// For now, we assume HDR images to be 3-channels, LDR images to be 4.
	const GLenum format = infos.hdr ? GL_RGB : GL_RGBA;
	const GLenum type = infos.hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
	const GLenum preciseFormat = (infos.hdr ? GL_RGB32F : (sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA));
	
	for(unsigned int mipid = 0; mipid < paths.size(); ++mipid){
		
		const std::string path = paths[mipid];
		
		void* image;
		int ret = ImageUtilities::loadImage(path, width, height, channels, &image, !infos.hdr);
		
		if (ret != 0) {
			Log::Error() << Log::Resources << "Unable to load the texture at path " << path << "." << std::endl;
			free(image);
			return infos;
		}
		
		glTexImage2D(GL_TEXTURE_2D, mipid, preciseFormat, width, height, 0, format, type, image);
		free(image);
		
	}
	
	// If only level 0 was given, generate mipmaps pyramid automatically.
	if(paths.size() == 1){
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	infos.id = textureId;
	infos.width = width;
	infos.height = height;
	return infos;
}

TextureInfos GLUtilities::loadTextureCubemap(const std::vector<std::vector<std::string>> & allPaths, bool sRGB){
	TextureInfos infos;
	infos.cubemap = true;
	if(allPaths.empty() ){
		Log::Error() << Log::Resources << "Unable to find cubemap." << std::endl;
		return infos;
	}
	
	// If not enough images, return empty texture.
	if(allPaths.front().size() == 0 ){
		Log::Error() << Log::Resources << "Unable to find cubemap." << std::endl;
		return infos;
	}
	
	// Create and bind texture.
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
	
	// Set proper max mipmap level.
	if(allPaths.size()>1){
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, (int)(allPaths.size())-1);
	} else {
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 1000);
	}
	// Texture settings.
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	// Image infos.
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int channels = 4;
	infos.hdr = ImageUtilities::isHDR(allPaths[0][0]);
	
	// For now, we assume HDR images to be 3-channels, LDR images to be 4.
	const GLenum format = infos.hdr ? GL_RGB : GL_RGBA;
	const GLenum type = infos.hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
	const GLenum preciseFormat = (infos.hdr ? GL_RGB32F : (sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA));
	
	for(unsigned int mipid = 0; mipid < allPaths.size(); ++mipid){
		
		const std::vector<std::string> paths = allPaths[mipid];
		// For each side, load the image and upload it in the right slot.
		// We don't need to flip them.
		
		for(size_t side = 0; side < 6; ++side){
			void* image;
			int ret = ImageUtilities::loadImage(paths[side], width, height, channels, &image, false);
			if (ret != 0) {
				Log::Error() << Log::Resources << "Unable to load the texture at path " << paths[side] << "." << std::endl;
				free(image);
				return infos;
			}
			
			glTexImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side), mipid, preciseFormat, width, height, 0, format, type, image);
			free(image);
		}
		
	}
	
	// If only level 0 was given, generate mipmaps pyramid automatically.
	if(allPaths.size() == 1){
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
	
	infos.id = textureId;
	infos.width = width;
	infos.height = height;
	return infos;
}


MeshInfos GLUtilities::setupBuffers(const Mesh & mesh){
	MeshInfos infos;
	GLuint vbo = 0;
	GLuint vbo_nor = 0;
	GLuint vbo_uv = 0;
	GLuint vbo_tan = 0;
	GLuint vbo_binor = 0;
	
	// Create an array buffer to host the geometry data.
	if(mesh.positions.size() > 0){
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.positions.size() * 3, &(mesh.positions[0]), GL_STATIC_DRAW);
	}
	
	if(mesh.normals.size() > 0){
		glGenBuffers(1, &vbo_nor);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.normals.size() * 3, &(mesh.normals[0]), GL_STATIC_DRAW);
	}
	
	if(mesh.texcoords.size() > 0){
		glGenBuffers(1, &vbo_uv);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.texcoords.size() * 2, &(mesh.texcoords[0]), GL_STATIC_DRAW);
	}
	
	if(mesh.tangents.size() > 0){
		glGenBuffers(1, &vbo_tan);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_tan);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.tangents.size() * 3, &(mesh.tangents[0]), GL_STATIC_DRAW);
	}
	
	if(mesh.binormals.size() > 0){
		glGenBuffers(1, &vbo_binor);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_binor);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.binormals.size() * 3, &(mesh.binormals[0]), GL_STATIC_DRAW);
	}
	
	// Generate a vertex array.
	GLuint vao = 0;
	glGenVertexArrays (1, &vao);
	glBindVertexArray(vao);
	
	// Setup attributes.
	int currentAttribute = 0;
	if(vbo > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(currentAttribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	if(vbo_nor > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
		glVertexAttribPointer(currentAttribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	if(vbo_uv > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
		glVertexAttribPointer(currentAttribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	if(vbo_tan > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_tan);
		glVertexAttribPointer(currentAttribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	if(vbo_binor > 0){
		glEnableVertexAttribArray(currentAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_binor);
		glVertexAttribPointer(currentAttribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		++currentAttribute;
	}
	
	// We load the indices data
	GLuint ebo = 0;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.indices.size(), &(mesh.indices[0]), GL_STATIC_DRAW);
	
	glBindVertexArray(0);
	
	infos.vId = vao;
	infos.eId = ebo;
	infos.count = (GLsizei)mesh.indices.size();
	return infos;
}

void GLUtilities::saveDefaultFramebuffer(const unsigned int width, const unsigned int height, const std::string & path){
	
	GLint currentBoundFB = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentBoundFB);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GLUtilities::savePixels(GL_UNSIGNED_BYTE, GL_RGBA, width, height, 4, path, true, true);
	
	glBindFramebuffer(GL_FRAMEBUFFER, currentBoundFB);
}

void GLUtilities::saveFramebuffer(const std::shared_ptr<Framebuffer> & framebuffer, const unsigned int width, const unsigned int height, const std::string & path, const bool flip, const bool ignoreAlpha){
	
	GLint currentBoundFB = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentBoundFB);
	
	framebuffer->bind();
	const GLenum type = framebuffer->type();
	const GLenum format = framebuffer->format();
	const unsigned int components = (format == GL_RED ? 1 : (format == GL_RG ? 2 : (format == GL_RGB ? 3 : 4)));

	GLUtilities::savePixels(type, format, width, height, components, path, flip, ignoreAlpha);
	
	glBindFramebuffer(GL_FRAMEBUFFER, currentBoundFB);
}

void GLUtilities::savePixels(const GLenum type, const GLenum format, const unsigned int width, const unsigned int height, const unsigned int components, const std::string & path, const bool flip, const bool ignoreAlpha){
	
	glFlush();
	glFinish();
	
	const bool hdr = type == GL_FLOAT;
	
	Log::Info() << Log::OpenGL << "Saving framebuffer to file " << path << (hdr ? ".exr" : ".png") << "... " << std::flush;
	int ret = 0;
	if(hdr){
		// Get back values.
		GLfloat * data = new GLfloat[width * height * components];
		glReadPixels(0, 0, width, height, format, type, &data[0]);
		// Save data. We don't flip HDR images because we don't flip them when loading...
		ret = ImageUtilities::saveHDRImage(path + ".exr", width, height, components, (float*)data, flip, ignoreAlpha);
		delete[] data;
	} else {
		// Get back values.
		GLubyte * data = new GLubyte[width * height * components];
		glReadPixels(0, 0, width, height, format, type, &data[0]);
		// Save data.
		ret = ImageUtilities::saveLDRImage(path + ".png", width, height, components, (unsigned char*)data, flip, ignoreAlpha);
		delete[] data;
	}
	
	if(ret != 0){
		Log::Error() << "Error." << std::endl;
	} else {
		Log::Info() << "Done." << std::endl;
	}
	
}







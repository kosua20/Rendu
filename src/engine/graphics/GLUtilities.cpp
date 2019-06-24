#include "GLUtilities.hpp"
#include "Framebuffer.hpp"
#include "resources/ImageUtilities.hpp"
#include "resources/TextUtilities.hpp"

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

int checkGLFramebufferError(){
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE){
		switch(status){
		case GL_FRAMEBUFFER_UNDEFINED:
			Log::Error() << Log::OpenGL << "Error GL_FRAMEBUFFER_UNDEFINED" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			Log::Error() << Log::OpenGL << "Error GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			Log::Error() << Log::OpenGL << "Error GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			Log::Error() << Log::OpenGL << "Error GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			Log::Error() << Log::OpenGL << "Error GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			Log::Error() << Log::OpenGL << "Error GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			Log::Error() << Log::OpenGL << "Error GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			Log::Error() << Log::OpenGL << "Error GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" << std::endl;
			break;
		default:
			Log::Error() << Log::OpenGL << "Unknown framebuffer error." << std::endl;
			break;
		}
		return 1;
	}
	return 0;
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

Descriptor::Descriptor(){
	typedFormat = GL_RGB8; filtering = GL_LINEAR_MIPMAP_LINEAR; wrapping = GL_CLAMP_TO_EDGE;
}

Descriptor::Descriptor(const GLuint typedFormat_, const GLuint filtering_, const GLuint wrapping_){
	typedFormat = typedFormat_; filtering = filtering_; wrapping = wrapping_;
}

MeshInfos::MeshInfos() : vId(0), eId(0), count(0), bbox() {
	vbos[0] = vbos[1] = vbos[2] = vbos[3] = vbos[4] = 0;
}

void MeshInfos::clean(){
	glDeleteBuffers(1, &eId);
	glDeleteVertexArrays(1, &vId);
	glDeleteBuffers(5, &vbos[0]);
	count = 0;
	eId = vId = vbos[0] = vbos[1] = vbos[2] = vbos[3] = vbos[4] = 0;
	bbox = BoundingBox();
}

void replace(std::string & source, const std::string& fromString, const std::string & toString){
	std::string::size_type nextPos = 0;
	const size_t fromSize = fromString.size();
	const size_t toSize = toString.size();
	while((nextPos = source.find(fromString, nextPos)) != std::string::npos){
		source.replace(nextPos, fromSize, toString);
		nextPos += toSize;
	}
}

GLuint GLUtilities::loadShader(const std::string & prog, GLuint type, std::map<std::string, int> & bindings, std::string & finalLog){
	// We need to detect texture slots and store them, to avoid having to register them in
	// the rest of the code (object, renderer), while not having support for 'layout(binding=n)' in OpenGL <4.2.
	std::stringstream inputLines(prog);
	std::vector<std::string> outputLines;
	std::string line;
	bool isInMultiLineComment = false;
	while(std::getline(inputLines, line)){
		
		// Comment handling.
		const std::string::size_type commentPosBegin = line.find("/*");
		const std::string::size_type commentPosEnd = line.rfind("*/");
		const std::string::size_type commentMonoPos = line.find("//");
		// We suppose no multi-line comment nesting, that way we can tackle them linearly.
		if(commentPosBegin != std::string::npos && commentPosEnd != std::string::npos){
			// Both token exist.
			// Either this is "end begin", in which case we are still in a comment.
			// Or this is "begin end", ie a single ligne comment.
			isInMultiLineComment = commentPosBegin > commentPosEnd;
		} else if(commentPosEnd != std::string::npos){
			// Only an end token.
			isInMultiLineComment = false;
		} else if(commentPosBegin != std::string::npos){
			// Only a begin token.
			isInMultiLineComment = true;
		}
		
		// Find a line containing "layout...binding...uniform...sampler..."
		const std::string::size_type layoutPos = line.find("layout");
		const std::string::size_type bindingPos = line.find("binding");
		const std::string::size_type uniformPos = line.find("uniform");
		const std::string::size_type samplerPos = line.find("sampler");
		
		const bool isNotALayoutBindingUniformSampler = (layoutPos == std::string::npos || bindingPos == std::string::npos || uniformPos == std::string::npos || samplerPos == std::string::npos);
		const bool isALayoutInsideAMultiLineComment = isInMultiLineComment && (layoutPos > commentPosBegin || samplerPos < commentPosEnd);
		const bool isALayoutInsideASingleLineComment = commentMonoPos != std::string::npos && layoutPos > commentMonoPos;
		
		// Detect sampler with no bindings.
		const bool isAUniformSamplerWithNoBinding = samplerPos != std::string::npos && uniformPos != std::string::npos && bindingPos == std::string::npos;
		if(isAUniformSamplerWithNoBinding){
			const std::string::size_type endPosName  = line.find_first_of(";")-1;
			const std::string::size_type startPosName  = line.find_last_of(" ", endPosName)+1;
			const std::string name = line.substr(startPosName, endPosName - startPosName + 1);
			Log::Warning() << Log::OpenGL << "Missing binding info for sampler \"" << name << "\"." << std::endl;
			outputLines.push_back(line);
			continue;
		}
		
		if(isNotALayoutBindingUniformSampler || isALayoutInsideAMultiLineComment || isALayoutInsideASingleLineComment) {
			// We don't modify the line.
			outputLines.push_back(line);
			continue;
		}
		
		// Layout on basic uniforms is not really used < 4.2, so we can be quite aggressive in our extraction.
		const std::string::size_type firstSlotPos = line.find_first_of("0123456789", bindingPos);
		const std::string::size_type lastSlotPos = line.find_first_not_of("0123456789", firstSlotPos)-1;
		const int slot = std::stoi(line.substr(firstSlotPos, lastSlotPos - firstSlotPos + 1));
			
		const std::string::size_type endPosName  = line.find_first_of(";", lastSlotPos)-1;
		const std::string::size_type startPosName  = line.find_last_of(" ", endPosName)+1;
		const std::string name = line.substr(startPosName, endPosName - startPosName + 1);
		
		const std::string::size_type endSamplerPos = line.find_first_of(" ", samplerPos) - 1;
		const std::string::size_type startSamplerPos = line.find_last_of(" ", samplerPos) + 1;
		const std::string samplerType = line.substr(startSamplerPos, endSamplerPos - startSamplerPos + 1);
		const std::string outputLine = "uniform " + samplerType + " " + name + ";";
		outputLines.push_back(outputLine);
		
		if(bindings.count(name) > 0 && bindings[name] != slot){
			Log::Warning() << Log::OpenGL << "Inconsistent sampler location between linked shaders for \"" << name << "\"." << std::endl;
		}
		bindings[name] = slot;
		Log::Verbose() << Log::OpenGL << "Detected texture (" << name << ", " << slot << ") => " << outputLine << std::endl;
	}
	std::string outputProg;
	for(const auto & outputLine : outputLines){
		outputProg.append(outputLine + "\n");
	}
	
	// Create shader object.
	GLuint id = glCreateShader(type);
	checkGLError();
	// Setup string as source.
	const char * shaderProg = outputProg.c_str();
	glShaderSource(id,1,&shaderProg,(const GLint*)NULL);
	// Compile the shader on the GPU.
	glCompileShader(id);
	checkGLError();

	GLint success;
	glGetShaderiv(id,GL_COMPILE_STATUS, &success);
	finalLog = "";
	// If compilation failed, get information and display it.
	if (success != GL_TRUE) {
		// Get the log string length for allocation.
		GLint infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		// Get the log string.
		std::vector<char> infoLog((size_t)(std::max)(infoLogLength, int(1)));
		glGetShaderInfoLog(id, infoLogLength, NULL, &infoLog[0]);
		// Indent and clean.
		std::string infoLogString(infoLog.data(), infoLogLength);
		
		TextUtilities::replace(infoLogString, "\n", "\n\t");
		infoLogString.insert(0, "\t");
		finalLog = infoLogString;
	}
	// Return the id to the successfuly compiled shader program.
	return id;
}

GLuint GLUtilities::createProgram(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, std::map<std::string, int> & bindings, const std::string & debugInfos){
	GLuint vp(0), fp(0), gp(0), id(0);
	id = glCreateProgram();
	checkGLError();
	
	Log::Verbose() << Log::OpenGL << "Compiling " << debugInfos << "." << std::endl;
	
	std::string compilationLog;
	// If vertex program code is given, compile it.
	if (!vertexContent.empty()) {
		vp = loadShader(vertexContent, GL_VERTEX_SHADER, bindings, compilationLog);
		glAttachShader(id,vp);
		if(!compilationLog.empty()){
			Log::Error() << Log::OpenGL << "Vertex shader failed to compile:" << std::endl
			<< compilationLog << std::endl;
		}
	}
	// If fragment program code is given, compile it.
	if (!fragmentContent.empty()) {
		fp = loadShader(fragmentContent, GL_FRAGMENT_SHADER, bindings, compilationLog);
		glAttachShader(id,fp);
		if(!compilationLog.empty()){
			Log::Error() << Log::OpenGL << "Fragment shader failed to compile:" << std::endl
			<< compilationLog << std::endl;
		}
	}
	// If geometry program code is given, compile it.
	if (!geometryContent.empty()) {
		gp = loadShader(geometryContent, GL_GEOMETRY_SHADER, bindings, compilationLog);
		glAttachShader(id,gp);
		if(!compilationLog.empty()){
			Log::Error() << Log::OpenGL << "Geometry shader failed to compile:" << std::endl
			<< compilationLog << std::endl;
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
		// Get the log string length for allocation.
		GLint infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		// Get the log string.
		std::vector<char> infoLog((size_t)(std::max)(infoLogLength, int(1)));
		glGetProgramInfoLog(id, infoLogLength, NULL, &infoLog[0]);
		// Indent and clean.
		std::string infoLogString(infoLog.data(), infoLogLength);
		TextUtilities::replace(infoLogString, "\n", "\n\t");
		infoLogString.insert(0, "\t");
		// Output.
		Log::Error() << Log::OpenGL
			<< "Failed linking program " << debugInfos << ": " << std::endl
			<< infoLogString << std::endl;
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

	checkGLError();
	// Return the id to the successfuly linked GLProgram.
	return id;
}

GLuint GLUtilities::createTexture(const GLenum destination, const Descriptor & descriptor, const int mipmapCount){
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(destination, textureId);
	
	// Set proper max mipmap level.
	if(mipmapCount>=1){
		glTexParameteri(destination, GL_TEXTURE_MAX_LEVEL, mipmapCount-1);
	} else {
		glTexParameteri(destination, GL_TEXTURE_MAX_LEVEL, 1000);
	}
	// Texture settings.
	glTexParameteri(destination, GL_TEXTURE_MIN_FILTER, descriptor.filtering);
	glTexParameteri(destination, GL_TEXTURE_MAG_FILTER, GLUtilities::getMagnificationFilter(descriptor.filtering));
	glTexParameteri(destination, GL_TEXTURE_WRAP_R, descriptor.wrapping);
	glTexParameteri(destination, GL_TEXTURE_WRAP_S, descriptor.wrapping);
	glTexParameteri(destination, GL_TEXTURE_WRAP_T, descriptor.wrapping);
	glBindTexture(destination, 0);
	return textureId;
}

void GLUtilities::uploadTexture(const GLenum destination, const GLuint texId, const GLenum destTypedFormat, const unsigned int mipid, const unsigned int lid, const Image & image){
	
	// Sanity check the texture destination format.
	GLenum destType, destFormat;
	const unsigned int destChannels = getTypeAndFormat(destTypedFormat, destType, destFormat);
	if(destChannels != image.components){
		Log::Error() << Log::OpenGL << "Not enough values in source data for texture upload." << std::endl;
		return;
	}
	
	const size_t destSize = destChannels * image.height * image.width;
	//Perform conversion if needed.
	const GLubyte* finalDataPtr;
	GLubyte * finalData = nullptr;
	if(destType == GL_UNSIGNED_BYTE) {
		// If we want a uchar image, we convert and scale from [0,1] float to [0, 255] uchars.
		finalData = new GLubyte[destSize];
		// Handle the conversion by hand.
		for(size_t pid = 0; pid < destSize; ++pid){
			const float newValue = std::min(255.0f, std::max(0.0f, image.pixels[pid] * 255.0f));
			finalData[pid] = GLubyte(newValue);
		}
		finalDataPtr = &finalData[0];
	} else {
		// Just reinterpret the data.
		finalDataPtr = reinterpret_cast<const GLubyte*>(&image.pixels[0]);
	}
	
	// Upload.
	glBindTexture(destination, texId);
	if(destination == GL_TEXTURE_2D){
		glTexImage2D(destination, (GLint)mipid, destTypedFormat, (GLsizei)image.width, (GLsizei)image.height, 0, destFormat, destType, finalDataPtr);
	} else if(destination == GL_TEXTURE_CUBE_MAP){
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lid, (GLint)mipid, destTypedFormat, (GLsizei)image.width, (GLsizei)image.height, 0, destFormat, destType, finalDataPtr);
	} else if(destination == GL_TEXTURE_2D_ARRAY){
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, (GLint)mipid, 0, 0, lid, (GLsizei)image.width, (GLsizei)image.height, 1, destFormat, destType, finalDataPtr);
	} else {
		Log::Error() << Log::OpenGL << "Unsupported texture upload destination." << std::endl;
	}
	glBindTexture(destination, 0);
	delete[] finalData;
}

TextureInfos GLUtilities::loadTexture(const GLenum target, const std::vector<std::vector<std::string>>& mipsList, const Descriptor & descriptor, Storage mode){
	TextureInfos infos;
	infos.descriptor = descriptor;
	infos.cubemap = target == GL_TEXTURE_CUBE_MAP;
	infos.array = target == GL_TEXTURE_2D_ARRAY;
	infos.mipmap = int(mipsList.size());
	if(mipsList.empty() || mipsList[0].empty()){
		Log::Error() << Log::Resources << "Unable to find texture." << std::endl;
		return infos;
	}
	// Check that the descriptor type is valid.
	GLenum format, type;
	const unsigned int channels = getTypeAndFormat(descriptor.typedFormat, type, format);
	const bool validType = type == GL_FLOAT || type == GL_UNSIGNED_BYTE;
	const bool validFormat = format == GL_RED || format == GL_RG || format == GL_RGB || format == GL_RGBA;
	if(!validType || !validFormat){
		Log::Error() << "Invalid descriptor for creating texture from file." << std::endl;
		return infos;
	}
	
	if(mode & Storage::GPU){
		// Create texture, if only one path, automatically generate mipmaps.
		infos.id = createTexture(target, descriptor, infos.mipmap == 1 ? 0 : infos.mipmap);
		checkGLError();
	}
	
	
	// Cubemaps don't need to be flipped.
	const bool shouldFlip = (target != GL_TEXTURE_CUBE_MAP);
	
	// Load and upload each mip level.
	for(unsigned int mipid = 0; mipid < mipsList.size(); ++mipid){
		const auto & layersList = mipsList[mipid];
		const bool shouldStoreImage = (mipid == 0) && (mode & Storage::CPU);
		
		for(unsigned int lid = 0; lid < layersList.size(); ++lid){
			Image localImage;
			if(shouldStoreImage){
				infos.images.emplace_back();
			}
			Image & image = shouldStoreImage ? infos.images.back() : localImage;
			
			int ret = ImageUtilities::loadImage(layersList[lid], channels, shouldFlip, false, image);
			if (ret != 0) {
				Log::Error() << Log::Resources << "Unable to load the texture at path " << layersList[lid] << "." << std::endl;
				continue;
			}
			// Obtain the reference size of the image.
			if(mipid == 0){
				infos.width = image.width;
				infos.height = image.height;
			}
			
			// Send data to the gpu.
			if(mode & Storage::GPU){
				// Texture arrays are filled by subcopies, and have to be initialized first.
				/// \todo Test in practice.
				if(mipid == 0 && lid == 0 && target == GL_TEXTURE_2D_ARRAY){
					glTexStorage3D(GL_TEXTURE_2D_ARRAY, infos.mipmap, descriptor.typedFormat, infos.width, infos.height, layersList.size());
				}
				uploadTexture(target, infos.id, descriptor.typedFormat, mipid, lid, image);
				checkGLError();
			}
			
		}
	}
	
	// If only level 0 was given, generate mipmaps pyramid automatically.
	if((mode & Storage::GPU) && infos.mipmap == 1){
		glBindTexture(target, infos.id);
		glGenerateMipmap(target);
		glBindTexture(target, 0);
		checkGLError();
	}
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
	unsigned int currentAttribute = 0;
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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	infos.vId = vao;
	infos.eId = ebo;
	infos.count = (GLsizei)mesh.indices.size();
	infos.vbos[0] = vbo;
	infos.vbos[1] = vbo_nor;
	infos.vbos[2] = vbo_uv;
	infos.vbos[3] = vbo_tan;
	infos.vbos[4] = vbo_binor;
	return infos;
}

void GLUtilities::saveDefaultFramebuffer(const unsigned int width, const unsigned int height, const std::string & path){
	
	GLint currentBoundFB = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentBoundFB);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GLUtilities::savePixels(GL_UNSIGNED_BYTE, GL_RGBA, width, height, 4, path, true, true);
	
	glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)currentBoundFB);
}

void GLUtilities::saveFramebuffer(const Framebuffer & framebuffer, const unsigned int width, const unsigned int height, const std::string & path, const bool flip, const bool ignoreAlpha){
	
	GLint currentBoundFB = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentBoundFB);
	
	framebuffer.bind();
	GLenum type, format;
	const unsigned int components = GLUtilities::getTypeAndFormat(framebuffer.typedFormat(), type, format);
	GLUtilities::savePixels(type, format, width, height, components, path, flip, ignoreAlpha);
	
	glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)currentBoundFB);
}


void GLUtilities::savePixels(const GLenum type, const GLenum format, const unsigned int width, const unsigned int height, const unsigned int components, const std::string & path, const bool flip, const bool ignoreAlpha){
	
	glFlush();
	glFinish();
	
	const bool hdr = type == GL_FLOAT;
	
	Log::Info() << Log::OpenGL << "Saving framebuffer to file " << path << (hdr ? ".exr" : ".png") << "... " << std::flush;
	int ret = 0;
	Image image(width, height, components);
	
	const size_t fullSize = image.width*image.height*image.components;
	if(hdr){
		// Get back values.
		glReadPixels(0, 0, (GLsizei)image.width, (GLsizei)image.height, format, type, &image.pixels[0]);
		// Save data.
		ret = ImageUtilities::saveHDRImage(path + ".exr", image, flip, ignoreAlpha);
		
	} else {
		// Get back values.
		GLubyte * data = new GLubyte[fullSize];
		glReadPixels(0, 0, (GLsizei)image.width, (GLsizei)image.height, format, type, &data[0]);
		// Convert to image float format.
		for(size_t pid = 0; pid < fullSize; ++pid){
			image.pixels[pid] = float(data[pid])/255.0f;
		}
		// Save data.
		ret = ImageUtilities::saveLDRImage(path + ".png", image, flip, ignoreAlpha);
		delete[] data;
	}
	
	if(ret != 0){
		Log::Error() << "Error." << std::endl;
	} else {
		Log::Info() << "Done." << std::endl;
	}
	
}

unsigned int GLUtilities::getTypeAndFormat(const GLuint typedFormat, GLuint & type, GLuint & format){
	
	struct FormatAndType {
		GLuint format;
		GLuint type;
	};
	
	static std::map<GLuint, FormatAndType> formatInfos = {
		{ GL_R8, { GL_RED, GL_UNSIGNED_BYTE } },
		{ GL_RG8, { GL_RG, GL_UNSIGNED_BYTE } },
		{ GL_RGB8, { GL_RGB, GL_UNSIGNED_BYTE } },
		{ GL_RGBA8, { GL_RGBA, GL_UNSIGNED_BYTE } },
		{ GL_SRGB8, {GL_RGB, GL_UNSIGNED_BYTE} },
		{ GL_SRGB8_ALPHA8, {GL_RGBA, GL_UNSIGNED_BYTE} },
		{ GL_R16, { GL_RED, GL_UNSIGNED_SHORT } },
		{ GL_RG16, { GL_RG, GL_UNSIGNED_SHORT } },
		{ GL_RGBA16, { GL_RGBA, GL_UNSIGNED_SHORT } },
		{ GL_R8_SNORM, { GL_RED, GL_BYTE } },
		{ GL_RG8_SNORM, { GL_RG, GL_BYTE } },
		{ GL_RGB8_SNORM, { GL_RGB, GL_BYTE } },
		{ GL_RGBA8_SNORM, { GL_RGBA, GL_BYTE } },
		{ GL_R16_SNORM, { GL_RED, GL_SHORT } },
		{ GL_RG16_SNORM, { GL_RG, GL_SHORT } },
		{ GL_RGB16_SNORM, { GL_RGB, GL_SHORT } },
		{ GL_R16F, { GL_RED, GL_HALF_FLOAT } },
		{ GL_RG16F, { GL_RG, GL_HALF_FLOAT } },
		{ GL_RGB16F, { GL_RGB, GL_HALF_FLOAT } },
		{ GL_RGBA16F, { GL_RGBA, GL_HALF_FLOAT } },
		{ GL_R32F, { GL_RED, GL_FLOAT } },
		{ GL_RG32F, { GL_RG, GL_FLOAT } },
		{ GL_RGB32F, { GL_RGB, GL_FLOAT } },
		{ GL_RGBA32F, { GL_RGBA, GL_FLOAT } },
		{ GL_RGB5_A1, { GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1 } },
		{ GL_RGB10_A2, { GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV } },
		{ GL_R11F_G11F_B10F, { GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV } },
		{ GL_DEPTH_COMPONENT16, { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT } },
		{ GL_DEPTH_COMPONENT24, { GL_DEPTH_COMPONENT, GL_UNSIGNED_INT } },
		{ GL_DEPTH_COMPONENT32F, { GL_DEPTH_COMPONENT, GL_FLOAT } },
		{ GL_DEPTH24_STENCIL8, { GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 } },
		{ GL_DEPTH32F_STENCIL8, { GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV } },
		{ GL_R8UI, { GL_RED_INTEGER, GL_UNSIGNED_BYTE } },
		{ GL_R16I, { GL_RED_INTEGER, GL_SHORT } },
		{ GL_R16UI, { GL_RED_INTEGER, GL_UNSIGNED_SHORT } },
		{ GL_R32I, { GL_RED_INTEGER, GL_INT } },
		{ GL_R32UI, { GL_RED_INTEGER, GL_UNSIGNED_INT } },
		{ GL_RG8I, { GL_RG_INTEGER, GL_BYTE } },
		{ GL_RG8UI, { GL_RG_INTEGER, GL_UNSIGNED_BYTE } },
		{ GL_RG16I, { GL_RG_INTEGER, GL_SHORT } },
		{ GL_RG16UI, { GL_RG_INTEGER, GL_UNSIGNED_SHORT } },
		{ GL_RG32I, { GL_RG_INTEGER, GL_INT } },
		{ GL_RG32UI, { GL_RG_INTEGER, GL_UNSIGNED_INT } },
		{ GL_RGB8I, { GL_RGB_INTEGER, GL_BYTE } },
		{ GL_RGB8UI, { GL_RGB_INTEGER, GL_UNSIGNED_BYTE } },
		{ GL_RGB16I, { GL_RGB_INTEGER, GL_SHORT } },
		{ GL_RGB16UI, { GL_RGB_INTEGER, GL_UNSIGNED_SHORT } },
		{ GL_RGB32I, { GL_RGB_INTEGER, GL_INT } },
		{ GL_RGB32UI, { GL_RGB_INTEGER, GL_UNSIGNED_INT } },
		{ GL_RGBA8I, { GL_RGBA_INTEGER, GL_BYTE } },
		{ GL_RGBA8UI, { GL_RGBA_INTEGER ,GL_UNSIGNED_BYTE } },
		{ GL_RGBA16I, { GL_RGBA_INTEGER, GL_SHORT } },
		{ GL_RGBA16UI, { GL_RGBA_INTEGER, GL_UNSIGNED_SHORT } },
		{ GL_RGBA32I, { GL_RGBA_INTEGER, GL_INT } },
		{ GL_RGBA32UI, { GL_RGBA_INTEGER, GL_UNSIGNED_INT } }
	};
	
	if(formatInfos.count(typedFormat)>0){
		const auto & infos = formatInfos[typedFormat];
		type = infos.type;
		format = infos.format;
		const bool oneChannel = (format == GL_RED || format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL);
		return (oneChannel ? 1 : (format == GL_RG ? 2 : (format == GL_RGB ? 3 : 4)));
	}
	
	Log::Error() << Log::OpenGL << "Unable to find type and format (typed format " << typedFormat << ")." << std::endl;
	return 0;
}

GLuint GLUtilities::getMagnificationFilter(const GLuint minificationFilter){
	if(minificationFilter == GL_NEAREST_MIPMAP_NEAREST || minificationFilter == GL_NEAREST_MIPMAP_LINEAR){
		return GL_NEAREST;
	}
	if(minificationFilter == GL_LINEAR_MIPMAP_NEAREST || minificationFilter == GL_LINEAR_MIPMAP_LINEAR){
		return GL_LINEAR;
	}
	return minificationFilter;
}

void GLUtilities::drawMesh(const MeshInfos & mesh) {
	glBindVertexArray(mesh.vId);
	glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, (void*)0);
}

void GLUtilities::bindTextures(const std::vector<const TextureInfos*> & textures, int startingSlot){
	for (unsigned int i = 0; i < textures.size(); ++i){
		const TextureInfos * infos = textures[i];
		glActiveTexture(startingSlot + i);
		const GLenum textureType = infos->cubemap ? (infos->array ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP) : (infos->array ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D);
		glBindTexture(textureType, infos->id);
	}
}

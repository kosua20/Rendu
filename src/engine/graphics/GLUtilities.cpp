#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/Texture.hpp"
#include "resources/Image.hpp"
#include "system/TextUtilities.hpp"


/** Converts a GLenum error number into a human-readable string.
 \param error the OpenGl error value
 \return the corresponding string
 */
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

GLenum GLUtilities::targetFromShape(const TextureShape & shape){
	
	static const std::map<TextureShape, GLenum> shapesTargets = {
		{ TextureShape::D1, GL_TEXTURE_1D },
		{ TextureShape::D2, GL_TEXTURE_2D },
		{ TextureShape::D3, GL_TEXTURE_3D },
		{ TextureShape::Cube, GL_TEXTURE_CUBE_MAP },
		{ TextureShape::Array1D, GL_TEXTURE_1D_ARRAY },
		{ TextureShape::Array2D, GL_TEXTURE_2D_ARRAY },
		{ TextureShape::ArrayCube, GL_TEXTURE_CUBE_MAP_ARRAY }
	};
	return shapesTargets.at(shape);
}

void GLUtilities::setupTexture(Texture & texture, const Descriptor & descriptor){
	
	if(texture.gpu){
		texture.gpu->clean();
	}
	
	texture.gpu.reset(new GPUTexture(descriptor, texture.shape));
	GLuint textureId;
	glGenTextures(1, &textureId);
	texture.gpu->id = textureId;
	
	const GLenum target = texture.gpu->target;
	const GLenum wrap = texture.gpu->wrapping;
	
	glBindTexture(target, textureId);
	
	// Set proper max mipmap level.
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture.levels-1);
	// Texture settings.
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, texture.gpu->minFiltering);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, texture.gpu->magFiltering);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
	glBindTexture(target, 0);
	
	// Allocate.
	GLUtilities::allocateTexture(texture);
	return;
}

void GLUtilities::allocateTexture(const Texture & texture){
	if(!texture.gpu){
		Log::Error() << Log::OpenGL << "Uninitialized GPU texture." << std::endl;
		return;
	}
	
	const GLenum target = texture.gpu->target;
	const GLenum typeFormat = texture.gpu->typedFormat;
	const GLenum type = texture.gpu->type;
	const GLenum format = texture.gpu->format;
	glBindTexture(target, texture.gpu->id);
	
	for(size_t mid = 0; mid < texture.levels; ++mid){
		// Mipmap dimensions.
		const GLsizei w = GLsizei(texture.width/std::pow(2, mid));
		const GLsizei h = GLsizei(texture.height/std::pow(2, mid));
		const GLsizei d = GLsizei(texture.depth/std::pow(2, mid));
		const GLint mip = GLint(mid);
		
		if(texture.shape == TextureShape::D1){
			glTexImage1D(target, mip, typeFormat, w, 0, format, type, nullptr);
			
		} else if(texture.shape == TextureShape::D2){
			glTexImage2D(target, mip, typeFormat, w, h, 0, format, type, nullptr);
			
		} else if(texture.shape == TextureShape::Cube){
			// Here the number of levels is 6.
			if(texture.depth != 6){
				Log::Error() << Log::OpenGL << "Incorrect number of levels in a cubemap (" << texture.depth << ")." << std::endl;
				return;
			}
			// In that case each level is a cubemap face.
			for(size_t lid = 0; lid < texture.depth; ++lid){
				// We need to allocate each level
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lid, mip, typeFormat, w, h, 0, format, type, nullptr);
			}
			
		} else if(texture.shape == TextureShape::D3){
			glTexImage3D(target, mip, typeFormat, w, h, d, 0, format, type, nullptr);
			
		} else if(texture.shape == TextureShape::Array1D){
			// For 1D texture arrays, we do a one-shot allocation using 2D.
			glTexImage2D(target, mip, typeFormat, w, texture.depth, 0, format, type, nullptr);
			
		} else if(texture.shape == TextureShape::Array2D){
			// For 2D texture arrays, we do a one-shot allocation using 3D.
			glTexImage3D(target, mip, typeFormat, w, h, texture.depth, 0, format, type, nullptr);
			
		} else if(texture.shape == TextureShape::ArrayCube){
			// Here the number of levels is a multiple of 6.
			if(texture.depth % 6 != 0){
				Log::Error() << Log::OpenGL << "Incorrect number of levels in a cubemap array (" << texture.depth << ")." << std::endl;
				return;
			}
			// Unsure about this, would expect GL_TEXTURE_CUBE_MAP_ARRAY instead but the doc says so.
			glTexImage3D(GL_TEXTURE_2D_ARRAY, mip, typeFormat, w, h, texture.depth, 0, format, type, nullptr);
			
		} else {
			Log::Error() << Log::OpenGL << "Unsupported texture shape." << std::endl;
			return;
		}
		
	}
	glBindTexture(target, 0);
}

void GLUtilities::uploadTexture(const Texture & texture){
	if(!texture.gpu){
		Log::Error() << Log::OpenGL << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(texture.images.empty()){
		Log::Warning() << Log::OpenGL << "No images to upload." << std::endl;
		return;
	}
	
	const GLenum target = texture.gpu->target;
	const GLenum destType = texture.gpu->type;
	const GLenum destFormat = texture.gpu->format;
	// Sanity check the texture destination format.
	const unsigned int destChannels = texture.gpu->channels;
	if(destChannels != texture.images[0].components){
		Log::Error() << Log::OpenGL << "Not enough values in source data for texture upload." << std::endl;
		return;
	}
	// Check that the descriptor type is valid.
	const bool validType = destType == GL_FLOAT || destType == GL_UNSIGNED_BYTE;
	const bool validFormat = destFormat == GL_RED || destFormat == GL_RG || destFormat == GL_RGB || destFormat == GL_RGBA;
	if(!validType || !validFormat){
		Log::Error() << "Invalid descriptor for creating texture from image data." << std::endl;
		return;
	}
	
	// Determine the required pack alignment.
	const bool defaultAlign = (destType == GL_FLOAT || destType == GL_UNSIGNED_INT || destType == GL_INT) || destChannels == 4;
	glPixelStorei(GL_UNPACK_ALIGNMENT, defaultAlign ? 4 : 1);
	glBindTexture(target, texture.gpu->id);
	
	// For each mip level.
	for(size_t mid = 0; mid < texture.levels; ++ mid){
		// For each layer.
		for(size_t lid = 0; lid < texture.depth; ++lid){
			const Image & image = texture.images[mid * texture.depth + lid];
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
			} else if(destType == GL_FLOAT){
				// Just reinterpret the data.
				finalDataPtr = reinterpret_cast<const GLubyte*>(&image.pixels[0]);
			} else {
				Log::Error() << Log::OpenGL << "Unsupported texture type for upload." << std::endl;
			}
			
			// Upload.
			const GLint mip = GLint(mid);
			const GLsizei w = GLsizei(image.width);
			const GLsizei h = GLsizei(image.height);
			if(target == GL_TEXTURE_1D){
				glTexSubImage1D(target, mip, 0, w, destFormat, destType, finalDataPtr);
				
			} else if(target == GL_TEXTURE_2D){
				glTexSubImage2D(target, mip, 0, 0, w, h, destFormat, destType, finalDataPtr);
				
			} else if(target == GL_TEXTURE_CUBE_MAP){
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lid, mip, 0, 0, w, h, destFormat, destType, finalDataPtr);
				
			} else if(target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY){
				glTexSubImage3D(target, mip, 0, 0, lid, w, h, 1, destFormat, destType, finalDataPtr);
				
			} else if(target == GL_TEXTURE_1D_ARRAY){
				glTexSubImage2D(target, mip, 0, lid, w, 1, destFormat, destType, finalDataPtr);
				
			} else if(target == GL_TEXTURE_3D){
				/// \bug This is false because the number of layers decrease with the mip level. Fix the loop.
				glTexSubImage3D(target, mip, 0, 0, lid, w, h, 1, destFormat, destType, finalDataPtr);
			} else {
				Log::Error() << Log::OpenGL << "Unsupported texture upload destination." << std::endl;
			}
			
			delete[] finalData;
		}
	}
	glBindTexture(target, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void GLUtilities::downloadTexture(Texture & texture){
	if(!texture.gpu){
		Log::Error() << Log::OpenGL << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(texture.shape != TextureShape::D2 && texture.shape != TextureShape::Cube){
		Log::Error() << Log::OpenGL << "Unsupported download format." << std::endl;
		return;
	}
	if(!texture.images.empty()){
		Log::Warning() << Log::OpenGL << "Texture already contain CPU data, will be erased." << std::endl;
		texture.images.clear();
	}
	
	const GLenum target = texture.gpu->target;
	const GLenum type = GL_FLOAT;
	const GLenum format = texture.gpu->format;
	const unsigned int channels = texture.gpu->channels;
	
	// We enforce float type, we can use 4 alignment.
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glBindTexture(target, texture.gpu->id);
	
	// For each mip level.
	for(size_t mid = 0; mid < texture.levels; ++ mid){
		const GLsizei w = GLsizei(texture.width/std::pow(2, mid));
		const GLsizei h = GLsizei(texture.height/std::pow(2, mid));
		const GLint mip = GLint(mid);
		
		if(texture.shape == TextureShape::D2){
			texture.images.emplace_back(w, h, channels);
			Image & image = texture.images.back();
			glGetTexImage(GL_TEXTURE_2D, mip, format, type, &image.pixels[0]);
			
		} else if (texture.shape == TextureShape::Cube){
			for(size_t lid = 0; lid < texture.depth; ++lid){
				texture.images.emplace_back(w, h, channels);
				Image & image = texture.images.back();
				glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lid, mip, format, type, &image.pixels[0]);
			}
			
		}
	}
	glBindTexture(target, 0);
}

void GLUtilities::setupBuffers(Mesh & mesh){
	if(mesh.gpu){
		mesh.gpu->clean();
	}
	mesh.gpu.reset(new GPUMesh());
	GLuint vbo = 0;
	// Generate a vertex array.
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	// Create an array buffer to host the geometry data.
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Compute full allocation size.
	size_t totalSize = 0;
	totalSize += 3 * mesh.positions.size();
	totalSize += 3 * mesh.normals.size();
	totalSize += 2 * mesh.texcoords.size();
	totalSize += 3 * mesh.tangents.size();
	totalSize += 3 * mesh.binormals.size();
	totalSize += 3 * mesh.colors.size();
	
	// Allocate.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * totalSize, NULL, GL_STATIC_DRAW);
	size_t offset = 0;
	// Fill in subregions.
	if(!mesh.positions.empty()){
		const size_t size = sizeof(GLfloat) * 3 * mesh.positions.size();
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, &(mesh.positions[0]));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
		offset += size;
	}
	if(!mesh.normals.empty()){
		const size_t size = sizeof(GLfloat) * 3 * mesh.normals.size();
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, &(mesh.normals[0]));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
		offset += size;
	}
	if(!mesh.texcoords.empty()){
		const size_t size = sizeof(GLfloat) * 2 * mesh.texcoords.size();
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, &(mesh.texcoords[0]));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)offset);
		offset += size;
	}
	if(!mesh.tangents.empty()){
		const size_t size = sizeof(GLfloat) * 3 * mesh.tangents.size();
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, &(mesh.tangents[0]));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
		offset += size;
	}
	if(!mesh.binormals.empty()){
		const size_t size = sizeof(GLfloat) * 3 * mesh.binormals.size();
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, &(mesh.binormals[0]));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
		offset += size;
	}
	if(!mesh.colors.empty()){
		const size_t size = sizeof(GLfloat) * 3 * mesh.colors.size();
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, &(mesh.colors[0]));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
		offset += size;
	}
	
	// We load the indices data
	GLuint ebo = 0;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.indices.size(), &(mesh.indices[0]), GL_STATIC_DRAW);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	mesh.gpu->vId = vao;
	mesh.gpu->eId = ebo;
	mesh.gpu->count = (GLsizei)mesh.indices.size();
	mesh.gpu->vbo = vbo;
}

void GLUtilities::saveFramebuffer(const Framebuffer & framebuffer, const unsigned int width, const unsigned int height, const std::string & path, const bool flip, const bool ignoreAlpha){
	
	GLint currentBoundFB = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentBoundFB);
	
	framebuffer.bind();
	const std::unique_ptr<GPUTexture> & gpu = framebuffer.textureId()->gpu;
	GLUtilities::savePixels(gpu->type, gpu->format, width, height, gpu->channels, path, flip, ignoreAlpha);
	
	glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)currentBoundFB);
}

void GLUtilities::sync(){
	glFlush();
	glFinish();
}

void GLUtilities::savePixels(const GLenum type, const GLenum format, const unsigned int width, const unsigned int height, const unsigned int components, const std::string & path, const bool flip, const bool ignoreAlpha){
	
	GLUtilities::sync();
	
	const bool hdr = type == GL_FLOAT;
	
	Log::Info() << Log::OpenGL << "Saving framebuffer to file " << path << (hdr ? ".exr" : ".png") << "... " << std::flush;
	int ret = 0;
	Image image(width, height, components);
	
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	const size_t fullSize = image.width*image.height*image.components;
	if(hdr){
		// Get back values.
		glReadPixels(0, 0, (GLsizei)image.width, (GLsizei)image.height, format, type, &image.pixels[0]);
		// Save data.
		ret = Image::saveHDRImage(path + ".exr", image, flip, ignoreAlpha);
		
	} else {
		// Get back values.
		GLubyte * data = new GLubyte[fullSize];
		glReadPixels(0, 0, (GLsizei)image.width, (GLsizei)image.height, format, type, &data[0]);
		// Convert to image float format.
		for(size_t pid = 0; pid < fullSize; ++pid){
			image.pixels[pid] = float(data[pid])/255.0f;
		}
		// Save data.
		ret = Image::saveLDRImage(path + ".png", image, flip, ignoreAlpha);
		delete[] data;
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	
	if(ret != 0){
		Log::Error() << "Error." << std::endl;
	} else {
		Log::Info() << "Done." << std::endl;
	}
	
}

void GLUtilities::generateMipMaps(const Texture & texture){
	if(!texture.gpu){
		Log::Error() << Log::OpenGL << "Uninitialized GPU texture." << std::endl;
		return;
	}
	const GLenum target = texture.gpu->target;
	glBindTexture(target, texture.gpu->id);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 1000);
	glGenerateMipmap(target);
	glBindTexture(target, 0);
}

void GLUtilities::drawMesh(const Mesh & mesh) {
	glBindVertexArray(mesh.gpu->vId);
	glDrawElements(GL_TRIANGLES, mesh.gpu->count, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
}

void GLUtilities::bindTexture(const Texture * texture, unsigned int slot){
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(texture->gpu->target, texture->gpu->id);
}


void GLUtilities::bindTextures(const std::vector<const Texture*> & textures, unsigned int startingSlot){
	for (unsigned int i = 0; i < textures.size(); ++i){
		const Texture * infos = textures[i];
		glActiveTexture(GL_TEXTURE0 + startingSlot + i);
		glBindTexture(infos->gpu->target, infos->gpu->id);
	}
}

void GLUtilities::deviceInfos(std::string & vendor, std::string & renderer, std::string & version, std::string & shaderVersion){
	const GLubyte * vendorString = glGetString(GL_VENDOR);
	const GLubyte * rendererString = glGetString(GL_RENDERER);
	const GLubyte * versionString = glGetString(GL_VERSION);
	const GLubyte * glslVersionString = glGetString(GL_SHADING_LANGUAGE_VERSION);
	vendor = std::string(reinterpret_cast<const char*>(vendorString));
	renderer = std::string(reinterpret_cast<const char*>(rendererString));
	version = std::string(reinterpret_cast<const char*>(versionString));
	shaderVersion = std::string(reinterpret_cast<const char*>(glslVersionString));
}

std::vector<std::string> GLUtilities::deviceExtensions(){
	int extensionCount = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
	std::vector<std::string> extensions(extensionCount);
	
	for(int i = 0; i < extensionCount; ++i){
		const GLubyte * extStr = glGetStringi(GL_EXTENSIONS, i);
		extensions[i] = std::string(reinterpret_cast<const char*>(extStr));
	}
	return extensions;
}

void GLUtilities::setViewport(int x, int y, int w, int h){
	glViewport(GLsizei(x), GLsizei(y), GLsizei(w), GLsizei(h));
}

void GLUtilities::clearColor(const glm::vec4 & color){
	glClearColor(color[0], color[1], color[2], color[3]);
	glClear(GL_COLOR_BUFFER_BIT);
}

void GLUtilities::clearDepth(float depth){
	glClearDepth(depth);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void GLUtilities::clearColorAndDepth(const glm::vec4 & color, float depth){
	glClearColor(color[0], color[1], color[2], color[3]);
	glClearDepth(depth);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLUtilities::blit(const Framebuffer & src, const Framebuffer & dst, Filter filter){
	src.bind(Framebuffer::Mode::READ);
	dst.bind(Framebuffer::Mode::WRITE);
	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
	glBlitFramebuffer(0, 0, src.width(), src.height(), 0, 0, src.width(), src.height(), GL_COLOR_BUFFER_BIT, filterGL);
	src.unbind();
	dst.unbind();
}

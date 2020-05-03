#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/Texture.hpp"
#include "resources/Image.hpp"
#include "system/TextUtilities.hpp"

#include <sstream>

/** Converts a GLenum error number into a human-readable string.
 \param error the OpenGl error value
 \return the corresponding string
 */
std::string getGLErrorString(GLenum error) {
	std::string msg;
	switch(error) {
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

int checkGLFramebufferError() {
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE) {
		switch(status) {
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

int _checkGLError(const char * file, int line, const std::string & infos) {
	const GLenum glErr = glGetError();
	if(glErr != GL_NO_ERROR) {
		const std::string filePath(file);
		size_t pos = std::min(filePath.find_last_of('/'), filePath.find_last_of('\\'));
		if(pos == std::string::npos) {
			pos = 0;
		}
		Log::Error() << Log::OpenGL << "Error " << getGLErrorString(glErr) << " in " << filePath.substr(pos + 1) << " (" << line << ").";
		if(!infos.empty()) {
			Log::Error() << " Infos: " << infos;
		}
		Log::Error() << std::endl;
		return 1;
	}
	return 0;
}

void GLUtilities::setup() {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_FRAMEBUFFER_SRGB);
}

GLuint GLUtilities::loadShader(const std::string & prog, ShaderType type, Bindings & bindings, std::string & finalLog) {
	// We need to detect texture slots and store them, to avoid having to register them in
	// the rest of the code (object, renderer), while not having support for 'layout(binding=n)' in OpenGL <4.2.
	std::stringstream inputLines(prog);
	std::vector<std::string> outputLines;
	std::string line;
	bool isInMultiLineComment = false;
	while(std::getline(inputLines, line)) {

		// Comment handling.
		const std::string::size_type commentPosBegin = line.find("/*");
		const std::string::size_type commentPosEnd	 = line.rfind("*/");
		const std::string::size_type commentMonoPos	 = line.find("//");
		// We suppose no multi-line comment nesting, that way we can tackle them linearly.
		if(commentPosBegin != std::string::npos && commentPosEnd != std::string::npos) {
			// Both token exist.
			// Either this is "end begin", in which case we are still in a comment.
			// Or this is "begin end", ie a single ligne comment.
			isInMultiLineComment = commentPosBegin > commentPosEnd;
		} else if(commentPosEnd != std::string::npos) {
			// Only an end token.
			isInMultiLineComment = false;
		} else if(commentPosBegin != std::string::npos) {
			// Only a begin token.
			isInMultiLineComment = true;
		}

		// Find a line containing "layout...binding...uniform..."
		const std::string::size_type layoutPos	= line.find("layout");
		const std::string::size_type bindingPos = line.find("binding");
		const std::string::size_type uniformPos = line.find("uniform");

		const bool isNotALayoutBindingUniform		 = (layoutPos == std::string::npos || bindingPos == std::string::npos || uniformPos == std::string::npos);
		const bool isALayoutInsideAMultiLineComment	 = isInMultiLineComment && (layoutPos > commentPosBegin || uniformPos < commentPosEnd);
		const bool isALayoutInsideASingleLineComment = commentMonoPos != std::string::npos && layoutPos > commentMonoPos;
		if(isNotALayoutBindingUniform || isALayoutInsideAMultiLineComment || isALayoutInsideASingleLineComment) {
			// We don't modify the line.
			outputLines.push_back(line);
			continue;
		}
		// Extract the statement.
		const std::string::size_type startStatement = std::min(layoutPos, uniformPos);
		const std::string::size_type endStatement	= line.find_first_of(";{", startStatement);
		int slot									= 0;
		std::string name;
		{
			const std::string statement = TextUtilities::trim(line.substr(startStatement, endStatement - startStatement), "\t ");
			// Extract the location and the name.
			const std::string::size_type bindingPosSub = statement.find("binding");
			const std::string::size_type firstSlotPos  = statement.find_first_of("0123456789", bindingPosSub);
			const std::string::size_type lastSlotPos   = statement.find_first_not_of("0123456789", firstSlotPos) - 1;
			const std::string::size_type startPosName  = statement.find_last_of(" \t") + 1;
			name									   = statement.substr(startPosName);
			slot									   = std::stoi(statement.substr(firstSlotPos, lastSlotPos - firstSlotPos + 1));
		}
		// Two possibles cases, sampler or buffer.
		const std::string::size_type samplerPos = line.find("sampler", layoutPos);
		const bool isSampler					= samplerPos != std::string::npos;
		if(isSampler) {
			const std::string::size_type endSamplerPos	 = line.find_first_of(' ', samplerPos) - 1;
			const std::string::size_type startSamplerPos = line.find_last_of(' ', samplerPos) + 1;
			const std::string samplerType				 = line.substr(startSamplerPos, endSamplerPos - startSamplerPos + 1);
			std::string outputLine						 = "uniform " + samplerType + " ";
			outputLine += name + ";";
			outputLines.push_back(outputLine);
		} else {
			// We just need to remove the binding spec from the layout.
			const std::string::size_type layoutContentStart = line.find_first_of("(", layoutPos) + 1;
			const std::string::size_type layoutContentEnd	= line.find_first_of(")", layoutContentStart);
			// Two options: either binding is the only argument.
			const std::string::size_type splitPos = line.find_first_of(",", layoutContentStart, layoutContentEnd - layoutContentStart);
			if(splitPos == std::string::npos) {
				// Remove layout entirely.
				const std::string outputLine = line.substr(0, layoutPos) + line.substr(layoutContentEnd + 1);
				outputLines.push_back(outputLine);
			} else {
				// Or there are other specifiers to preserve.
				std::string::size_type sepBefore = line.find_last_of("(,", bindingPos);
				std::string::size_type sepAfter	 = line.find_first_of("),", bindingPos);
				if(line[sepBefore] == '(') {
					sepBefore += 1;
				}
				if(line[sepAfter] == ')') {
					sepAfter -= 1;
				}
				const std::string outputLine = line.substr(0, sepBefore) + line.substr(sepAfter + 1);
				outputLines.push_back(outputLine);
			}
		}

		if(bindings.count(name) > 0 && bindings[name].location != slot) {
			Log::Warning() << Log::OpenGL << "Inconsistent binding location between linked shaders for \"" << name << "\"." << std::endl;
		}
		bindings[name].location = slot;
		bindings[name].type		= isSampler ? BindingType::TEXTURE : BindingType::UNIFORM_BUFFER;
		Log::Verbose() << Log::OpenGL << "Detected binding (" << name << ", " << slot << ") => " << outputLines.back() << std::endl;
	}
	std::string outputProg;
	for(const auto & outputLine : outputLines) {
		outputProg.append(outputLine + "\n");
	}

	// Create shader object.
	static const std::map<ShaderType, GLenum> types = {
		{ShaderType::VERTEX, GL_VERTEX_SHADER},
		{ShaderType::FRAGMENT, GL_FRAGMENT_SHADER},
		{ShaderType::GEOMETRY, GL_GEOMETRY_SHADER}
	};

	GLuint id = glCreateShader(types.at(type));
	checkGLError();
	// Setup string as source.
	const char * shaderProg = outputProg.c_str();
	glShaderSource(id, 1, &shaderProg, static_cast<const GLint *>(nullptr));
	// Compile the shader on the GPU.
	glCompileShader(id);
	checkGLError();

	GLint success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	finalLog = "";
	// If compilation failed, get information and display it.
	if(success != GL_TRUE) {
		// Get the log string length for allocation.
		GLint infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		// Get the log string.
		std::vector<char> infoLog(size_t(std::max(infoLogLength, int(1))));
		glGetShaderInfoLog(id, infoLogLength, nullptr, &infoLog[0]);
		// Indent and clean.
		std::string infoLogString(infoLog.data(), infoLogLength);

		TextUtilities::replace(infoLogString, "\n", "\n\t");
		infoLogString.insert(0, "\t");
		finalLog = infoLogString;
	}
	// Return the id to the successfuly compiled shader program.
	return id;
}

GLuint GLUtilities::createProgram(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, Bindings & bindings, const std::string & debugInfos) {
	GLuint vp(0), fp(0), gp(0);
	const GLuint id = glCreateProgram();
	checkGLError();

	Log::Verbose() << Log::OpenGL << "Compiling " << debugInfos << "." << std::endl;

	std::string compilationLog;
	// If vertex program code is given, compile it.
	if(!vertexContent.empty()) {
		vp = loadShader(vertexContent, ShaderType::VERTEX, bindings, compilationLog);
		glAttachShader(id, vp);
		if(!compilationLog.empty()) {
			Log::Error() << Log::OpenGL << "Vertex shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If fragment program code is given, compile it.
	if(!fragmentContent.empty()) {
		fp = loadShader(fragmentContent, ShaderType::FRAGMENT, bindings, compilationLog);
		glAttachShader(id, fp);
		if(!compilationLog.empty()) {
			Log::Error() << Log::OpenGL << "Fragment shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If geometry program code is given, compile it.
	if(!geometryContent.empty()) {
		gp = loadShader(geometryContent, ShaderType::GEOMETRY, bindings, compilationLog);
		glAttachShader(id, gp);
		if(!compilationLog.empty()) {
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
		std::vector<char> infoLog(size_t(std::max(infoLogLength, int(1))));
		glGetProgramInfoLog(id, infoLogLength, nullptr, &infoLog[0]);
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
	if(vp != 0) {
		glDetachShader(id, vp);
	}
	if(fp != 0) {
		glDetachShader(id, fp);
	}
	if(gp != 0) {
		glDetachShader(id, gp);
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

void GLUtilities::saveFramebuffer(const Framebuffer & framebuffer, unsigned int width, unsigned int height, const std::string & path, bool flip, bool ignoreAlpha) {

	GLint currentBoundFB = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &currentBoundFB);

	framebuffer.bind(Framebuffer::Mode::READ);
	const std::unique_ptr<GPUTexture> & gpu = framebuffer.texture()->gpu;
	GLUtilities::savePixels(gpu->type, gpu->format, width, height, gpu->channels, path, flip, ignoreAlpha);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(currentBoundFB));
}

void GLUtilities::bindTexture(const Texture * texture, size_t slot) {
	glActiveTexture(GLenum(GL_TEXTURE0 + slot));
	glBindTexture(texture->gpu->target, texture->gpu->id);
}

void GLUtilities::bindTextures(const std::vector<const Texture *> & textures, size_t startingSlot) {
	for(size_t i = 0; i < textures.size(); ++i) {
		const Texture * infos = textures[i];
		glActiveTexture(GLenum(GL_TEXTURE0 + startingSlot + i));
		glBindTexture(infos->gpu->target, infos->gpu->id);
	}
}

void GLUtilities::setupTexture(Texture & texture, const Descriptor & descriptor) {

	if(texture.gpu) {
		texture.gpu->clean();
	}

	texture.gpu.reset(new GPUTexture(descriptor, texture.shape));
	GLuint textureId;
	glGenTextures(1, &textureId);
	texture.gpu->id = textureId;

	const GLenum target = texture.gpu->target;
	const GLenum wrap	= texture.gpu->wrapping;

	glBindTexture(target, textureId);

	// Set proper max mipmap level.
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture.levels - 1);
	// Texture settings.
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, texture.gpu->minFiltering);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, texture.gpu->magFiltering);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
	glBindTexture(target, 0);

	// Allocate.
	GLUtilities::allocateTexture(texture);
}

void GLUtilities::allocateTexture(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::OpenGL << "Uninitialized GPU texture." << std::endl;
		return;
	}

	const GLenum target		= texture.gpu->target;
	const GLenum typeFormat = texture.gpu->typedFormat;
	const GLenum type		= texture.gpu->type;
	const GLenum format		= texture.gpu->format;
	glBindTexture(target, texture.gpu->id);

	for(size_t mid = 0; mid < texture.levels; ++mid) {
		// Mipmap dimensions.
		const GLsizei w = GLsizei(std::max<uint>(1, texture.width / (1 << mid)));
		const GLsizei h = GLsizei(std::max<uint>(1, texture.height / (1 << mid)));

		const GLint mip = GLint(mid);

		if(texture.shape == TextureShape::D1) {
			glTexImage1D(target, mip, typeFormat, w, 0, format, type, nullptr);

		} else if(texture.shape == TextureShape::D2) {
			glTexImage2D(target, mip, typeFormat, w, h, 0, format, type, nullptr);

		} else if(texture.shape == TextureShape::Cube) {
			// Here the number of levels is 6.
			if(texture.depth != 6) {
				Log::Error() << Log::OpenGL << "Incorrect number of levels in a cubemap (" << texture.depth << ")." << std::endl;
				return;
			}
			// In that case each level is a cubemap face.
			for(size_t lid = 0; lid < texture.depth; ++lid) {
				// We need to allocate each level
				glTexImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lid), mip, typeFormat, w, h, 0, format, type, nullptr);
			}

		} else if(texture.shape == TextureShape::D3) {
			const GLsizei d = GLsizei(std::max<uint>(1, texture.depth / (1 << mid)));
			glTexImage3D(target, mip, typeFormat, w, h, d, 0, format, type, nullptr);

		} else if(texture.shape == TextureShape::Array1D) {
			// For 1D texture arrays, we do a one-shot allocation using 2D.
			glTexImage2D(target, mip, typeFormat, w, texture.depth, 0, format, type, nullptr);

		} else if(texture.shape == TextureShape::Array2D) {
			// For 2D texture arrays, we do a one-shot allocation using 3D.
			glTexImage3D(target, mip, typeFormat, w, h, texture.depth, 0, format, type, nullptr);

		} else if(texture.shape == TextureShape::ArrayCube) {
			// Here the number of levels is a multiple of 6.
			if(texture.depth % 6 != 0) {
				Log::Error() << Log::OpenGL << "Incorrect number of levels in a cubemap array (" << texture.depth << ")." << std::endl;
				return;
			}
			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, typeFormat, w, h, texture.depth, 0, format, type, nullptr);

		} else {
			Log::Error() << Log::OpenGL << "Unsupported texture shape." << std::endl;
			return;
		}
	}
	glBindTexture(target, 0);
}

void GLUtilities::uploadTexture(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::OpenGL << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(texture.images.empty()) {
		Log::Warning() << Log::OpenGL << "No images to upload." << std::endl;
		return;
	}

	const GLenum target		= texture.gpu->target;
	const GLenum destFormat = texture.gpu->format;
	// Sanity check the texture destination format.
	const unsigned int destChannels = texture.gpu->channels;
	if(destChannels != texture.images[0].components) {
		Log::Error() << Log::OpenGL << "Not enough values in source data for texture upload." << std::endl;
		return;
	}
	// Check that the descriptor type is valid.
	const bool validFormat = destFormat == GL_RED || destFormat == GL_RG || destFormat == GL_RGB || destFormat == GL_RGBA;
	if(!validFormat) {
		Log::Error() << "Invalid descriptor for creating texture from image data." << std::endl;
		return;
	}

	// We always upload data as floats (and let the driver convert internally if needed),
	// so the alignment is always 4.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(target, texture.gpu->id);

	int currentImg = 0;
	// For each mip level.
	for(size_t mid = 0; mid < texture.levels; ++mid) {
		// For 3D textures, the number of layers decreases with the mip level.
		const size_t depth = target == GL_TEXTURE_3D ? (texture.depth / (1 << mid)) : texture.depth;
		// For each layer.
		for(size_t lid = 0; lid < depth; ++lid) {
			const Image & image = texture.images[currentImg];
			currentImg += 1;
			// Upload.
			const GLubyte * finalDataPtr = reinterpret_cast<const GLubyte *>(image.pixels.data());
			const GLint mip = GLint(mid);
			const GLint lev = GLint(lid);
			const GLsizei w = GLsizei(image.width);
			const GLsizei h = GLsizei(image.height);
			if(target == GL_TEXTURE_1D) {
				glTexSubImage1D(target, mip, 0, w, destFormat, GL_FLOAT, finalDataPtr);

			} else if(target == GL_TEXTURE_2D) {
				glTexSubImage2D(target, mip, 0, 0, w, h, destFormat, GL_FLOAT, finalDataPtr);

			} else if(target == GL_TEXTURE_CUBE_MAP) {
				glTexSubImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lev), mip, 0, 0, w, h, destFormat, GL_FLOAT, finalDataPtr);

			} else if(target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY) {
				glTexSubImage3D(target, mip, 0, 0, lev, w, h, 1, destFormat, GL_FLOAT, finalDataPtr);

			} else if(target == GL_TEXTURE_1D_ARRAY) {
				glTexSubImage2D(target, mip, 0, lev, w, 1, destFormat, GL_FLOAT, finalDataPtr);

			} else if(target == GL_TEXTURE_3D) {
				glTexSubImage3D(target, mip, 0, 0, lev, w, h, 1, destFormat, GL_FLOAT, finalDataPtr);

			} else {
				Log::Error() << Log::OpenGL << "Unsupported texture upload destination." << std::endl;
			}
		}
	}
	glBindTexture(target, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void GLUtilities::downloadTexture(Texture & texture) {
	downloadTexture(texture, -1);
}

void GLUtilities::downloadTexture(Texture & texture, int level) {
	if(!texture.gpu) {
		Log::Error() << Log::OpenGL << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(texture.shape != TextureShape::D2 && texture.shape != TextureShape::Cube) {
		Log::Error() << Log::OpenGL << "Unsupported download format." << std::endl;
		return;
	}
	if(!texture.images.empty()) {
		Log::Verbose() << Log::OpenGL << "Texture already contain CPU data, will be erased." << std::endl;
	}
	texture.images.resize(texture.depth * texture.levels);

	const GLenum target			= texture.gpu->target;
	const GLenum type			= GL_FLOAT;
	const GLenum format			= texture.gpu->format;
	const unsigned int channels = texture.gpu->channels;

	// We enforce float type, we can use 4 alignment.
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glBindTexture(target, texture.gpu->id);

	// For each mip level.
	for(size_t mid = 0; mid < texture.levels; ++mid) {
		if(level >= 0 && int(mid) != level) {
			continue;
		}
		const GLsizei w = GLsizei(std::max<uint>(1, texture.width / (1 << mid)));
		const GLsizei h = GLsizei(std::max<uint>(1, texture.height / (1 << mid)));
		const GLint mip = GLint(mid);

		if(texture.shape == TextureShape::D2) {
			texture.images[mid] = Image(w, h, channels);
			Image & image		= texture.images[mid];
			glGetTexImage(GL_TEXTURE_2D, mip, format, type, &image.pixels[0]);

		} else if(texture.shape == TextureShape::Cube) {
			for(size_t lid = 0; lid < texture.depth; ++lid) {
				const size_t id	   = mid * texture.levels + lid;
				texture.images[id] = Image(w, h, channels);
				Image & image	   = texture.images[id];
				glGetTexImage(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lid), mip, format, type, &image.pixels[0]);
			}
		}
	}
	glBindTexture(target, 0);
}

void GLUtilities::generateMipMaps(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::OpenGL << "Uninitialized GPU texture." << std::endl;
		return;
	}
	const GLenum target = texture.gpu->target;
	glBindTexture(target, texture.gpu->id);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture.levels - 1);
	glGenerateMipmap(target);
	glBindTexture(target, 0);
}

GLenum GLUtilities::targetFromShape(const TextureShape & shape) {

	static const std::map<TextureShape, GLenum> shapesTargets = {
		{TextureShape::D1, GL_TEXTURE_1D},
		{TextureShape::D2, GL_TEXTURE_2D},
		{TextureShape::D3, GL_TEXTURE_3D},
		{TextureShape::Cube, GL_TEXTURE_CUBE_MAP},
		{TextureShape::Array1D, GL_TEXTURE_1D_ARRAY},
		{TextureShape::Array2D, GL_TEXTURE_2D_ARRAY},
		{TextureShape::ArrayCube, GL_TEXTURE_CUBE_MAP_ARRAY}};
	return shapesTargets.at(shape);
}

void GLUtilities::bindBuffer(const BufferBase & buffer, size_t slot) {
	glBindBuffer(GL_UNIFORM_BUFFER, buffer.gpu->id);
	glBindBufferBase(GL_UNIFORM_BUFFER, GLuint(slot), buffer.gpu->id);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GLUtilities::setupBuffer(BufferBase & buffer) {
	if(buffer.gpu) {
		buffer.gpu->clean();
	}
	// Create.
	buffer.gpu.reset(new GPUBuffer(buffer.type, buffer.usage));
	GLuint bufferId;
	glGenBuffers(1, &bufferId);
	buffer.gpu->id = bufferId;
	// Allocate.
	GLUtilities::allocateBuffer(buffer);
}

void GLUtilities::allocateBuffer(const BufferBase & buffer) {
	if(!buffer.gpu) {
		Log::Error() << Log::OpenGL << "Uninitialized GPU buffer." << std::endl;
		return;
	}

	const GLenum target = buffer.gpu->target;
	glBindBuffer(target, buffer.gpu->id);
	glBufferData(target, buffer.sizeMax, nullptr, buffer.gpu->usage);
	glBindBuffer(target, 0);
}

void GLUtilities::uploadBuffer(const BufferBase & buffer, size_t size, unsigned char * data, size_t offset) {
	if(!buffer.gpu) {
		Log::Error() << Log::OpenGL << "Uninitialized GPU buffer." << std::endl;
		return;
	}
	if(size == 0) {
		Log::Warning() << Log::OpenGL << "No data to upload." << std::endl;
		return;
	}
	if(offset + size > buffer.sizeMax) {
		Log::Warning() << Log::OpenGL << "Not enough allocated space to upload." << std::endl;
		return;
	}

	const GLenum target = buffer.gpu->target;
	glBindBuffer(target, buffer.gpu->id);
	glBufferSubData(target, offset, size, data);
	glBindBuffer(target, 0);
}

void GLUtilities::downloadBuffer(const BufferBase & buffer, size_t size, unsigned char * data, size_t offset) {
	if(!buffer.gpu) {
		Log::Error() << Log::OpenGL << "Uninitialized GPU buffer." << std::endl;
		return;
	}
	if(offset + size > buffer.sizeMax) {
		Log::Warning() << Log::OpenGL << "Not enough available data to download." << std::endl;
		return;
	}

	const GLenum target = buffer.gpu->target;
	glBindBuffer(target, buffer.gpu->id);
	glGetBufferSubData(target, offset, size, data);
	glBindBuffer(target, 0);
}

void GLUtilities::setupMesh(Mesh & mesh) {
	if(mesh.gpu) {
		mesh.gpu->clean();
	}
	mesh.gpu.reset(new GPUMesh());
	// Generate a vertex array.
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Compute full allocation size.
	size_t totalSize = 0;
	totalSize += 3 * mesh.positions.size();
	totalSize += 3 * mesh.normals.size();
	totalSize += 2 * mesh.texcoords.size();
	totalSize += 3 * mesh.tangents.size();
	totalSize += 3 * mesh.binormals.size();
	totalSize += 3 * mesh.colors.size();

	// Create an array buffer to host the geometry data.
	BufferBase vertexBuffer(sizeof(GLfloat) * totalSize, BufferType::VERTEX, DataUse::STATIC);
	GLUtilities::setupBuffer(vertexBuffer);
	// Fill in subregions.
	size_t offset = 0;
	if(!mesh.positions.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.positions.size();
		GLUtilities::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.positions.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.normals.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.normals.size();
		GLUtilities::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.normals.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.texcoords.empty()) {
		const size_t size = sizeof(GLfloat) * 2 * mesh.texcoords.size();
		GLUtilities::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.texcoords.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.tangents.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.tangents.size();
		GLUtilities::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.tangents.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.binormals.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.binormals.size();
		GLUtilities::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.binormals.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.colors.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.colors.size();
		GLUtilities::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.colors.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
	}

	// We load the indices data
	const size_t inSize = sizeof(unsigned int) * mesh.indices.size();
	BufferBase indexBuffer(inSize, BufferType::INDEX, DataUse::STATIC);
	GLUtilities::setupBuffer(indexBuffer);
	GLUtilities::uploadBuffer(indexBuffer, inSize, reinterpret_cast<unsigned char *>(mesh.indices.data()));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.gpu->id);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	mesh.gpu->id		   = vao;
	mesh.gpu->count		   = GLsizei(mesh.indices.size());
	mesh.gpu->indexBuffer  = std::move(indexBuffer.gpu);
	mesh.gpu->vertexBuffer = std::move(vertexBuffer.gpu);
}

void GLUtilities::drawMesh(const Mesh & mesh) {
	glBindVertexArray(mesh.gpu->id);
	glDrawElements(GL_TRIANGLES, mesh.gpu->count, GL_UNSIGNED_INT, static_cast<void *>(nullptr));
	glBindVertexArray(0);
}

void GLUtilities::sync() {
	glFlush();
	glFinish();
}

void GLUtilities::deviceInfos(std::string & vendor, std::string & renderer, std::string & version, std::string & shaderVersion) {
	const GLubyte * vendorString	  = glGetString(GL_VENDOR);
	const GLubyte * rendererString	  = glGetString(GL_RENDERER);
	const GLubyte * versionString	  = glGetString(GL_VERSION);
	const GLubyte * glslVersionString = glGetString(GL_SHADING_LANGUAGE_VERSION);
	vendor							  = std::string(reinterpret_cast<const char *>(vendorString));
	renderer						  = std::string(reinterpret_cast<const char *>(rendererString));
	version							  = std::string(reinterpret_cast<const char *>(versionString));
	shaderVersion					  = std::string(reinterpret_cast<const char *>(glslVersionString));
}

std::vector<std::string> GLUtilities::deviceExtensions() {
	int extensionCount = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
	std::vector<std::string> extensions(extensionCount);

	for(int i = 0; i < extensionCount; ++i) {
		const GLubyte * extStr = glGetStringi(GL_EXTENSIONS, i);
		extensions[i]		   = std::string(reinterpret_cast<const char *>(extStr));
	}
	return extensions;
}

void GLUtilities::setViewport(int x, int y, int w, int h) {
	glViewport(GLsizei(x), GLsizei(y), GLsizei(w), GLsizei(h));
}

void GLUtilities::clearColor(const glm::vec4 & color) {
	glClearColor(color[0], color[1], color[2], color[3]);
	glClear(GL_COLOR_BUFFER_BIT);
}

void GLUtilities::clearDepth(float depth) {
	glClearDepth(depth);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void GLUtilities::clearColorAndDepth(const glm::vec4 & color, float depth) {
	glClearColor(color[0], color[1], color[2], color[3]);
	glClearDepth(depth);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLUtilities::setDepthState(bool test) {
	(test ? glEnable : glDisable)(GL_DEPTH_TEST);
}

void GLUtilities::setDepthState(bool test, DepthEquation equation, bool write) {
	(test ? glEnable : glDisable)(GL_DEPTH_TEST);
	static const std::map<DepthEquation, GLenum> eqs = {
		{DepthEquation::NEVER, GL_NEVER},
		{DepthEquation::LESS, GL_LESS},
		{DepthEquation::LEQUAL, GL_LEQUAL},
		{DepthEquation::EQUAL, GL_EQUAL},
		{DepthEquation::GREATER, GL_GREATER},
		{DepthEquation::GEQUAL, GL_GEQUAL},
		{DepthEquation::NOTEQUAL, GL_NOTEQUAL},
		{DepthEquation::ALWAYS, GL_ALWAYS}};
	glDepthFunc(eqs.at(equation));
	glDepthMask(write ? GL_TRUE : GL_FALSE);
}

void GLUtilities::setBlendState(bool test) {
	(test ? glEnable : glDisable)(GL_BLEND);
}

void GLUtilities::setBlendState(bool test, BlendEquation equation, BlendFunction src, BlendFunction dst) {
	static const std::map<BlendEquation, GLenum> eqs = {
		{BlendEquation::ADD, GL_FUNC_ADD},
		{BlendEquation::SUBTRACT, GL_FUNC_SUBTRACT},
		{BlendEquation::REVERSE_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT},
		{BlendEquation::MIN, GL_MIN},
		{BlendEquation::MAX, GL_MAX}};
	static const std::map<BlendFunction, GLenum> funcs = {
		{BlendFunction::ONE, GL_ONE},
		{BlendFunction::ZERO, GL_ZERO},
		{BlendFunction::SRC_COLOR, GL_SRC_COLOR},
		{BlendFunction::ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR},
		{BlendFunction::SRC_ALPHA, GL_SRC_ALPHA},
		{BlendFunction::ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA},
		{BlendFunction::DST_COLOR, GL_DST_COLOR},
		{BlendFunction::ONE_MINUS_DST_COLOR, GL_ONE_MINUS_DST_COLOR},
		{BlendFunction::DST_ALPHA, GL_DST_ALPHA},
		{BlendFunction::ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA}};
	glBlendFunc(funcs.at(src), funcs.at(dst));
	glBlendEquation(eqs.at(equation));
	(test ? glEnable : glDisable)(GL_BLEND);
}

void GLUtilities::setCullState(bool cull) {
	(cull ? glEnable : glDisable)(GL_CULL_FACE);
}

static const std::map<Faces, GLenum> faces = {
	{Faces::FRONT, GL_FRONT},
	{Faces::BACK, GL_BACK},
	{Faces::ALL, GL_FRONT_AND_BACK}};

void GLUtilities::setCullState(bool cull, Faces culledFaces) {
	(cull ? glEnable : glDisable)(GL_CULL_FACE);
	glCullFace(faces.at(culledFaces));
}

void GLUtilities::setPolygonState(PolygonMode mode, Faces selectedFaces) {
	static const std::map<PolygonMode, GLenum> modes = {
		{PolygonMode::FILL, GL_FILL},
		{PolygonMode::LINE, GL_LINE},
		{PolygonMode::POINT, GL_POINT}};
	glPolygonMode(faces.at(selectedFaces), modes.at(mode));
}

void GLUtilities::blitDepth(const Framebuffer & src, const Framebuffer & dst) {
	src.bind(Framebuffer::Mode::READ);
	dst.bind(Framebuffer::Mode::WRITE);
	glBlitFramebuffer(0, 0, src.width(), src.height(), 0, 0, dst.width(), dst.height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	src.unbind();
	dst.unbind();
}

void GLUtilities::blit(const Framebuffer & src, const Framebuffer & dst, Filter filter) {
	src.bind(Framebuffer::Mode::READ);
	dst.bind(Framebuffer::Mode::WRITE);
	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
	glBlitFramebuffer(0, 0, src.width(), src.height(), 0, 0, dst.width(), dst.height(), GL_COLOR_BUFFER_BIT, filterGL);
	src.unbind();
	dst.unbind();
}

void GLUtilities::blit(const Framebuffer & src, const Framebuffer & dst, size_t lSrc, size_t lDst, Filter filter) {
	GLUtilities::blit(src, dst, lSrc, lDst, 0, 0, filter);
}

void GLUtilities::blit(const Framebuffer & src, const Framebuffer & dst, size_t lSrc, size_t lDst, size_t mipSrc, size_t mipDst, Filter filter) {
	src.bind(lSrc, mipSrc, Framebuffer::Mode::READ);
	dst.bind(lDst, mipDst, Framebuffer::Mode::WRITE);
	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
	glBlitFramebuffer(0, 0, src.width() / (1 << mipSrc), src.height() / (1 << mipSrc), 0, 0, dst.width() / (1 << mipDst), dst.height() / (1 << mipDst), GL_COLOR_BUFFER_BIT, filterGL);
	src.unbind();
	dst.unbind();
}

void GLUtilities::blit(const Texture & src, Texture & dst, Filter filter) {
	// Prepare the destination.
	dst.width  = src.width;
	dst.height = src.height;
	dst.depth  = src.depth;
	dst.levels = 1;
	dst.shape  = src.shape;
	if(src.levels != 1) {
		Log::Warning() << Log::OpenGL << "Only the first mipmap level will be used." << std::endl;
	}
	if(!src.images.empty()) {
		Log::Warning() << Log::OpenGL << "CPU data won't be copied." << std::endl;
	}
	GLUtilities::setupTexture(dst, src.gpu->descriptor());

	// Create two framebuffers.
	GLuint srcFb, dstFb;
	glGenFramebuffers(1, &srcFb);
	glGenFramebuffers(1, &dstFb);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFb);

	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;

	if(src.shape == TextureShape::Cube) {
		for(size_t i = 0; i < 6; ++i) {
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), src.gpu->id, 0);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), dst.gpu->id, 0);
			checkGLFramebufferError();
			glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width, dst.height, GL_COLOR_BUFFER_BIT, filterGL);
		}
	} else {
		if(src.shape == TextureShape::D1) {
			glFramebufferTexture1D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);
			glFramebufferTexture1D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst.gpu->target, dst.gpu->id, 0);

		} else if(src.shape == TextureShape::D2) {
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst.gpu->target, dst.gpu->id, 0);

		} else {
			Log::Error() << Log::OpenGL << "Unsupported texture shape for blitting." << std::endl;
			return;
		}
		checkGLFramebufferError();
		glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width, dst.height, GL_COLOR_BUFFER_BIT, filterGL);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &srcFb);
	glDeleteFramebuffers(1, &dstFb);
}

void GLUtilities::savePixels(GLenum type, GLenum format, unsigned int width, unsigned int height, unsigned int components, const std::string & path, bool flip, bool ignoreAlpha) {

	GLUtilities::sync();

	const bool hdr = type == GL_FLOAT;

	Log::Info() << Log::OpenGL << "Saving framebuffer to file " << path << (hdr ? ".exr" : ".png") << "... " << std::flush;
	int ret;
	Image image(width, height, components);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	const size_t fullSize = image.width * image.height * image.components;
	if(hdr) {
		// Get back values.
		glReadPixels(0, 0, GLsizei(image.width), GLsizei(image.height), format, type, &image.pixels[0]);
		// Save data.
		ret = image.save(path + ".exr", flip, ignoreAlpha);

	} else {
		// Get back values.
		GLubyte * data = new GLubyte[fullSize];
		glReadPixels(0, 0, GLsizei(image.width), GLsizei(image.height), format, type, &data[0]);
		// Convert to image float format.
		for(size_t pid = 0; pid < fullSize; ++pid) {
			image.pixels[pid] = float(data[pid]) / 255.0f;
		}
		// Save data.
		ret = image.save(path + ".png", flip, ignoreAlpha);
		delete[] data;
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	if(ret != 0) {
		Log::Error() << "Error." << std::endl;
	} else {
		Log::Info() << "Done." << std::endl;
	}
}

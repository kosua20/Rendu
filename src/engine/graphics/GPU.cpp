#include "graphics/GPU.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/Texture.hpp"
#include "resources/Image.hpp"
#include "system/TextUtilities.hpp"

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <sstream>
#include <GLFW/glfw3.h>


/** Converts a GLenum error number into a human-readable string.
 \param error the GPU error value
 \return the corresponding string
 */
static std::string getGLErrorString(GLenum error) {
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

void GPU::setup() {
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
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_PROGRAM_POINT_SIZE);
	Framebuffer::backbuffer()->bind();

	// Cache initial state.
	GPU::getState(_state);
	_state.polygonMode = PolygonMode::FILL;

	// Create empty VAO for screenquad.
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);
	glBindVertexArray(0);
	_state.vertexArray = 0;
}

int GPU::checkError(const char * file, int line, const std::string & infos) {
	const GLenum glErr = glGetError();
	if(glErr != GL_NO_ERROR) {
		const std::string filePath(file);
		size_t pos = std::min(filePath.find_last_of('/'), filePath.find_last_of('\\'));
		if(pos == std::string::npos) {
			pos = 0;
		}
		Log::Error() << Log::GPU << "Error " << getGLErrorString(glErr) << " in " << filePath.substr(pos + 1) << " (" << line << ").";
		if(!infos.empty()) {
			Log::Error() << " Infos: " << infos;
		}
		Log::Error() << std::endl;
		return 1;
	}
	return 0;
}

int GPU::checkFramebufferStatus() {
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE) {
		switch(status) {
			case GL_FRAMEBUFFER_UNDEFINED:
				Log::Error() << Log::GPU << "Error GL_FRAMEBUFFER_UNDEFINED" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				Log::Error() << Log::GPU << "Error GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				Log::Error() << Log::GPU << "Error GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				Log::Error() << Log::GPU << "Error GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				Log::Error() << Log::GPU << "Error GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				Log::Error() << Log::GPU << "Error GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
				Log::Error() << Log::GPU << "Error GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << std::endl;
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
				Log::Error() << Log::GPU << "Error GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" << std::endl;
				break;
			default:
				Log::Error() << Log::GPU << "Unknown framebuffer error." << std::endl;
				break;
		}
		return 1;
	}
	return 0;
}

GLuint GPU::loadShader(const std::string & prog, ShaderType type, Bindings & bindings, std::string & finalLog) {
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
			Log::Warning() << Log::GPU << "Inconsistent binding location between linked shaders for \"" << name << "\"." << std::endl;
		}
		bindings[name].location = slot;
		bindings[name].type		= isSampler ? BindingType::TEXTURE : BindingType::UNIFORM_BUFFER;
		Log::Verbose() << Log::GPU << "Detected binding (" << name << ", " << slot << ") => " << outputLines.back() << std::endl;
	}
	// Add OpenGL version.
	std::string outputProg = "#version 400\n#line 1 0\n";
	for(const auto & outputLine : outputLines) {
		outputProg.append(outputLine + "\n");
	}

	// Create shader object.
	static const std::map<ShaderType, GLenum> types = {
		{ShaderType::VERTEX, GL_VERTEX_SHADER},
		{ShaderType::FRAGMENT, GL_FRAGMENT_SHADER},
		{ShaderType::GEOMETRY, GL_GEOMETRY_SHADER},
		{ShaderType::TESSCONTROL, GL_TESS_CONTROL_SHADER},
		{ShaderType::TESSEVAL, GL_TESS_EVALUATION_SHADER}
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

GLuint GPU::createProgram(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent, const std::string & tessEvalContent, Bindings & bindings, const std::string & debugInfos) {
	GLuint vp(0), fp(0), gp(0), tcp(0), tep(0);
	const GLuint id = glCreateProgram();
	checkGLError();

	Log::Verbose() << Log::GPU << "Compiling " << debugInfos << "." << std::endl;

	std::string compilationLog;
	// If vertex program code is given, compile it.
	if(!vertexContent.empty()) {
		vp = loadShader(vertexContent, ShaderType::VERTEX, bindings, compilationLog);
		glAttachShader(id, vp);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Vertex shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If fragment program code is given, compile it.
	if(!fragmentContent.empty()) {
		fp = loadShader(fragmentContent, ShaderType::FRAGMENT, bindings, compilationLog);
		glAttachShader(id, fp);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Fragment shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If geometry program code is given, compile it.
	if(!geometryContent.empty()) {
		gp = loadShader(geometryContent, ShaderType::GEOMETRY, bindings, compilationLog);
		glAttachShader(id, gp);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Geometry shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If tesselation control program code is given, compile it.
	if(!tessControlContent.empty()) {
		tcp = loadShader(tessControlContent, ShaderType::TESSCONTROL, bindings, compilationLog);
		glAttachShader(id, tcp);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Tessellation control shader failed to compile:" << std::endl
						 << compilationLog << std::endl;
		}
	}
	// If tessellation evaluation program code is given, compile it.
	if(!tessEvalContent.empty()) {
		tep = loadShader(tessEvalContent, ShaderType::TESSEVAL, bindings, compilationLog);
		glAttachShader(id, tep);
		if(!compilationLog.empty()) {
			Log::Error() << Log::GPU << "Tessellation evaluation shader failed to compile:" << std::endl
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
		Log::Error() << Log::GPU
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
	if(tcp != 0) {
		glDetachShader(id, tcp);
	}
	if(tep != 0) {
		glDetachShader(id, tep);
	}
	checkGLError();
	//And deleting them
	glDeleteShader(vp);
	glDeleteShader(fp);
	glDeleteShader(gp);
	glDeleteShader(tcp);
	glDeleteShader(tep);

	checkGLError();
	// Return the id to the successfuly linked GLProgram.
	return id;
}

void GPU::bindProgram(const Program & program){
	if(_state.program != program._id){
		_state.program = program._id;
		glUseProgram(program._id);
		_metrics.programBindings += 1;
	}
}

void GPU::bindFramebuffer(const Framebuffer & framebuffer){
	if(_state.drawFramebuffer != framebuffer._id){
		_state.drawFramebuffer = framebuffer._id;
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer._id);
		_metrics.framebufferBindings += 1;
	}
}

void GPU::bindFramebuffer(const Framebuffer & framebuffer, Framebuffer::Mode mode){
	if(mode == Framebuffer::Mode::WRITE && _state.drawFramebuffer != framebuffer._id){
		_state.drawFramebuffer = framebuffer._id;
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer._id);
		_metrics.framebufferBindings += 1;
	} else if(mode == Framebuffer::Mode::READ && _state.readFramebuffer != framebuffer._id){
		_state.readFramebuffer = framebuffer._id;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer._id);
		_metrics.framebufferBindings += 1;
	}

}

void GPU::saveFramebuffer(const Framebuffer & framebuffer, const std::string & path, bool flip, bool ignoreAlpha) {

	// Don't alter the GPU state, this is a temporary action.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer._id);

	const std::unique_ptr<GPUTexture> & gpu = framebuffer.texture()->gpu;
	GPU::savePixels(gpu->type, gpu->format, framebuffer.width(), framebuffer.height(), gpu->channels, path, flip, ignoreAlpha);
	
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _state.readFramebuffer);
	_metrics.framebufferBindings += 2;
}

void GPU::bindTexture(const Texture * texture, size_t slot) {
	auto & currId = _state.textures[slot][texture->gpu->target];
	if(currId != texture->gpu->id){
		currId = texture->gpu->id;
		_state.activeTexture = GLenum(GL_TEXTURE0 + slot);
		glActiveTexture(_state.activeTexture);
		glBindTexture(texture->gpu->target, texture->gpu->id);
		_metrics.textureBindings += 1;
	}
}

void GPU::bindTexture(const Texture & texture, size_t slot) {
	auto & currId = _state.textures[slot][texture.gpu->target];
	if(currId != texture.gpu->id){
		currId = texture.gpu->id;
		_state.activeTexture = GLenum(GL_TEXTURE0 + slot);
		glActiveTexture(_state.activeTexture);
		glBindTexture(texture.gpu->target, texture.gpu->id);
		_metrics.textureBindings += 1;
	}
}

void GPU::bindTextures(const std::vector<const Texture *> & textures, size_t startingSlot) {
	for(size_t i = 0; i < textures.size(); ++i) {
		const Texture * infos = textures[i];
		const int slot = startingSlot + i;
		auto & currId = _state.textures[slot][infos->gpu->target];

		if(currId != infos->gpu->id){
			currId = infos->gpu->id;
			_state.activeTexture = GLenum(GL_TEXTURE0 + slot);
			glActiveTexture(_state.activeTexture);
			glBindTexture(infos->gpu->target, infos->gpu->id);
			_metrics.textureBindings += 1;
		}
	}
}

void GPU::setupTexture(Texture & texture, const Descriptor & descriptor) {

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
	_metrics.textureBindings += 1;

	// Set proper max mipmap level.
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture.levels - 1);
	// Texture settings.
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, texture.gpu->minFiltering);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, texture.gpu->magFiltering);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);

	GPU::restoreTexture(texture.shape);

	// Allocate.
	GPU::allocateTexture(texture);
}

void GPU::allocateTexture(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}

	const GLenum target		= texture.gpu->target;
	const GLenum typeFormat = texture.gpu->typedFormat;
	const GLenum type		= texture.gpu->type;
	const GLenum format		= texture.gpu->format;
	glBindTexture(target, texture.gpu->id);
	_metrics.textureBindings += 1;

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
				Log::Error() << Log::GPU << "Incorrect number of levels in a cubemap (" << texture.depth << ")." << std::endl;
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
				Log::Error() << Log::GPU << "Incorrect number of levels in a cubemap array (" << texture.depth << ")." << std::endl;
				return;
			}
			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, typeFormat, w, h, texture.depth, 0, format, type, nullptr);

		} else {
			Log::Error() << Log::GPU << "Unsupported texture shape." << std::endl;
			return;
		}
	}
	GPU::restoreTexture(texture.shape);
}

void GPU::uploadTexture(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(texture.images.empty()) {
		Log::Warning() << Log::GPU << "No images to upload." << std::endl;
		return;
	}

	const GLenum target		= texture.gpu->target;
	const GLenum destFormat = texture.gpu->format;
	// Sanity check the texture destination format.
	const unsigned int destChannels = texture.gpu->channels;
	if(destChannels != texture.images[0].components) {
		Log::Error() << Log::GPU << "Not enough values in source data for texture upload." << std::endl;
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
	_metrics.stateChanges += 1;
	glBindTexture(target, texture.gpu->id);
	_metrics.textureBindings += 1;

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
				Log::Error() << Log::GPU << "Unsupported texture upload destination." << std::endl;
			}
			_metrics.uploads += 1;
		}
	}
	GPU::restoreTexture(texture.shape);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	_metrics.stateChanges += 1;
}

void GPU::downloadTexture(Texture & texture) {
	downloadTexture(texture, -1);
}

void GPU::downloadTexture(Texture & texture, int level) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}
	if(texture.shape != TextureShape::D2 && texture.shape != TextureShape::Cube) {
		Log::Error() << Log::GPU << "Unsupported download format." << std::endl;
		return;
	}
	if(!texture.images.empty()) {
		Log::Verbose() << Log::GPU << "Texture already contain CPU data, will be erased." << std::endl;
	}
	texture.images.resize(texture.depth * texture.levels);

	const GLenum target			= texture.gpu->target;
	const GLenum type			= GL_FLOAT;
	const GLenum format			= texture.gpu->format;
	const unsigned int channels = texture.gpu->channels;

	// We enforce float type, we can use 4 alignment.
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	_metrics.stateChanges += 1;
	glBindTexture(target, texture.gpu->id);
	_metrics.textureBindings += 1;

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
			_metrics.downloads += 1;
		} else if(texture.shape == TextureShape::Cube) {
			for(size_t lid = 0; lid < texture.depth; ++lid) {
				const size_t id	   = mid * texture.levels + lid;
				texture.images[id] = Image(w, h, channels);
				Image & image	   = texture.images[id];
				glGetTexImage(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + lid), mip, format, type, &image.pixels[0]);
				_metrics.downloads += 1;
			}
		}
	}
	GPU::restoreTexture(texture.shape);
}

void GPU::generateMipMaps(const Texture & texture) {
	if(!texture.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU texture." << std::endl;
		return;
	}
	const GLenum target = texture.gpu->target;
	glBindTexture(target, texture.gpu->id);
	_metrics.textureBindings += 1;
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture.levels - 1);
	glGenerateMipmap(target);
	GPU::restoreTexture(texture.shape);
}

GLenum GPU::targetFromShape(const TextureShape & shape) {

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

void GPU::bindBuffer(const BufferBase & buffer, size_t slot) {
	glBindBuffer(GL_UNIFORM_BUFFER, buffer.gpu->id);
	glBindBufferBase(GL_UNIFORM_BUFFER, GLuint(slot), buffer.gpu->id);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	_metrics.bufferBindings += 2;
	_metrics.uniforms += 1;
}

void GPU::setupBuffer(BufferBase & buffer) {
	if(buffer.gpu) {
		buffer.gpu->clean();
	}
	// Create.
	buffer.gpu.reset(new GPUBuffer(buffer.type, buffer.usage));
	GLuint bufferId;
	glGenBuffers(1, &bufferId);
	buffer.gpu->id = bufferId;
	// Allocate.
	GPU::allocateBuffer(buffer);
}

void GPU::allocateBuffer(const BufferBase & buffer) {
	if(!buffer.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU buffer." << std::endl;
		return;
	}

	const GLenum target = buffer.gpu->target;
	glBindBuffer(target, buffer.gpu->id);
	glBufferData(target, buffer.sizeMax, nullptr, buffer.gpu->usage);
	glBindBuffer(target, 0);
	_metrics.bufferBindings += 2;
}

void GPU::uploadBuffer(const BufferBase & buffer, size_t size, unsigned char * data, size_t offset) {
	if(!buffer.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU buffer." << std::endl;
		return;
	}
	if(size == 0) {
		Log::Warning() << Log::GPU << "No data to upload." << std::endl;
		return;
	}
	if(offset + size > buffer.sizeMax) {
		Log::Warning() << Log::GPU << "Not enough allocated space to upload." << std::endl;
		return;
	}

	const GLenum target = buffer.gpu->target;
	glBindBuffer(target, buffer.gpu->id);
	glBufferSubData(target, offset, size, data);
	_metrics.uploads += 1;
	glBindBuffer(target, 0);
	_metrics.bufferBindings += 2;
}

void GPU::downloadBuffer(const BufferBase & buffer, size_t size, unsigned char * data, size_t offset) {
	if(!buffer.gpu) {
		Log::Error() << Log::GPU << "Uninitialized GPU buffer." << std::endl;
		return;
	}
	if(offset + size > buffer.sizeMax) {
		Log::Warning() << Log::GPU << "Not enough available data to download." << std::endl;
		return;
	}

	const GLenum target = buffer.gpu->target;
	glBindBuffer(target, buffer.gpu->id);
	glGetBufferSubData(target, offset, size, data);
	_metrics.downloads += 1;
	glBindBuffer(target, 0);
	_metrics.bufferBindings += 2;
}

void GPU::setupMesh(Mesh & mesh) {
	if(mesh.gpu) {
		mesh.gpu->clean();
	}
	mesh.gpu.reset(new GPUMesh());
	// Generate a vertex array.
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	_metrics.vertexBindings += 1;

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
	GPU::setupBuffer(vertexBuffer);
	// Fill in subregions.
	size_t offset = 0;
	if(!mesh.positions.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.positions.size();
		GPU::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.positions.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		_metrics.bufferBindings += 1;
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.normals.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.normals.size();
		GPU::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.normals.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		_metrics.bufferBindings += 1;
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.texcoords.empty()) {
		const size_t size = sizeof(GLfloat) * 2 * mesh.texcoords.size();
		GPU::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.texcoords.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		_metrics.bufferBindings += 1;
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.tangents.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.tangents.size();
		GPU::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.tangents.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		_metrics.bufferBindings += 1;
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.binormals.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.binormals.size();
		GPU::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.binormals.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		_metrics.bufferBindings += 1;
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
		offset += size;
	}
	if(!mesh.colors.empty()) {
		const size_t size = sizeof(GLfloat) * 3 * mesh.colors.size();
		GPU::uploadBuffer(vertexBuffer, size, reinterpret_cast<unsigned char *>(mesh.colors.data()), offset);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.gpu->id);
		_metrics.bufferBindings += 1;
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>(offset));
	}

	// We load the indices data
	const size_t inSize = sizeof(unsigned int) * mesh.indices.size();
	BufferBase indexBuffer(inSize, BufferType::INDEX, DataUse::STATIC);
	GPU::setupBuffer(indexBuffer);
	GPU::uploadBuffer(indexBuffer, inSize, reinterpret_cast<unsigned char *>(mesh.indices.data()));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.gpu->id);
	// Restore previously bound vertex array.
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(_state.vertexArray);
	_metrics.vertexBindings += 2;
	_metrics.bufferBindings += 2;

	mesh.gpu->id		   = vao;
	mesh.gpu->count		   = GLsizei(mesh.indices.size());
	mesh.gpu->indexBuffer  = std::move(indexBuffer.gpu);
	mesh.gpu->vertexBuffer = std::move(vertexBuffer.gpu);
}

void GPU::drawMesh(const Mesh & mesh) {
	if(_state.vertexArray != mesh.gpu->id){
		_state.vertexArray = mesh.gpu->id;
		glBindVertexArray(mesh.gpu->id);
		_metrics.vertexBindings += 1;
	}
	glDrawElements(GL_TRIANGLES, mesh.gpu->count, GL_UNSIGNED_INT, static_cast<void *>(nullptr));
	_metrics.drawCalls += 1;
}

void GPU::drawTesselatedMesh(const Mesh & mesh, uint patchSize){
	glPatchParameteri(GL_PATCH_VERTICES, GLint(patchSize));
	if(_state.vertexArray != mesh.gpu->id){
		_state.vertexArray = mesh.gpu->id;
		glBindVertexArray(mesh.gpu->id);
		_metrics.vertexBindings += 1;
	}
	glDrawElements(GL_PATCHES, mesh.gpu->count, GL_UNSIGNED_INT, static_cast<void *>(nullptr));
	_metrics.drawCalls += 1;

}

void GPU::drawQuad(){
	if(_state.vertexArray != _vao){
		_state.vertexArray = _vao;
		glBindVertexArray(_vao);
		_metrics.vertexBindings += 1;
	}
	glDrawArrays(GL_TRIANGLES, 0, 3);
	_metrics.quadCalls += 1;
}

void GPU::sync(){
	glFlush();
	glFinish();
}

void GPU::nextFrame(){
	// Save and reset stats.
	_metricsPrevious = _metrics;
	_metrics = Metrics();
}

void GPU::deviceInfos(std::string & vendor, std::string & renderer, std::string & version, std::string & shaderVersion) {
	const GLubyte * vendorString	  = glGetString(GL_VENDOR);
	const GLubyte * rendererString	  = glGetString(GL_RENDERER);
	const GLubyte * versionString	  = glGetString(GL_VERSION);
	const GLubyte * glslVersionString = glGetString(GL_SHADING_LANGUAGE_VERSION);
	vendor							  = std::string(reinterpret_cast<const char *>(vendorString));
	renderer						  = std::string(reinterpret_cast<const char *>(rendererString));
	version							  = std::string(reinterpret_cast<const char *>(versionString));
	shaderVersion					  = std::string(reinterpret_cast<const char *>(glslVersionString));
}

std::vector<std::string> GPU::deviceExtensions() {
	int extensionCount = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
	std::vector<std::string> extensions(extensionCount);

	for(int i = 0; i < extensionCount; ++i) {
		const GLubyte * extStr = glGetStringi(GL_EXTENSIONS, i);
		extensions[i]		   = std::string(reinterpret_cast<const char *>(extStr));
	}
	return extensions;
}

void GPU::setViewport(int x, int y, int w, int h) {
	if(_state.viewport[0] != x || _state.viewport[1] != y || _state.viewport[2] != w || _state.viewport[3] != h){
		_state.viewport[0] = x;
		_state.viewport[1] = y;
		_state.viewport[2] = w;
		_state.viewport[3] = h;
		glViewport(GLsizei(x), GLsizei(y), GLsizei(w), GLsizei(h));
		_metrics.stateChanges += 1;
	}
}

void GPU::clearColor(const glm::vec4 & color) {
	if(_state.colorClearValue != color){
		_state.colorClearValue = color;
		glClearColor(color[0], color[1], color[2], color[3]);
		_metrics.stateChanges += 1;
	}
	glClear(GL_COLOR_BUFFER_BIT);
	_metrics.clearAndBlits += 1;
}

void GPU::clearDepth(float depth) {
	if(_state.depthClearValue != depth){
		_state.depthClearValue = depth;
		glClearDepth(depth);
		_metrics.stateChanges += 1;
	}
	glClear(GL_DEPTH_BUFFER_BIT);
	_metrics.clearAndBlits += 1;
}

void GPU::clearStencil(uchar stencil) {
	// The stencil mask applies to clearing.
	// Disable it temporarily.
	if(!_state.stencilWriteMask){
		glStencilMask(0xFF);
		_metrics.stateChanges += 1;
	}

	if(_state.stencilClearValue != stencil){
		_state.stencilClearValue = stencil;
		glClearStencil(GLint(stencil));
		_metrics.stateChanges += 1;
	}
	glClear(GL_STENCIL_BUFFER_BIT);
	_metrics.clearAndBlits += 1;

	if(!_state.stencilWriteMask){
		glStencilMask(0x00);
		_metrics.stateChanges += 1;
	}
}

void GPU::clearColorAndDepth(const glm::vec4 & color, float depth) {
	if(_state.colorClearValue != color){
		_state.colorClearValue = color;
		glClearColor(color[0], color[1], color[2], color[3]);
		_metrics.stateChanges += 1;
	}
	if(_state.depthClearValue != depth){
		_state.depthClearValue = depth;
		glClearDepth(depth);
		_metrics.stateChanges += 1;
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	_metrics.clearAndBlits += 1;
}

void GPU::clearColorDepthStencil(const glm::vec4 & color, float depth, uchar stencil) {
	// The stencil mask applies to clearing.
	// Disable it temporarily.
	if(!_state.stencilWriteMask){
		glStencilMask(0xFF);
		_metrics.stateChanges += 1;
	}
	if(_state.colorClearValue != color){
		_state.colorClearValue = color;
		glClearColor(color[0], color[1], color[2], color[3]);
		_metrics.stateChanges += 1;
	}
	if(_state.depthClearValue != depth){
		_state.depthClearValue = depth;
		glClearDepth(depth);
		_metrics.stateChanges += 1;
	}
	if(_state.stencilClearValue != stencil){
		_state.stencilClearValue = stencil;
		glClearStencil(GLint(stencil));
		_metrics.stateChanges += 1;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	_metrics.clearAndBlits += 1;

	if(!_state.stencilWriteMask){
		glStencilMask(0x00);
		_metrics.stateChanges += 1;
	}
}

void GPU::setDepthState(bool test) {
	if(_state.depthTest != test){
		_state.depthTest = test;
		(test ? glEnable : glDisable)(GL_DEPTH_TEST);
		_metrics.stateChanges += 1;
	}
}

void GPU::setDepthState(bool test, TestFunction equation, bool write) {
	if(_state.depthTest != test){
		_state.depthTest = test;
		(test ? glEnable : glDisable)(GL_DEPTH_TEST);
		_metrics.stateChanges += 1;
	}

	static const std::map<TestFunction, GLenum> eqs = {
		{TestFunction::NEVER, GL_NEVER},
		{TestFunction::LESS, GL_LESS},
		{TestFunction::LEQUAL, GL_LEQUAL},
		{TestFunction::EQUAL, GL_EQUAL},
		{TestFunction::GREATER, GL_GREATER},
		{TestFunction::GEQUAL, GL_GEQUAL},
		{TestFunction::NOTEQUAL, GL_NOTEQUAL},
		{TestFunction::ALWAYS, GL_ALWAYS}};

	if(_state.depthFunc != equation){
		_state.depthFunc = equation;
		glDepthFunc(eqs.at(equation));
		_metrics.stateChanges += 1;
	}

	if(_state.depthWriteMask != write){
		_state.depthWriteMask = write;
		glDepthMask(write ? GL_TRUE : GL_FALSE);
		_metrics.stateChanges += 1;
	}
}

void GPU::setStencilState(bool test, bool write){
	if(_state.stencilTest != test){
		_state.stencilTest = test;
		(test ? glEnable : glDisable)(GL_STENCIL_TEST);
		_metrics.stateChanges += 1;
	}
	if(_state.stencilWriteMask != write){
		_state.stencilWriteMask = write;
		glStencilMask(write ? 0xFF : 0x00);
		_metrics.stateChanges += 1;
	}
}

void GPU::setStencilState(bool test, TestFunction function, StencilOp fail, StencilOp pass, StencilOp depthFail, uchar value){

	if(_state.stencilTest != test){
		_state.stencilTest = test;
		(test ? glEnable : glDisable)(GL_STENCIL_TEST);
		_metrics.stateChanges += 1;
	}

	static const std::map<TestFunction, GLenum> funs = {
		{TestFunction::NEVER, GL_NEVER},
		{TestFunction::LESS, GL_LESS},
		{TestFunction::LEQUAL, GL_LEQUAL},
		{TestFunction::EQUAL, GL_EQUAL},
		{TestFunction::GREATER, GL_GREATER},
		{TestFunction::GEQUAL, GL_GEQUAL},
		{TestFunction::NOTEQUAL, GL_NOTEQUAL},
		{TestFunction::ALWAYS, GL_ALWAYS}};

	static const std::map<StencilOp, GLenum> ops = {
		{ StencilOp::KEEP, GL_KEEP },
		{ StencilOp::ZERO, GL_ZERO },
		{ StencilOp::REPLACE, GL_REPLACE },
		{ StencilOp::INCR, GL_INCR },
		{ StencilOp::INCRWRAP, GL_INCR_WRAP },
		{ StencilOp::DECR, GL_DECR },
		{ StencilOp::DECRWRAP, GL_DECR_WRAP },
		{ StencilOp::INVERT, GL_INVERT }};

	if(_state.stencilFunc != function){
		_state.stencilFunc = function;
		glStencilFunc(funs.at(function), GLint(value), 0xFF);
		_metrics.stateChanges += 1;
	}
	if(!_state.stencilWriteMask){
		_state.stencilWriteMask = true;
		glStencilMask(0xFF);
		_metrics.stateChanges += 1;
	}
	if(_state.stencilFail != fail || _state.stencilPass != depthFail || _state.stencilDepthPass != pass){
		_state.stencilFail = fail;
		_state.stencilPass = depthFail;
		_state.stencilDepthPass = pass;
		glStencilOp(ops.at(fail), ops.at(depthFail), ops.at(pass));
		_metrics.stateChanges += 1;
	}
}

void GPU::setBlendState(bool test) {
	if(_state.blend != test){
		_state.blend = test;
		(test ? glEnable : glDisable)(GL_BLEND);
		_metrics.stateChanges += 1;
	}
}

void GPU::setBlendState(bool test, BlendEquation equation, BlendFunction src, BlendFunction dst) {

	if(_state.blend != test){
		_state.blend = test;
		(test ? glEnable : glDisable)(GL_BLEND);
		_metrics.stateChanges += 1;
	}

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

	if(_state.blendEquationRGB != equation){
		_state.blendEquationRGB = _state.blendEquationAlpha = equation;
		glBlendEquation(eqs.at(equation));
		_metrics.stateChanges += 1;
	}

	if(_state.blendSrcRGB != src || _state.blendDstRGB != dst){
		_state.blendSrcRGB = _state.blendSrcAlpha = src;
		_state.blendDstRGB = _state.blendDstAlpha = dst;
		glBlendFunc(funcs.at(src), funcs.at(dst));
		_metrics.stateChanges += 1;
	}

}

void GPU::setCullState(bool cull) {
	if(_state.cullFace != cull){
		_state.cullFace = cull;
		(cull ? glEnable : glDisable)(GL_CULL_FACE);
		_metrics.stateChanges += 1;
	}
}

void GPU::setCullState(bool cull, Faces culledFaces) {
	if(_state.cullFace != cull){
		_state.cullFace = cull;
		(cull ? glEnable : glDisable)(GL_CULL_FACE);
		_metrics.stateChanges += 1;
	}

	static const std::map<Faces, GLenum> faces = {
	 {Faces::FRONT, GL_FRONT},
	 {Faces::BACK, GL_BACK},
	 {Faces::ALL, GL_FRONT_AND_BACK}};

	if(_state.cullFaceMode != culledFaces){
		_state.cullFaceMode = culledFaces;
		glCullFace(faces.at(culledFaces));
		_metrics.stateChanges += 1;
	}
}

void GPU::setPolygonState(PolygonMode mode) {
	
	static const std::map<PolygonMode, GLenum> modes = {
		{PolygonMode::FILL, GL_FILL},
		{PolygonMode::LINE, GL_LINE},
		{PolygonMode::POINT, GL_POINT}};

	if(_state.polygonMode != mode){
		_state.polygonMode = mode;
		glPolygonMode(GL_FRONT_AND_BACK, modes.at(mode));
		_metrics.stateChanges += 1;
	}
}

void GPU::setColorState(bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha){
	if(_state.colorWriteMask.r != writeRed || _state.colorWriteMask.g != writeGreen || _state.colorWriteMask.b != writeBlue || _state.colorWriteMask.a != writeAlpha){
		_state.colorWriteMask.r = writeRed;
		_state.colorWriteMask.g = writeGreen;
		_state.colorWriteMask.b = writeBlue;
		_state.colorWriteMask.a = writeAlpha;
		glColorMask(writeRed ? GL_TRUE : GL_FALSE, writeGreen ? GL_TRUE : GL_FALSE, writeBlue ? GL_TRUE : GL_FALSE, writeAlpha ? GL_TRUE : GL_FALSE);
		_metrics.stateChanges += 1;
	}

}
void GPU::setSRGBState(bool convert){
	if(_state.framebufferSRGB != convert){
		_state.framebufferSRGB = convert;
		(convert ? glEnable : glDisable)(GL_FRAMEBUFFER_SRGB);
		_metrics.stateChanges += 1;
	}
}

void GPU::blitDepth(const Framebuffer & src, const Framebuffer & dst) {
	src.bind(Framebuffer::Mode::READ);
	dst.bind(Framebuffer::Mode::WRITE);
	glBlitFramebuffer(0, 0, src.width(), src.height(), 0, 0, dst.width(), dst.height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	_metrics.clearAndBlits += 1;
}

void GPU::blit(const Framebuffer & src, const Framebuffer & dst, Filter filter) {
	src.bind(Framebuffer::Mode::READ);
	dst.bind(Framebuffer::Mode::WRITE);
	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
	glBlitFramebuffer(0, 0, src.width(), src.height(), 0, 0, dst.width(), dst.height(), GL_COLOR_BUFFER_BIT, filterGL);
	_metrics.clearAndBlits += 1;
}

void GPU::blit(const Framebuffer & src, const Framebuffer & dst, size_t lSrc, size_t lDst, Filter filter) {
	GPU::blit(src, dst, lSrc, lDst, 0, 0, filter);
}

void GPU::blit(const Framebuffer & src, const Framebuffer & dst, size_t lSrc, size_t lDst, size_t mipSrc, size_t mipDst, Filter filter) {
	src.bind(lSrc, mipSrc, Framebuffer::Mode::READ);
	dst.bind(lDst, mipDst, Framebuffer::Mode::WRITE);
	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;
	glBlitFramebuffer(0, 0, src.width() / (1 << mipSrc), src.height() / (1 << mipSrc), 0, 0, dst.width() / (1 << mipDst), dst.height() / (1 << mipDst), GL_COLOR_BUFFER_BIT, filterGL);
	_metrics.clearAndBlits += 1;
}

void GPU::blit(const Texture & src, Texture & dst, Filter filter) {
	// Prepare the destination.
	dst.width  = src.width;
	dst.height = src.height;
	dst.depth  = src.depth;
	dst.levels = 1;
	dst.shape  = src.shape;
	if(src.levels != 1) {
		Log::Warning() << Log::GPU << "Only the first mipmap level will be used." << std::endl;
	}
	if(!src.images.empty()) {
		Log::Warning() << Log::GPU << "CPU data won't be copied." << std::endl;
	}
	GPU::setupTexture(dst, src.gpu->descriptor());

	// Create two framebuffers.
	GLuint srcFb, dstFb;
	glGenFramebuffers(1, &srcFb);
	glGenFramebuffers(1, &dstFb);
	// Because these two are temporary and will be unbound at the end of the call
	// we do not update the cached GPU state.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFb);
	_metrics.framebufferBindings += 2;

	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;

	if(src.shape == TextureShape::Cube) {
		for(size_t i = 0; i < 6; ++i) {
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), src.gpu->id, 0);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), dst.gpu->id, 0);
			GPU::checkFramebufferStatus();
			glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width, dst.height, GL_COLOR_BUFFER_BIT, filterGL);
			_metrics.clearAndBlits += 1;
		}
	} else {
		if(src.shape == TextureShape::D1) {
			glFramebufferTexture1D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);
			glFramebufferTexture1D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst.gpu->target, dst.gpu->id, 0);

		} else if(src.shape == TextureShape::D2) {
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst.gpu->target, dst.gpu->id, 0);

		} else {
			Log::Error() << Log::GPU << "Unsupported texture shape for blitting." << std::endl;
			return;
		}
		GPU::checkFramebufferStatus();
		glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width, dst.height, GL_COLOR_BUFFER_BIT, filterGL);
		_metrics.clearAndBlits += 1;
	}
	// Restore the proper framebuffers from the cache.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _state.readFramebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _state.drawFramebuffer);
	_metrics.framebufferBindings += 2;
	glDeleteFramebuffers(1, &srcFb);
	glDeleteFramebuffers(1, &dstFb);
}

void GPU::blit(const Texture & src, Framebuffer & dst, Filter filter) {
	// Prepare the destination.
	if(src.levels != 1) {
		Log::Warning() << Log::GPU << "Only the first mipmap level will be used." << std::endl;
	}
	if(src.shape != dst.shape()){
		Log::Error() << Log::GPU << "The texture and framebuffer don't have the same shape." << std::endl;
		return;
	}

	// Create one framebuffer.
	GLuint srcFb;
	glGenFramebuffers(1, &srcFb);
	// Because it's temporary and will be unbound at the end of the call
	// we do not update the cached GPU state.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFb);
	_metrics.framebufferBindings += 1;
	const GLenum filterGL = filter == Filter::LINEAR ? GL_LINEAR : GL_NEAREST;

	if(src.shape == TextureShape::Cube) {
		for(size_t i = 0; i < 6; ++i) {
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i), src.gpu->id, 0);
			GPU::checkFramebufferStatus();
			dst.bind(i, 0, Framebuffer::Mode::WRITE);
			glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width(), dst.height(), GL_COLOR_BUFFER_BIT, filterGL);
			_metrics.clearAndBlits += 1;
		}
	} else {
		if(src.shape == TextureShape::D1) {
			glFramebufferTexture1D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);

		} else if(src.shape == TextureShape::D2) {
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src.gpu->target, src.gpu->id, 0);

		} else {
			Log::Error() << Log::GPU << "Unsupported texture shape for blitting." << std::endl;
			return;
		}
		GPU::checkFramebufferStatus();
		dst.bind(0, 0, Framebuffer::Mode::WRITE);
		glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dst.width(), dst.height(), GL_COLOR_BUFFER_BIT, filterGL);
		_metrics.clearAndBlits += 1;
	}
	// Restore the proper framebuffer from the cache.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _state.readFramebuffer);
	_metrics.framebufferBindings += 1;
	glDeleteFramebuffers(1, &srcFb);
}

void GPU::savePixels(GLenum type, GLenum format, unsigned int width, unsigned int height, unsigned int components, const std::string & path, bool flip, bool ignoreAlpha) {

	GPU::sync();

	const bool hdr = type == GL_FLOAT;

	Log::Info() << Log::GPU << "Saving framebuffer to file " << path << (hdr ? ".exr" : ".png") << "... " << std::flush;
	int ret;
	Image image(width, height, components);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	_metrics.stateChanges += 1;
	const size_t fullSize = image.width * image.height * image.components;
	if(hdr) {
		// Get back values.
		glReadPixels(0, 0, GLsizei(image.width), GLsizei(image.height), format, type, &image.pixels[0]);
		_metrics.downloads += 1;
		// Save data.
		ret = image.save(path + ".exr", flip, ignoreAlpha);

	} else {
		// Get back values.
		GLubyte * data = new GLubyte[fullSize];
		glReadPixels(0, 0, GLsizei(image.width), GLsizei(image.height), format, type, &data[0]);
		_metrics.downloads += 1;
		// Convert to image float format.
		for(size_t pid = 0; pid < fullSize; ++pid) {
			image.pixels[pid] = float(data[pid]) / 255.0f;
		}
		// Save data.
		ret = image.save(path + ".png", flip, ignoreAlpha);
		delete[] data;
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	_metrics.stateChanges += 1;

	if(ret != 0) {
		Log::Error() << "Error." << std::endl;
	} else {
		Log::Info() << "Done." << std::endl;
	}
}

void GPU::getState(GPUState& state) {
	
	// Boolean flags.
	state.blend = glIsEnabled(GL_BLEND);
	state.cullFace = glIsEnabled(GL_CULL_FACE);
	state.depthClamp = glIsEnabled(GL_DEPTH_CLAMP);
	state.depthTest = glIsEnabled(GL_DEPTH_TEST);
	state.framebufferSRGB = glIsEnabled(GL_FRAMEBUFFER_SRGB);
	state.polygonOffsetFill = glIsEnabled(GL_POLYGON_OFFSET_FILL);
	state.polygonOffsetLine = glIsEnabled(GL_POLYGON_OFFSET_LINE);
	state.polygonOffsetPoint = glIsEnabled(GL_POLYGON_OFFSET_POINT);
	state.scissorTest = glIsEnabled(GL_SCISSOR_TEST);
	state.stencilTest = glIsEnabled(GL_STENCIL_TEST);

	// Blend state.
	static const std::map<GLenum, BlendEquation> blendEqs = {
		{GL_FUNC_ADD, BlendEquation::ADD},
		{GL_FUNC_SUBTRACT, BlendEquation::SUBTRACT},
		{GL_FUNC_REVERSE_SUBTRACT, BlendEquation::REVERSE_SUBTRACT},
		{GL_MIN, BlendEquation::MIN},
		{GL_MAX, BlendEquation::MAX}};
	GLint ber, bea;
	glGetIntegerv(GL_BLEND_EQUATION_RGB, &ber);
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &bea);
	state.blendEquationRGB = blendEqs.at(ber);
	state.blendEquationAlpha = blendEqs.at(bea);

	static const std::map<GLenum, BlendFunction> funcs = {
		{GL_ONE, BlendFunction::ONE},
		{GL_ZERO, BlendFunction::ZERO},
		{GL_SRC_COLOR, BlendFunction::SRC_COLOR},
		{GL_ONE_MINUS_SRC_COLOR, BlendFunction::ONE_MINUS_SRC_COLOR},
		{GL_SRC_ALPHA, BlendFunction::SRC_ALPHA},
		{GL_ONE_MINUS_SRC_ALPHA, BlendFunction::ONE_MINUS_SRC_ALPHA},
		{GL_DST_COLOR, BlendFunction::DST_COLOR},
		{GL_ONE_MINUS_DST_COLOR, BlendFunction::ONE_MINUS_DST_COLOR},
		{GL_DST_ALPHA, BlendFunction::DST_ALPHA},
		{GL_ONE_MINUS_DST_ALPHA, BlendFunction::ONE_MINUS_DST_ALPHA}};
	GLint bsr, bsa, bdr, bda;
	glGetIntegerv(GL_BLEND_SRC_RGB, &bsr);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &bsa);
	glGetIntegerv(GL_BLEND_DST_RGB, &bdr);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &bda);
	state.blendSrcRGB = funcs.at(bsr);
	state.blendSrcAlpha = funcs.at(bsa);
	state.blendDstRGB = funcs.at(bdr);
	state.blendDstAlpha = funcs.at(bda);
	glGetFloatv(GL_BLEND_COLOR, &state.blendColor[0]);
	
	// Color state.
	glGetFloatv(GL_COLOR_CLEAR_VALUE, &state.colorClearValue[0]);
	GLboolean cwm[4];
	glGetBooleanv(GL_COLOR_WRITEMASK, &cwm[0]);
	state.colorWriteMask = glm::bvec4(cwm[0], cwm[1], cwm[2], cwm[3]);

	// Geometry state.
	static const std::map<GLenum, Faces> faces = {
		{GL_FRONT, Faces::FRONT},
		{GL_BACK, Faces::BACK},
		{GL_FRONT_AND_BACK, Faces::ALL}};
	GLint cfm;
	glGetIntegerv(GL_CULL_FACE_MODE, &cfm);
	state.cullFaceMode = faces.at(cfm);
	glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &state.polygonOffsetFactor);
	glGetFloatv(GL_POLYGON_OFFSET_UNITS, &state.polygonOffsetUnits);

	// Depth state.
	static const std::map<GLenum, TestFunction> testFuncs = {
		{GL_NEVER, 		TestFunction::NEVER},
		{GL_LESS, 		TestFunction::LESS},
		{GL_LEQUAL, 	TestFunction::LEQUAL},
		{GL_EQUAL, 		TestFunction::EQUAL},
		{GL_GREATER, 	TestFunction::GREATER},
		{GL_GEQUAL, 	TestFunction::GEQUAL},
		{GL_NOTEQUAL, 	TestFunction::NOTEQUAL},
		{GL_ALWAYS, 	TestFunction::ALWAYS}};
	GLint dfc;
	glGetIntegerv(GL_DEPTH_FUNC, &dfc);
	state.depthFunc = testFuncs.at(dfc);
	glGetFloatv(GL_DEPTH_CLEAR_VALUE, &state.depthClearValue);
	glGetFloatv(GL_DEPTH_RANGE, &state.depthRange[0]);
	GLboolean dwm;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &dwm);
	state.depthWriteMask = dwm;

	// Stencil state
	static const std::map<GLenum, StencilOp> ops = {
		{ GL_KEEP, StencilOp::KEEP },
		{ GL_ZERO, StencilOp::ZERO },
		{ GL_REPLACE, StencilOp::REPLACE },
		{ GL_INCR, StencilOp::INCR },
		{ GL_INCR_WRAP, StencilOp::INCRWRAP },
		{ GL_DECR, StencilOp::DECR },
		{ GL_DECR_WRAP, StencilOp::DECRWRAP },
		{ GL_INVERT, StencilOp::INVERT }};
	GLint sfc, sof, sos, sod;
	glGetIntegerv(GL_STENCIL_FUNC, &sfc);
	glGetIntegerv(GL_STENCIL_FAIL, &sof);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &sos);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &sod);
	state.stencilFunc = testFuncs.at(sfc);
	state.stencilFail = ops.at(sof);
	state.stencilPass = ops.at(sos);
	state.stencilDepthPass = ops.at(sod);
	GLint swm, scv, srv;
	glGetIntegerv(GL_STENCIL_WRITEMASK, &swm);
	glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &scv);
	glGetIntegerv(GL_STENCIL_REF, &srv);
	state.stencilWriteMask = (swm != 0);
	state.stencilValue = uchar(srv);
	state.stencilClearValue = uchar(scv);

	// Viewport and scissor state.
	glGetFloatv(GL_VIEWPORT, &state.viewport[0]);
	glGetFloatv(GL_SCISSOR_BOX, &state.scissorBox[0]);

	// Binding state.
	GLint fbr, fbd, pgb, ats, vab;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &fbr);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fbd);
	glGetIntegerv(GL_CURRENT_PROGRAM, &pgb);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &ats);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vab);

	state.readFramebuffer = fbr;
	state.drawFramebuffer = fbd;
	state.program = pgb;
	state.activeTexture = GLenum(ats);
	state.vertexArray = vab;

	static const std::vector<GLenum> bindings = {
		GL_TEXTURE_BINDING_1D, GL_TEXTURE_BINDING_2D,
		GL_TEXTURE_BINDING_3D, GL_TEXTURE_BINDING_CUBE_MAP,
		GL_TEXTURE_BINDING_1D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY,
		GL_TEXTURE_BINDING_CUBE_MAP_ARRAY,
	};
	static const std::vector<GLenum> shapes = {
		GL_TEXTURE_1D, GL_TEXTURE_2D,
		GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP,
		GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY,
		GL_TEXTURE_CUBE_MAP_ARRAY,
	};
	for(size_t tid = 0; tid < state.textures.size(); ++tid){
		glActiveTexture(GLenum(GL_TEXTURE0 + tid));
		for(size_t bid = 0; bid < bindings.size(); ++bid){
			GLint texId = 0;
			glGetIntegerv(bindings[bid], &texId);
			state.textures[tid][shapes[bid]] = GLuint(texId);
		}
	}
	glActiveTexture(state.activeTexture);
}


const GPU::Metrics & GPU::getMetrics(){
	return _metricsPrevious;
}

void GPU::restoreTexture(TextureShape shape){
	const GLenum target = GPU::targetFromShape(shape);
	const int slot = _state.activeTexture - int(GL_TEXTURE0);
	glBindTexture(target, _state.textures[slot][target]);
	_metrics.textureBindings += 1;
}

void GPU::deleted(GPUTexture & tex){
	// If any active slot is using it, set it to 0.
	for(auto& bind : _state.textures){
		if(bind[tex.target] == tex.id){
			bind[tex.target] = 0;
		}
	}
}

void GPU::deleted(Framebuffer & framebuffer){
	if(_state.drawFramebuffer == framebuffer._id){
		_state.drawFramebuffer = 0;
	}
	if(_state.readFramebuffer == framebuffer._id){
		_state.readFramebuffer = 0;
	}
}

void GPU::deleted(GPUMesh & mesh){
	if(_state.vertexArray == mesh.id){
		_state.vertexArray = 0;
	}
}

GPUState GPU::_state;
GPU::Metrics GPU::_metrics;
GPU::Metrics GPU::_metricsPrevious;
GLuint GPU::_vao = 0;

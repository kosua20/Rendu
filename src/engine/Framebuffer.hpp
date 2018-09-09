#ifndef Framebuffer_h
#define Framebuffer_h

#include "Common.hpp"


/**
 \brief Represent a 2D rendering target, of any size, format and type, backed by an OpenGL framebuffer.
 \ingroup Engine
 */
class Framebuffer {
public:
	
	/** \brief Regroups format, type, filtering and wrapping informations for a color buffer. */
	struct Descriptor {
		
		GLuint typedFormat; ///< The precise typed format.
		GLuint filtering; ///< Filtering mode.
		GLuint wrapping; ///< Wrapping mode.
		
		/** Default constructor. RGB8, linear, clamp. */
		Descriptor(){
			typedFormat = GL_RGB8; filtering = GL_LINEAR; wrapping = GL_CLAMP_TO_EDGE;
		}
		/** Convenience constructor. Custom typed format, linear, clamp.
		 \param typedFormat_ the precise typed format to use
		*/
		Descriptor(const GLuint typedFormat_){
			typedFormat = typedFormat_; filtering = GL_LINEAR; wrapping = GL_CLAMP_TO_EDGE;
		}
		
		/** Constructor.
		\param typedFormat_ the precise typed format to use
		\param filtering_ the texture filtering (GL_LINEAR,...) to use
		\param wrapping_ the texture wrapping mode (GL_CLAMP_TO_EDGE) to use
		 */
		Descriptor(const GLuint typedFormat_, const GLuint filtering_, const GLuint wrapping_){
			typedFormat = typedFormat_; filtering = filtering_; wrapping = wrapping_;
		}
	};
	
public:
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param typedFormat the precise typed format, combining format and type (GL_RGB8,...) to use
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	Framebuffer(unsigned int width, unsigned int height, const GLenum typedFormat, bool depthBuffer);
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param descriptor the color attachment texture descriptor (format, filtering,...)
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	Framebuffer(unsigned int width, unsigned int height, const Descriptor & descriptor, bool depthBuffer);
	
	/**
	 Bind the framebuffer.
	 */
	void bind() const;

	/**
	 Set the viewport to the size of the framebuffer.
	 */
	void setViewport() const;

	/**
	 Unbind the framebuffer.
	 \note Technically bind the window backbuffer.
	 */
	void unbind() const;
	
	/**
	 Resize the framebuffer to new dimensions.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	/**
	 Resize the framebuffer to new dimensions.
	 \param size the new size
	 */
	void resize(glm::vec2 size);
	
	/** Clean internal resources.
	 */
	void clean() const;
	
	/**
	 Query the ID of the 2D texture backing the framebuffer.
	 \return the texture ID
	 */
	const GLuint textureId() const { return _idColor; }
	
	/**
	 Query the framebuffer width.
	 \return the width
	 */
	const unsigned int width() const { return _width; }
	
	/**
	 Query the framebuffer height.
	 \return the height
	 */
	const unsigned int height() const { return _height; }
	
	/**
	 Query the framebuffer ID.
	 \return the ID
	 */
	const GLuint id() const { return _id; }
	
	/**
	 Query the framebuffer OpenGL type and format.
	 \return the typed format
	 */
	const GLuint typedFormat() const { return _descriptor.typedFormat; }
	
private:
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param descriptors the color attachments texture descriptors (format, filtering,...)
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	Framebuffer(unsigned int width, unsigned int height, const std::vector<Descriptor> & descriptors, bool depthBuffer);
	
	unsigned int _width; ///< The framebuffer width.
	unsigned int _height; ///< The framebuffer height.
	
	GLuint _id; ///< The framebuffer ID.
	GLuint _idColor; ///< The color texture ID.
	GLuint _idRenderbuffer; ///< The depth renderbuffer ID.
	
	Descriptor _descriptor;

	bool _useDepth; ///< Denotes if the framebuffer is backed by a depth renderbuffer.
	
};

#endif

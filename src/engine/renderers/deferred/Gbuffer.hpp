#ifndef Gbuffer_h
#define Gbuffer_h
#include "../../Common.hpp"
#include <map>

/**
 \brief Available G-buffer layers.
 \ingroup DeferredRendering
 */
enum class TextureType {
	Albedo, ///< (or base color)
	Normal,
	Depth,
	Effects ///< Roughness, metallicness, ambient occlusion factor, ID.
};

/**
 \brief Extended framebuffer with multiple color attachments containing albedo, normals, depth, and combined effects informations.
 \ingroup DeferredRendering
 */
class Gbuffer {

public:
	
	/** Constructor.
	 \param width the rendering width
	 \param height the rendering height
	 */
	Gbuffer(unsigned int width, unsigned int height);
	
	/**
	 Bind the g-buffer.
	 */
	void bind() const;
	
	/**
	 Unbind the g-buffer.
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
	
	/** Clean internal resources. */
	void clean() const;
	
	/**
	 Query the ID of one of the 2D textures of the g-buffer.
	 \param type the queried layer
	 \return the texture ID
	 */
	const GLuint textureId(const TextureType& type) { return _textureIds[type]; }
	
	/**
	 Query the ID of some of the 2D textures of the g-buffer.
	 \param included a list of requested layers
	 \return the textures IDs
	 \note if included is empty, all texture IDs are returned.
	 */
	const std::vector<GLuint> textureIds(const std::vector<TextureType>& included = std::vector<TextureType>()) const ;
	
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
	
private:
	
	unsigned int _width; ///< The framebuffer width.
	unsigned int _height; ///< The framebuffer height.
	
	GLuint _id; ///< The framebuffer ID.
	std::map<TextureType, GLuint> _textureIds; ///< The g-buffer textures IDs.
};

#endif

#pragma once

#include "graphics/Framebuffer.hpp"
#include "graphics/ScreenQuad.hpp"

#include "Common.hpp"

/**
 \brief Utility presenting a canvas to the user, along with a brush tool to paint on it using different shapes.
 \ingroup ImageFiltering
 */
class PaintingTool {

public:
	/** Constructor.
	 \param width the canvas width
	 \param height the canvas height
	 */
	PaintingTool(unsigned int width, unsigned int height);

	/** Draw the scene and effects */
	void draw();

	/** Perform once-per-frame update (buttons, GUI,...) */
	void update();

	/** Clean internal resources. */
	void clean() const;

	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height) const;

	/** Canvas content texture.
	 \return the canvas ID
	 */
	const Texture * textureId() const { return _canvas->textureId(); }

	/** Texture containing the canvas and the brush shape outline, for visualisation.
	 \return the texture ID
	 */
	const Texture * visuId() const { return _visu->textureId(); }

private:
	/** \brief The effect of a brush stroke. */
	enum class Mode : int {
		DRAW = 0,
		ERASE
	};

	/** \brief The shape of the brush. */
	enum class Shape : int {
		CIRCLE = 0,
		SQUARE,
		DIAMOND,
		COUNT
	};

	std::unique_ptr<Framebuffer> _canvas; ///< Scene rendering buffer.
	std::unique_ptr<Framebuffer> _visu;   ///< Scene rendering buffer.

	const Program * _brushShader; ///< Program for the brush and its outline.
	std::vector<Mesh> _brushes;   ///< Brush shape geometries.

	glm::vec3 _bgColor = glm::vec3(0.0f); ///< Canvas color.
	glm::vec3 _fgColor = glm::vec3(1.0f); ///< Brush color.
	glm::vec2 _drawPos = glm::vec2(0.0f); ///< Current brush position.
	int _radius		   = 40;			  ///< Brush radius, in pixels.
	Mode _mode		   = Mode::DRAW;	  ///< Current brush effect.
	Shape _shape	   = Shape::CIRCLE;   ///< Current brush shape.
	bool _shoudClear   = true;			  ///< Clear the canvas at the next frame.
	bool _shouldDraw   = false;			  ///< Apply the brush to the canvas at the next frame.
};

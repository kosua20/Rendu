#pragma once

#include "processing/PoissonFiller.hpp"
#include "processing/LaplacianIntegrator.hpp"
#include "processing/GaussianBlur.hpp"
#include "processing/BoxBlur.hpp"

#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"
#include "graphics/ScreenQuad.hpp"
#include "Config.hpp"

#include "renderers/Renderer.hpp"

#include "Common.hpp"

/**
 \brief Apply multiple image filters on an interactive rendering of a scene.
 Filters available: Box blur of fixed radius, Gaussian blur, Poisson filling, Laplacian integration.
 \ingroup ImageFiltering
 */
class FilteringRenderer : public Renderer {
	
public:
	
	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	FilteringRenderer(RenderingConfig & config);
	
	/** Draw the scene and effects */
	void draw();
	
	/** Perform once-per-frame update (buttons, GUI,...) */
	void update();
	
	/** Perform physics simulation update.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 \note This function can be called multiple times per frame.
	 */
	void physics(double fullTime, double frameTime);
	
	/** Clean internal resources. */
	void clean() const;
	
	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	
private:
	
	/** \brief The filter to apply. */
	enum FilterMode : int {
		INPUT = 0, FILL, INTEGRATE, BOXBLUR, GAUSSBLUR
	};
	
	/** \brief The viewing mode: either a rendering or a still image. */
	enum ViewMode : int {
		SCENE = 0, IMAGE
	};
	
	ControllableCamera _userCamera; ///< The interactive camera.
	std::unique_ptr<Framebuffer> _sceneBuffer; ///< Scene rendering buffer.
	
	std::unique_ptr<PoissonFiller> _pyramidFiller; ///< Poisson filling.
	std::unique_ptr<LaplacianIntegrator> _pyramidIntegrator; ///< Laplacian integration.
	std::unique_ptr<GaussianBlur> _gaussianBlur; ///< Gaussian blur processing.
	std::unique_ptr<BoxBlur> _boxBlur; ///< Box blur processing.
	
	const ProgramInfos * _passthrough; ///< Basic blit shader.
	const ProgramInfos * _sceneShader; ///< Object rendering shader.
	const MeshInfos * _mesh; ///< Basic sphere mesh.
	
	FilterMode _mode = INPUT; ///< Current filter mode.
	ViewMode _viewMode = SCENE; ///< Current view mode.
	TextureInfos _image; ///< The image to display in Image view mode.
	
	int _blurLevel = 3; ///< Gaussian blur level.
	int _intDownscale = 1; ///< Integrator internal resolution downscaling.
	int _fillDownscale = 1; ///< Poisson filling internal resolution downscaling.
	bool _showProcInput = false; ///< Option to show the input for both convolution filters.
	
};

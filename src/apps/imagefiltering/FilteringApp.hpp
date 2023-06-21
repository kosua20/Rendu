#pragma once

#include "PaintingTool.hpp"

#include "processing/PoissonFiller.hpp"
#include "processing/LaplacianIntegrator.hpp"
#include "processing/GaussianBlur.hpp"
#include "processing/BoxBlur.hpp"
#include "processing/FloodFiller.hpp"

#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"
#include "system/Config.hpp"

#include "Application.hpp"

#include "Common.hpp"

/**
 \brief Apply multiple image filters on an interactive rendering of a scene.
 
 Filters available: Box blur of fixed radius, Gaussian blur, Poisson filling, Laplacian integration, Flood filling.
 \ingroup ImageFiltering
 */
class FilteringApp final : public CameraApp {

public:
	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	explicit FilteringApp(RenderingConfig & config, Window & window);

	/** \copydoc CameraApp::draw */
	void draw() override;

	/** \copydoc CameraApp::update */
	void update() override;

	/** \copydoc CameraApp::physics */
	void physics(double fullTime, double frameTime) override;

	/** \copydoc CameraApp::resize */
	void resize() override;

private:
	/** \brief The filter to apply. */
	enum class Processing : int {
		INPUT = 0,
		FILL,
		INTEGRATE,
		BOXBLUR,
		GAUSSBLUR,
		FLOODFILL
	};

	/** \brief The viewing mode: either a rendering, a still image or a painting canvas. */
	enum class View : int {
		SCENE = 0,
		IMAGE,
		PAINT
	};

	/** Display mode-specific GUI options. */
	void showModeOptions();

	std::unique_ptr<Framebuffer> _sceneBuffer; ///< Scene rendering buffer.

	std::unique_ptr<PoissonFiller> _pyramidFiller;			 ///< Poisson filling.
	std::unique_ptr<LaplacianIntegrator> _pyramidIntegrator; ///< Laplacian integration.
	std::unique_ptr<GaussianBlur> _gaussianBlur;			 ///< Gaussian blur processing.
	std::unique_ptr<BoxBlur> _boxBlur;						 ///< Box blur processing.
	std::unique_ptr<FloodFiller> _floodFill;				 ///< Flood filling.
	std::unique_ptr<PaintingTool> _painter;					 ///< The painting interface.

	Program * _passthrough; ///< Basic blit shader.
	Program * _sceneShader; ///< Object rendering shader.
	const Mesh * _mesh;			  ///< Basic sphere mesh.

	Processing _mode = Processing::INPUT; ///< Current filter mode.
	View _viewMode   = View::SCENE;		  ///< Current view mode.
	Texture _image = Texture("image");	  ///< The image to display in Image view mode.

	int _blurLevel		= 3;	 ///< Gaussian blur level.
	int _intDownscale   = 1;	 ///< Integrator internal resolution downscaling.
	int _fillDownscale  = 1;	 ///< Poisson filling internal resolution downscaling.
	bool _showProcInput = false; ///< Option to show the input for both convolution filters.
};

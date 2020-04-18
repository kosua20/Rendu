#pragma once


#include "renderers/Renderer.hpp"
#include "renderers/Renderer.hpp"

#include "graphics/Framebuffer.hpp"
#include "processing/GaussianBlur.hpp"

#include "Common.hpp"

/**
 \brief Apply post process effects to a HDR rendering of a scene.
 \ingroup PBRDemo
 */
class PostProcessStack final : public Renderer {
public:
	
	/** \brief Post processing stack settings. */
	struct Settings {
		float exposure	= 1.0f;  ///< Film exposure.
		float bloomTh	= 1.2f;  ///< Threshold for blooming regions.
		float bloomMix	= 0.2f;  ///< Factor for applying the bloom.
		int bloomRadius	= 4;	 ///< Bloom blur radius.
		bool bloom		= true;  ///< Should bloom (bright lights halo-ing) be applied.
		bool tonemap 	= true;  ///< Should HDR to LDR tonemapping be applied.
		bool fxaa		= true;  ///< Apply screenspace anti-aliasing.
	};
	
	/** Constructor.
	 \param resolution the initial rendering resolution
	 */
	explicit PostProcessStack(const glm::vec2 & resolution);
	
	/** Apply post processing to the scene.
	 You can assume that there will be at least one operation applied so the same texture can be used as input and output.
	 \param texture the texture to process
	 \param framebuffer the destination framebuffer
	 \param layer the layer of the destination to write to
	 */
	void process(const Texture * texture, Framebuffer & framebuffer, size_t layer = 0) override;

	/** \copydoc Renderer::interface
	 */
	void interface() override;

	/** \copydoc Renderer::clean
	 */
	void clean() override;

	/** \copydoc Renderer::resize
	 */
	void resize(unsigned int width, unsigned int height) override;

	/** Get the stack settings.
	 \return a reference to the settings.
	 */
	Settings & settings(){ return _settings; }
	
private:

	/** Update the bloom pass depth based on the current set radius. */
	void updateBlurPass();

	std::unique_ptr<Framebuffer> _bloomBuffer; 	 ///< Bloom framebuffer
	std::unique_ptr<Framebuffer> _toneMapBuffer; ///< Tonemapping framebuffer
	std::unique_ptr<GaussianBlur> _blurBuffer;	 ///< Bloom blur processing.
	
	const Program * _bloomProgram;			///< Bloom program
	const Program * _bloomCompositeProgram; ///< Bloom compositing program.
	const Program * _toneMappingProgram; 	///< Tonemapping program
	const Program * _fxaaProgram;		 	///< FXAA program
	
	Settings _settings; ///< The processing settings.
	
};

#pragma once

#include "ForwardLight.hpp"

#include "renderers/DebugLightRenderer.hpp"

#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "renderers/Culler.hpp"

#include "resources/Texture.hpp"
#include "input/ControllableCamera.hpp"

#include "processing/SSAO.hpp"

#include "Common.hpp"


/**
 \brief A renderer that shade each object as it is drawn in the scene directly.
 \sa ForwardLight

 Lights and probes information is stored in large data buffers that each object shader iterates over, summing their lighting contribution and outputing the final result.
 \see GPUShaders::Frag::Object_forward, GPUShaders::Frag::Object_parallax_forward, GPUShaders::Frag::Object_clearcoat_forward, GPUShaders::Frag::Object_anisotropic_forward, GPUShaders::Frag::Object_sheen_forward, GPUShaders::Frag::Object_iridescent_forward,  GPUShaders::Frag::Object_subsurface_forward, GPUShaders::Frag::Object_emissive_forward, GPUShaders::Frag::Object_transparent_forward, GPUShaders::Frag::Object_transparent_irid_forward

 A depth prepass is used to avoid wasting lighting computations on surfaces that are occluded by other objects drawn later in the frame.
 \see GPUShaders::Frag::Object_prepass_forward

 \ingroup PBRDemo
 */
class ForwardRenderer final : public Renderer {

public:
	/** Constructor.
	 \param resolution the initial rendering resolution
	 \param ssao should screen space ambient occlusion be computed
	 \param name the debug name
	 */
	explicit ForwardRenderer(const glm::vec2 & resolution, bool ssao, const std::string & name);

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** \copydoc Renderer::draw */
	void draw(const Camera & camera, Texture* dstColor, Texture* dstDepth, uint layer = 0) override;

	/** \copydoc Renderer::resize
	 */
	void resize(uint width, uint height) override;

	/** \copydoc Renderer::interface */
	void interface() override;

	/** \return the texture containing the scene depth information */
	Texture& sceneDepth();

private:

	/** Render the scene object depth (prepass).
	 \param visibles list of indices of visible objects
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 \note Transparent and parallax objects will be skipped.
	 */
	void renderDepth(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj);

	/** Render the scene opaque objects.
	 \param visibles list of indices of visible objects
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 */
	void renderOpaque(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj);

	/** Render the scene transparent objects.
	 \param visibles list of indices of visible objects 
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 */
	void renderTransparent(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj);

	/** Render the scene background.
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 \param pos the camera position
	 */
	void renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos);

	Texture _sceneColor;	 ///< Scene color texture
	Texture _sceneDepth;	 ///< Scene depth texture
	std::unique_ptr<SSAO> _ssaoPass;				///< SSAO processing.
	std::unique_ptr<ForwardLight> _lightsGPU;	///< The lights renderer.
	std::unique_ptr<ForwardProbe> _probesGPU;	///< The probes renderer.

	Program * _objectProgram;		///< Basic PBR program
	Program * _parallaxProgram;	 	///< Parallax mapping PBR program
	Program * _emissiveProgram;	 	///< Parallax mapping PBR program
	Program * _transparentProgram;	///< Transparent PBR program
	Program * _transpIridProgram;	///< Transparent PBR with iridescent Fresnel
	Program * _clearCoatProgram; 	///< Basic PBR program with an additional clear coat specular layer.
	Program * _anisotropicProgram; 	///< Basic PBR with anisotropic roughness.
	Program * _sheenProgram; 		///< PBR with sheen BRDF.
	Program * _iridescentProgram; 	///< PBR with iridescent Fresnel.
	Program * _subsurfaceProgram;	///< PBR with subsurface scattering.
	Program * _depthPrepass; 		///< Depth prepass program.

	Program * _skyboxProgram; ///< Skybox program.
	Program * _bgProgram;		///< Planar background program.
	Program * _atmoProgram;   ///< Atmospheric scattering program.

	const Texture * _textureBrdf; ///< The BRDF lookup table.

	std::shared_ptr<Scene>  _scene;  ///< The scene to render
	std::unique_ptr<Culler> _culler; ///<Objects culler.

	bool _applySSAO			 = true;  ///< Screen space ambient occlusion.


};

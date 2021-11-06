#pragma once

#include "DeferredLight.hpp"
#include "ForwardLight.hpp"

#include "renderers/DebugLightRenderer.hpp"

#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "renderers/Culler.hpp"

#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"

#include "processing/SSAO.hpp"

#include "Common.hpp"

/**
 \brief Available G-buffer layers.
 \ingroup PBRDemo
 */
enum class TextureType {
	Albedo  = 0, ///< (or base color)
	Normal  = 1,
	Effects = 2, ///< Roughness, metallicness, ambient occlusion factor, ID.
	Depth   = 3
};

/**
 \brief Performs deferred rendering of a scene.
 \sa DeferredLight, DeferredProbe, Material, GPUShaders::Common::Common_pbr
 \details
 Objects material information is renderer in a framebuffer, packing the various parameters as efficiently as
 possible in a fixed number of texture channels. As many components as possible are reused between materials,
 and the precision of some parameters is lowered. See the Material class documentation for all parameter details.

 __Shared section:__
 |            | _______Red‏‏‎_______ | ______Green______ | _______Blue______ | ______Alpha______ |
 | :--------- | :---------------: | :---------------: | :---------------: | :---------------: |
 | 0: RGBA8   |             Albedo/reflectance/emissive color           |||    Material ID    |
 | 1: RGB10A2 |          Normal octahedral XY        ||                                      ||
 | 2: RGBA8   |                                                                            ||||

 __Standard (& Parallax):__
 |            | _______Red‏‏‎_______ | ______Green______ | _______Blue______ | ______Alpha______ |
 | :--------- | :---------------: | :---------------: | :---------------: | :---------------: |
 | 1: RGB10A2 |                                      ||                                      ||
 | 2: RGBA8   |     Roughness     |   Metalness(5)    | Ambient occlusion |                   |
 \see GPUShaders::Frag::Object_gbuffer, GPUShaders::Frag::Object_parallax_gbuffer

 __Clearcoat:__
 |            | _______Red‏‏‎_______ | ______Green______ | _______Blue______ | ______Alpha______ |
 | :--------- | :---------------: | :---------------: | :---------------: | :---------------: |
 | 1: RGB10A2 |                                      ||                                      ||
 | 2: RGBA8   |     Roughness     | Metal(5) Coating(3) | Ambient occlusion | Coat roughness  |
 \see GPUShaders::Frag::Object_clearcoat_gbuffer

 __Anisotropic:__
 |            | _______Red‏‏‎_______ | ______Green______ | _______Blue______ | ______Alpha______ |
 | :--------- | :---------------: | :---------------: | :---------------: | :---------------: |
 | 1: RGB10A2 |                                      || Anisotropy angle  | Anisotropy signs  |
 | 2: RGBA8   |     Roughness     |   Metalness(5)    | Ambient occlusion |     Anisotropy    |
 \see GPUShaders::Frag::Object_anisotropic_gbuffer

 __Sheen:__
 |            | _______Red‏‏‎_______ | ______Green______ | _______Blue______ | ______Alpha______ |
 | :--------- | :---------------: | :---------------: | :---------------: | :---------------: |
 | 1: RGB10A2 |                                      ||           Sheen color (3*4)          ||
 | 2: RGBA8   |     Roughness     |   Sheeness(5)     | Ambient occlusion |  Sheen roughness  |
 \see GPUShaders::Frag::Object_sheen_gbuffer

 __Iridescent:__
 |            | _______Red‏‏‎_______ | ______Green______ | _______Blue______ | ______Alpha______ |
 | :--------- | :---------------: | :---------------: | :---------------: | :---------------: |
 | 1: RGB10A2 |                                      || Index of refraction |                 |
 | 2: RGBA8   |     Roughness     |   Metalness(5)    | Ambient occlusion |     Thickness     |
 \see GPUShaders::Frag::Object_iridescent_gbuffer

 __Subsurface:__
 |            | _______Red‏‏‎_______ | ______Green______ | _______Blue______ | ______Alpha______ |
 | :--------- | :---------------: | :---------------: | :---------------: | :---------------: |
 | 1: RGB10A2 |                                      ||         Subsurface color (3*4)       ||
 | 2: RGBA8   |     Roughness     | Subsurf rough(5)  | Ambient occlusion |     Thickness     |
 \see GPUShaders::Frag::Object_subsurface_gbuffer

 __Emissive:__
 |            | _______Red‏‏‎_______ | ______Green______ | _______Blue______ | ______Alpha______ |
 | :--------- | :---------------: | :---------------: | :---------------: | :---------------: |
 | 1: RGB10A2 |                                      ||     Roughness     |                   |
 | 2: RGBA8   |                            Emissive intensity                              ||||
 \see GPUShaders::Frag::Object_emissive_gbuffer

 The G-buffer is then read by each light, rendered as a geometric proxy onto the scene depth buffer and writing its lighting contribution to a lighting buffer. Ambient probes are processed similarly. Direct and ambient lighting are finally merged to generate the final image.

 Transparent objects are rendered in a forward pass.
 \see GPUShaders::Frag::Object_transparent_forward, GPUShaders::Frag::Object_transparent_irid_forward

 \ingroup PBRDemo
 */
class DeferredRenderer final : public Renderer {

public:
	/** Constructor.
	 \param resolution the initial rendering resolution
	 \param mode the shadow rendering algorithm
	 \param ssao should screen space ambient occlusion be computed
	 \param name the debug name
	 */
	explicit DeferredRenderer(const glm::vec2 & resolution, ShadowMode mode, bool ssao, const std::string & name);

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** \copydoc Renderer::draw */
	void draw(const Camera & camera, Framebuffer & framebuffer, uint layer = 0) override;

	/** \copydoc Renderer::resize
	 */
	void resize(uint width, uint height) override;

	/** \copydoc Renderer::interface */
	void interface() override;

	/** \return the framebuffer containing the scene depth information */
	const Framebuffer * sceneDepth() const;

private:

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
	
	/** Render the scene background to the G-buffer.
	 \param view the camera view matrix
	 \param proj the camera projection matrix
	 \param pos the camera position
	 */
	void renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos);

	std::unique_ptr<Framebuffer> _gbuffer;			///< G-buffer.
	std::unique_ptr<Framebuffer> _lightBuffer;		///< Lighting accumulation.
	std::unique_ptr<Framebuffer> _indirectLightingBuffer;	///< Indirect lighting accumulation.
	std::unique_ptr<SSAO> _ssaoPass;				///< SSAO processing.
	std::unique_ptr<DeferredLight> _lightRenderer;	///< The lights renderer.
	std::unique_ptr<DeferredProbe> _probeRenderer;	///< The probes renderer.
	std::unique_ptr<ForwardLight> _fwdLightsGPU;	///< The lights forward renderer for transparent objects.
	std::unique_ptr<ForwardProbe> _fwdProbesGPU;	///< The probes forward renderer for transparent objects.

	Program * _objectProgram;		///< Basic PBR program
	Program * _parallaxProgram;		///< Parallax mapping PBR program
	Program * _clearCoatProgram;	///< Basic PBR program with an additional clear coat specular layer.
	Program * _anisotropicProgram;	///< Basic PBR with anisotropic roughness.
	Program * _sheenProgram;		///< PBR with sheen BRDF.
	Program * _iridescentProgram;	///< PBR with iridescent Fresnel.
	Program * _subsurfaceProgram;	///< PBR with subsurface scattering.
	Program * _emissiveProgram;	 	///< Emissive program.
	Program * _transparentProgram;	///< Transparent PBR program.
	Program * _transpIridProgram;   ///< Transparent PBR with iridescent Fresnel program.
	Program * _probeNormalization;	///< Indirect lighting normalization program.

	Program * _skyboxProgram; ///< Skybox program.
	Program * _bgProgram;		///< Planar background program.
	Program * _atmoProgram;   ///< Atmospheric scattering program.

	const Texture * _textureBrdf; ///< The BRDF lookup table.

	std::shared_ptr<Scene> _scene;	///< The scene to render
	std::unique_ptr<Culler>	_culler;	///< Objects culler.

	bool _applySSAO			 = true;  ///< Screen space ambient occlusion.
	ShadowMode  _shadowMode	 = ShadowMode::VARIANCE;  ///< Shadow mapping technique to use.
};

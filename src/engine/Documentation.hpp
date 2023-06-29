#pragma once
/**
 
 
 \defgroup Engine Engine
 \brief General code.
 \details This is the default module for Rendu objects.
 
 
 \defgroup Graphics Graphics
 \brief Contain items and functions interacting with the graphics API.
 \details This module provides wrapper around GPU primitives, such as textures, meshes and shader programs. Utilities are also provided to render screen quads, alter the rendering state and populate/save GPU data. The interface is currently based on the Vulkan API.

 \defgroup Generation Generation
 \brief Generation of randomness, noise and other procedural content.
 \details Utilities can be used to randomly samples various spaces and distributions, and to generate image content following certain procedural rules (Perlin noise,...).
 
 \defgroup Input Input
 \brief Handle user input through keyboard, mouse and controllers and provide controllable cameras.
 \details The Input manager is keeping track of the internal input state based on all external events. Support for common game controllers is provided through GLFW with a common mapping system. This module also provides simple and interactive cameras for inspecting scenes.
 
 
 \defgroup Processing Processing
 \brief General image processing utilities.
 \details Real-time image processing methods, such as box blur, approximate gaussian blur, flood-filling. An implementation of the Convolution pyramids paper also offer a Poisson filling tool and a laplacian integrator.


 \defgroup Renderers Renderers
 \brief General 3D renderers.
 \details Renderers are used to display geometry on screen or an off-screen texture. Some renderers can display debug visualization while others can be used to process different types of objects, or to generate information for shadows rendering.

 
 \defgroup Raycaster Raycaster
 \brief Compute intersection between rays and geometry.
 \details Provides a raycaster relying on a bounding volume hierarchy of axis-aligned boxes to accelerate ray/geometry intersection queries.
 
 
 \defgroup Resources Resources
 \brief Handle resource loading/saving tasks.
 \details The Resources manager is sole responsible for allocation/deallocation of resources loaded from the disk directory hierarchy or from an archive. Images and meshes can be sent to the GPU as textures and meshes. Storage settings (CPU/GPU, layout in memory, format) are specified at first use of a resource.
 
 
 \defgroup Scene Scene
 \brief Contain objects, lights and environment representations used in 3D scene rendering.
 \details A scene can be represented on disk as a text file referencing resources by name. The general scene parameters and environment are setup as follows:
 \verbatim
  * scene:
		translation: X,Y,Z
		scaling: scale
		orientation: axisX,axisY,axisZ angle
  * background:
		bgtype: value
 \endverbatim
 where bgtype is one of
 \verbatim
   color: R,G,B
   image: texturetype: texturename
   cube: texturetype: texturename
   bgsky: direction: X,Y,Z
 \endverbatim
 
 Each object can the be declared as follows:
 \verbatim
  * object:
		type: objecttype
		mesh: meshname
		translation: X,Y,Z
		scaling: scale
		orientation: axisX,axisY,axisZ angle
		shadows: bool
		twosided: bool
		masked: bool
		textures:
			- texturetype: texturename
			- ...
		animations:
			- rotation: speed frame axisX,axisY,axisZ
			- backandforth: speed frame axisX,axisY,axisZ amplitude
			- ...
 \endverbatim
 Animations can be applied in either the world or model frame.
 
 Lights of different types can be declared.
 Point lights:
 \verbatim
  * point:
		position: X,Y,Z
		radius: radius
		intensity: R,G,B
		shadows: bool
		animations:
		   - ...
 \endverbatim
 Spot lights:
 \verbatim
  * spot:
		direction: dirX,dirY,dirZ
		position: X,Y,Z
		radius: radius
		cone: innerAngle outerAngle
		intensity: R,G,B
		shadows: bool
		animations:
			- ...
 \endverbatim
 Directional lights:
 \verbatim
  * directional:
		direction: dirX,dirY,dirZ
		intensity: R,G,B
		shadows: bool
		animations:
		   - ...
 \endverbatim
 Finally, an environment probe can be defined as follow, either static (using a cubemap and precompute SH coeffs):
 \verbatim
 * probe:
	 radiance: texturetype: texturename
	 irradiance: shcoeffsfilename
 \endverbatim
 or updated on the fly
 \verbatim
 * probe:
	 position: X,Y,Z
 \endverbatim
 For more details on each parameter, consult each object decode function documentation.
 \see Scene::init, Object::decode, Light::decodeBase, PointLight::decode, DirectionalLight::decode, SpotLight::decode, Animation::decodeBase, BackAndForth::decode, Rotation::decode, Codable::decodeTexture, Sky::decode, LightProbe::decode
 
 
 \defgroup System System
 \brief Interactions with the operating system.
 \details General purpose helpers to process text, generate random numbers, perform logging and (de)serialization, parse command-line arguments, create a system window backed by a GPU context,  create directories, present a file picker...
 
 
 \defgroup Applications Applications
 \brief Applications built with Rendu.
 
 
 \defgroup Tools Tools
 \brief Preprocess tools for precomputation and validation, etc.
 
 
 \defgroup Shaders Shaders
 \brief GPU shading programs.
 \details Shaders are small programs compiled at runtime and executed by the GPU cores. They can process vertices (vertex shader), primitives (geometry shader) and compute per-pixel values (fragment shader).
 
 \namespace GPUShaders
 \brief Contains all shaders.

 \namespace GPUShaders::Common
 \brief Contains shared shader code.

 \namespace GPUShaders::Vert
 \brief Contains all vertex shaders.
 
 \namespace GPUShaders::Frag
 \brief Contains all fragment shaders.
 
 \namespace GPUShaders::Geom
 \brief Contains all geometry shaders.

 \namespace GPUShaders::TessControl
 \brief Contains all tessellation control shaders.

 \namespace GPUShaders::TessEval
 \brief Contains all tessellation evaluation shaders.

 */

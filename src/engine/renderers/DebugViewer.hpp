#pragma once
#include "graphics/Program.hpp"
#include "graphics/GPUTypes.hpp"

class Texture;
class Mesh;

/**
 \brief Provide helper GUI to display the content of textures and mesh infos.
 This can be useful to validate the content rendered to a specific texture when debugging.
 \ingroup Renderers
 */
class DebugViewer {

public:

	/** Constructor.
	 */
	explicit DebugViewer();

	/** Register a texture for debug.
	\param tex the texture to monitor
	*/
	void track(const Texture * tex);

	/** Register a mesh for debug.
	\param mesh the mesh to monitor
	*/
	void track(const Mesh * mesh);

	/** Track the GPU state at the moment of the call. Can be called at each frame to track varying state.
	 \param name the display name of the state
	 */
	void trackState(const std::string & name);

	/** Stop monitoring a texture.
	\param tex the texture to stop tracking
	*/
	void untrack(const Texture * tex);

	/** Stop monitoring a mesh.
	\param mesh the mesh to stop tracking
	*/
	void untrack(const Mesh * mesh);

	/** Register a scoped marker
	 \param category the marker category
	 \param label the marker name
	 \param color the marker color
	 */
	void pushMarker(const std::string& category, const std::string& label, const glm::vec4& color);

	/** Register a punctual marker
	 \param category the marker category
	 \param label the marker name
	 \param color the marker color
	 */
	void insertMarker(const std::string& category, const std::string& label, const glm::vec4& color);

	/** Notify of the end of a scoped marker
	 \param category the category to which the marker belonged
	 */
	void popMarker(const std::string& category);

	/** Move to the next frame */
	void nextFrame();

	/** Display interface and monitored data. */
	void interface();

	/** Destructor */
	~DebugViewer() = default;

	/** Copy constructor.*/
	DebugViewer(const DebugViewer &) = delete;

	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	DebugViewer & operator=(const DebugViewer &) = delete;

	/** Move constructor.*/
	DebugViewer(DebugViewer &&) = delete;

	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	DebugViewer & operator=(DebugViewer &&) = delete;

public:

	/** Register a default debug viewer.
	 \param viewer the viewer to use as default*/
	static void setDefault(DebugViewer * viewer);

	/** Register a texture for debug.
	\param tex the texture to monitor
	*/
	static void trackDefault(const Texture * tex);

	/** Register a mesh for debug.
	\param mesh the mesh to monitor
	*/
	static void trackDefault(const Mesh * mesh);

	/** Register current GPU state for debug;
	 \param name how to name the state in the list
	 */
	static void trackStateDefault(const std::string & name);

	/** Stop monitoring a texture.
	\param tex the texture to stop tracking
	*/
	static void untrackDefault(const Texture * tex);

	/** Stop monitoring a mesh.
	\param mesh the mesh to stop tracking
	*/
	static void untrackDefault(const Mesh * mesh);

	/** Register a scoped marker
	\param category the marker category
	\param label the marker name
	\param color the marker color
	*/
	static void pushMarkerDefault(const std::string& category, const std::string& label, const glm::vec4& color);

	/** Register a punctual marker
	\param category the marker category
	\param label the marker name
	\param color the marker color
	*/
	static void insertMarkerDefault(const std::string& category, const std::string& label, const glm::vec4& color);

	/** Notify of the end of a scoped marker
	\param category the category to which the marker belonged
	*/
	static void popMarkerDefault(const std::string& category);

private:

	static DebugViewer * _shared; ///< Optional shared debug viewer.

private:

	/** Texture display information */
	struct TextureInfos {
		const Texture * tex = nullptr; ///< The texture to display.
		std::string name; ///< Texture name.
		std::unique_ptr<Texture> display; ///< Texture used for visualization.
		std::string displayName; ///< Texture name with extra information about the layout,...
		glm::vec2 range = glm::vec2(0.0f, 1.0f); ///< Range of values to display normalized.
		glm::bvec4 channels = glm::bvec4(true, true, true, false); ///< Channels that should be displayed.
		int mip	= 0; ///< Mipmap level to display.
		int layer = 0; ///< Layer to display for arrays and 3D textures.
		bool gamma = false; ///< Should gamma correction be applied.
		bool visible = false; ///< Is the texture window visible.
	};

	/** Mesh information. */
	struct MeshInfos {
		const Mesh * mesh = nullptr; ///< Mesh to track.
		std::string name; ///< Mesh display name.
		bool visible = false; ///< Are the mesh details displayed.
	};

	/** Monitored GPU state. */
	struct StateInfos {
		GPUState state; ///< GPU state to track.
		bool visible = false; ///< Is the state window visible.
		bool populated = false; ///< Has the state already been queried.
	};

	/** Marker information. */
	struct MarkerInfos {
		std::string name; ///< Marker label.
		glm::vec4 color; ///< Marker color.
		uint index; ///< Marker index in the frame.
		std::vector<MarkerInfos> markers; ///< Child markers.
	};

	/** Markers category information. */
	struct MarkerCategoryInfos {
		std::vector<MarkerInfos> markers; ///< Root markers.
		int  frequency = 1; ///< Sampling frequency.
		int  offset = 0; ///< Sampling offset.
		uint depth = 0; ///< Current marker hierarchy depth.
		bool visible = false;  ///< Are the marker details displayed.
		bool record = true; ///< Should the category be recorded at this frame.
	};

	/** Display GPU metrics for the last completed frame in a panel.
	 */
	void displayMetrics();

	/** Display GPU marker and its children
	 \param marker the marker to display
	 */
	void displayMarker(const MarkerInfos& marker);

	/** Display a hierarchy of GPU markers
	 \param name the name of the category
	 \param category the category of markers to display
	 */
	void displayMarkers(const std::string & name, MarkerCategoryInfos& category);

	/** Display GPU state in a panel.
	 \param name name of the state
	 \param infos the state to display
	 */
	void displayState(const std::string & name, StateInfos & infos);

	/** Display a mesh information in a panel.
	 \param mesh the mesh to display
	 */
	void displayMesh(MeshInfos & mesh);

	/** Populate texture information based on an input texture.
	\param name the name of the texture
	\param tex the texture to monitor
	\param infos the information that should be populated
	*/
	void registerTexture(const std::string & name, const Texture * tex, TextureInfos & infos);

	/** Display a texture with some helper GUI.
	\param prefix the display name of the texture
	\param tex the texture information to display
	*/
	void displayTexture(const std::string & prefix, TextureInfos & tex);

	/** Update the visualization associated to a texture/
	\param tex the texture to update the display of
	*/
	void updateDisplay(const TextureInfos & tex);

	std::vector<TextureInfos> _textures; ///< The registered textures.
	std::vector<TextureInfos> _drawables; ///< The registered drawable textures.
	std::vector<MeshInfos> _meshes; ///< The registered meshes.
	std::unordered_map<std::string, StateInfos> _states; ///< GPU states currently tracked.
	std::unordered_map<std::string, MarkerCategoryInfos> _markers; ///< The registered markers.

	Program * _texDisplay; ///< Texture display shader.
	uint64_t _frameCounter = 0; ///< Frame counter.
	uint _textureId  = 0; ///< Default texture name counter.
	uint _drawableId = 0; ///< Default drawable name counter.
	uint _meshId     = 0; ///< Default mesh name counter.
	uint _winId		 = 0; ///< Internal window counter.
	uint _markerId   = 0; ///< Internal marker counter.

};

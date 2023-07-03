#pragma once
#include "system/Config.hpp"
#include "input/ControllableCamera.hpp"
#include "renderers/DebugViewer.hpp"

class Window;

/**
 \brief Base structure of an application.
 \ingroup Engine
 */
class Application {

public:
	/** Constructor.
	 \param config the configuration to apply when setting up
	 \param window the window to render to
	 */
	explicit Application(RenderingConfig & config, Window & window);

	/** Draw call. */
	virtual void draw() = 0;
	
	/** Interactions call. */
	virtual void update();

	/** End of frame operations (screenshot,...) */
	virtual void finish();
	
	/** Handle a window resize event, the configuration has been updated with the new size. */
	virtual void resize() = 0;

	/// \return the time elapsed since launch
	double timeElapsed();

	/// \return the last frame time
	double frameTime();

	/// \return the frame rate, smoothed over the last 30 frames.
	double frameRate();

	/// \return the application main window
	Window & window();

	/** Destructor */
	virtual ~Application();

	/** Copy constructor.*/
	Application(const Application &) = delete;

	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	Application & operator=(const Application &) = delete;

	/** Move constructor.*/
	Application(Application &&) = delete;

	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	Application & operator=(Application &&) = delete;

protected:

	RenderingConfig & _config; ///< The current configuration.

	std::unique_ptr<DebugViewer> _debug; ///< Debug viewer.

private:

	Window & _window;			///< The application main window
	
	double _timer = 0.0; 		///< Absolute timer.
	double _startTime = 0.0; 	///< TImer value at app start.
	double _frameTime = 0.0; 	///< Last frame duration.

	static const size_t _framesCount = 30; ///< Frames to average over for FPS display.
	std::array<double, _framesCount> _frameTimes; ///< Last N frame times.
	double _smoothTime = 0.0; ///< Time elapsed over the last N frames.
	size_t _currFrame = 0; ///< Current frame index for smoothing.
	bool _showDebug = false; ///< Show debug viewer.
};

/**
 \brief Application with an interactive camera.
 \ingroup Engine
 */
class CameraApp : public Application {
public:
	
	/** Constructor.
	 \param config the rendering configuration
	 \param window the window to render to
	 */
	explicit CameraApp(RenderingConfig & config, Window & window);
	
	/** Per-frame update. */
	virtual void update() override;
	
	/** Perform physics simulation update.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 \note This function can be called multiple times per frame.
	 */
	virtual void physics(double fullTime, double frameTime);
	
protected:
	
	/** Prevent the user from interacting with the camera.
	 \param shouldFreeze the toggle
	 */
	void freezeCamera(bool shouldFreeze);
	
	ControllableCamera _userCamera; ///< The interactive camera.

private:
	
	bool _freezeCamera    = false; 	///< Should the camera be frozen.
	double _fullTime	  = 0.0; 	///< Time elapsed since the app launch.
	double _remainingTime = 0.0;	///< Remaining accumulated time to process.
	const double _dt = 1.0 / 120.0; ///< Small physics timestep.
};


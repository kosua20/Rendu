#pragma once
#include "system/Config.hpp"
#include "input/Camera.hpp"

/**
 \brief Base structure of an application.
 \ingroup Engine
 */
class Application {

public:
	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	explicit Application(RenderingConfig & config);

	/** Draw. */
	virtual void draw() = 0;

	/** Perform once-per-frame update (buttons, GUI,...) */
	virtual void update();

	/** Perform physics simulation update.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 \note This function can be called multiple times per frame.
	 */
	virtual void physics(double fullTime, double frameTime) = 0;

	/** Clean internal resources. */
	virtual void clean() = 0;
	
	/** Handle a window resize event, the configuration has been updated with the new size. */
	virtual void resize() = 0;

	/** Destructor */
	virtual ~Application() = default;

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
};


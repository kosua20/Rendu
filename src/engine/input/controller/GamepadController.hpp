#pragma once
#include "input/controller/Controller.hpp"

/**
 \brief Represents a controller with a predefined mapping provided by GLFW/SDL.
 \ingroup Input
 */
class GamepadController final : public Controller {

public:
	/**
	 \copydoc Controller::activate
	 */
	bool activate(int id) override;

	/**
	 \copydoc Controller::deactivate
	 */
	void deactivate() override;

	/**
	 \copydoc Controller::update
	 */
	void update() override;
};

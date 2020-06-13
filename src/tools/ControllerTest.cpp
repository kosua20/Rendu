#include "input/Input.hpp"
#include "system/System.hpp"
#include "system/Config.hpp"
#include "system/Window.hpp"
#include "resources/ResourcesManager.hpp"
#include "input/controller/RawController.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GLUtilities.hpp"
#include "Common.hpp"

/**
 \defgroup ControllerTest Controller Test
 \brief Configuration tool to generate and test controller mappings.
 \ingroup Tools
 */

/** \brief Display a numbered combo list for a given button or axis mapping.
 \param label title of the combo
 \param count number of items
 \param prefix prefix to apply to each item in the list
 \param currentId the currently selected item
 \ingroup ControllerTest
 */
void showCombo(const std::string & label, const int count, const std::string & prefix, int & currentId) {
	const std::string currentLabel = currentId >= 0 ? (prefix + std::to_string(currentId)) : "None";
	if(ImGui::BeginCombo(label.c_str(), currentLabel.c_str())) {
		for(int i = -1; i < count; i++) {
			const bool itemSelected = currentId == i;
			const std::string lS	= i < 0 ? "None" : (prefix + std::to_string(i));
			ImGui::PushID(reinterpret_cast<void *>(static_cast<intptr_t>(i)));
			if(ImGui::Selectable(lS.c_str(), itemSelected)) {
				currentId = i;
			}
			if(itemSelected)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}
}

/** \brief Draw raw geometry for highlighting a given controller button.
 \param drawList the ImGui drawing list to use
 \param bid	the ID of the button to draw
 \param pos the reference position on screen (upper-left corner of the controller overlay)
 \param highlightColor the color to use for the highlight
 \ingroup ControllerTest
 */
void drawButton(ImDrawList * drawList, const Controller::Input bid, const ImVec2 & pos, const ImU32 highlightColor) {
	switch(bid) {
		case Controller::Input::ButtonX:
			drawList->AddCircleFilled(ImVec2(pos.x + 326, pos.y + 118), 12, highlightColor);
			break;
		case Controller::Input::ButtonY:
			drawList->AddCircleFilled(ImVec2(pos.x + 351, pos.y + 93), 12, highlightColor);
			break;
		case Controller::Input::ButtonA:
			drawList->AddCircleFilled(ImVec2(pos.x + 351, pos.y + 143), 12, highlightColor);
			break;
		case Controller::Input::ButtonB:
			drawList->AddCircleFilled(ImVec2(pos.x + 376, pos.y + 118), 12, highlightColor);
			break;
		case Controller::Input::BumperL1:
			drawList->AddRectFilled(ImVec2(pos.x + 69, pos.y + 43), ImVec2(pos.x + 137, pos.y + 67), highlightColor, 5.0);
			break;
		case Controller::Input::TriggerL2: {
			const std::vector<ImVec2> pointsL2 = {
				ImVec2(pos.x + 67, pos.y + 36),
				ImVec2(pos.x + 75, pos.y + 20),
				ImVec2(pos.x + 90, pos.y + 11),
				ImVec2(pos.x + 111, pos.y + 10),
				ImVec2(pos.x + 126, pos.y + 19),
				ImVec2(pos.x + 137, pos.y + 36)};

			drawList->AddConvexPolyFilled(&pointsL2[0], int(pointsL2.size()), highlightColor);
			break;
		}
		case Controller::Input::ButtonL3:
			drawList->AddCircleFilled(ImVec2(pos.x + 154, pos.y + 179), 26, highlightColor);
			break;
		case Controller::Input::BumperR1:
			drawList->AddRectFilled(ImVec2(pos.x + 316, pos.y + 43), ImVec2(pos.x + 384, pos.y + 67), highlightColor, 5.0);
			break;
		case Controller::Input::TriggerR2: {
			const std::vector<ImVec2> pointsR2 = {
				ImVec2(pos.x + 67 + 248, pos.y + 36),
				ImVec2(pos.x + 75 + 248, pos.y + 20),
				ImVec2(pos.x + 90 + 248, pos.y + 11),
				ImVec2(pos.x + 111 + 248, pos.y + 10),
				ImVec2(pos.x + 126 + 248, pos.y + 19),
				ImVec2(pos.x + 137 + 248, pos.y + 36)};

			drawList->AddConvexPolyFilled(&pointsR2[0], int(pointsR2.size()), highlightColor);
		} break;
		case Controller::Input::ButtonR3:
			drawList->AddCircleFilled(ImVec2(pos.x + 296, pos.y + 179), 26, highlightColor);
			break;
		case Controller::Input::ButtonUp:
			drawList->AddRectFilled(ImVec2(pos.x + 90, pos.y + 82), ImVec2(pos.x + 107, pos.y + 106), highlightColor, 5.0);
			break;
		case Controller::Input::ButtonLeft:
			drawList->AddRectFilled(ImVec2(pos.x + 62, pos.y + 110), ImVec2(pos.x + 87, pos.y + 126), highlightColor, 5.0);
			break;
		case Controller::Input::ButtonDown:
			drawList->AddRectFilled(ImVec2(pos.x + 90, pos.y + 132), ImVec2(pos.x + 107, pos.y + 156), highlightColor, 5.0);
			break;
		case Controller::Input::ButtonRight:
			drawList->AddRectFilled(ImVec2(pos.x + 112, pos.y + 110), ImVec2(pos.x + 137, pos.y + 126), highlightColor, 5.0);
			break;
		case Controller::Input::ButtonLogo:
			drawList->AddCircleFilled(ImVec2(pos.x + 225, pos.y + 120), 24, highlightColor);
			break;
		case Controller::Input::ButtonMenu:
			drawList->AddCircleFilled(ImVec2(pos.x + 275, pos.y + 96), 13, highlightColor);
			break;
		case Controller::Input::ButtonView:
			drawList->AddCircleFilled(ImVec2(pos.x + 175, pos.y + 96), 13, highlightColor);
			break;
		default:
			break;
	}
}

/**
 \brief Draw a target circle and threshold along with the current pad position.
 \param idX the ID of the horizontal axis
 \param idY the ID of the vertical axis
 \param axesValues the axes raw values
 \param threshRadius the radius of the filtering threshold
 \ingroup ControllerTest
 */
void drawPadTarget(const int idX, const int idY, const std::vector<float> & axesValues, const float threshRadius) {
	const ImU32 whiteColor = IM_COL32(255, 255, 255, 255);
	const int aidRX		   = idX;
	const int aidRY		   = idY;
	const float magRX	  = aidRX >= 0 ? axesValues[aidRX] : 0.0f;
	const float magRY	  = aidRY >= 0 ? axesValues[aidRY] : 0.0f;
	// Detect overflow on each axis.
	const bool overflow = (std::abs(magRX) > 1.0) || (std::abs(magRY) > 1.0);
	// Get current rendering position on screen.
	const ImVec2 posR	  = ImGui::GetCursorScreenPos();
	ImDrawList * drawListR = ImGui::GetWindowDrawList();
	// Draw "safe" region.
	drawListR->AddRectFilled(posR, ImVec2(posR.x + 200, posR.y + 200), overflow ? IM_COL32(30, 0, 0, 255) : IM_COL32(0, 30, 0, 255));
	drawListR->AddCircleFilled(ImVec2(posR.x + 100, posR.y + 100), threshRadius, IM_COL32(0, 0, 0, 255), 32);
	// Draw frame and cross lines.
	drawListR->AddRect(posR, ImVec2(posR.x + 200, posR.y + 200), overflow ? IM_COL32(255, 0, 0, 255) : whiteColor);
	drawListR->AddLine(ImVec2(posR.x + 100, posR.y), ImVec2(posR.x + 100, posR.y + 200), whiteColor);
	drawListR->AddLine(ImVec2(posR.x, posR.y + 100), ImVec2(posR.x + 200, posR.y + 100), whiteColor);
	// Draw threshold and unit radius circles.
	drawListR->AddCircle(ImVec2(posR.x + 100, posR.y + 100), threshRadius, IM_COL32(0, 255, 0, 255), 32);
	drawListR->AddCircle(ImVec2(posR.x + 100, posR.y + 100), 100, whiteColor, 32);
	// Current axis position.
	drawListR->AddCircleFilled(ImVec2(posR.x + magRX * 100 + 100, posR.y + magRY * 100 + 100), 10, whiteColor);
}

/**
 \brief Draw a target line and threshold along with the current trigger position.
 \param idT the ID of the trigger axis
 \param axesValues the axes raw values
 \param threshRadius the value of the filtering threshold
 \ingroup ControllerTest
 */
void drawTriggerTarget(const int idT, const std::vector<float> & axesValues, const float threshRadius) {
	const ImU32 whiteColor = IM_COL32(255, 255, 255, 255);
	const int aidLT		   = idT;
	const float magLT	  = aidLT >= 0 ? axesValues[aidLT] * 0.5f + 0.5f : 0.0f;
	// Detect overflow.
	const bool overflow = (magLT > 1.0f || magLT < 0.0f);
	// Get current rendering position on screen.
	const ImVec2 posR	  = ImGui::GetCursorScreenPos();
	ImDrawList * drawListR = ImGui::GetWindowDrawList();
	const float thresholdY = (200.0f - 2.0f * threshRadius);
	const float currentY   = 200 * (1.0f - magLT);
	// Draw "safe" region.
	drawListR->AddRectFilled(posR, ImVec2(posR.x + 40, posR.y + thresholdY), overflow ? IM_COL32(30, 0, 0, 255) : IM_COL32(0, 30, 0, 255));
	// Draw threshold line.
	drawListR->AddLine(ImVec2(posR.x, posR.y + thresholdY), ImVec2(posR.x + 40, posR.y + thresholdY), IM_COL32(0, 255, 0, 255));
	// Draw frame.
	drawListR->AddRect(posR, ImVec2(posR.x + 40, posR.y + 200), overflow ? IM_COL32(255, 0, 0, 255) : whiteColor);
	// Current axis position.
	drawListR->AddLine(ImVec2(posR.x, posR.y + currentY), ImVec2(posR.x + 40, posR.y + currentY), whiteColor, 4.0f);
}

/**
 The main function of the controller tester.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup ControllerTest
 */
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	// Override window dimensions.
	config.initialWidth  = 800;
	config.initialHeight = 800;
	Window window("Controller test", config);
	
	Resources::manager().addResources("../../../resources/common");

	// Enable raw mode for the input, that way all controllers will be raw controllers.
	Input::manager().preferRawControllers(true);

	// Will contain reference button/axes to raw input mappings.
	std::vector<int> buttonsMapping(uint(Controller::InputCount), -1);
	std::vector<int> axesMapping(uint(Controller::InputCount), -1);
	// Controller texture.
	const Texture * controllerTex = Resources::manager().getTexture("ControllerLayout", {Layout::RGBA8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

	bool firstFrame			   = true;
	const ImU32 highlightColor = IM_COL32(172, 172, 172, 255);

	float threshold = 0.02f;

	// Start the display/interaction loop.
	while(window.nextFrame()) {

		// Detect either a new connected controller or a first frame with an already connected controller.
		if(Input::manager().controllerConnected() || (firstFrame && Input::manager().controllerAvailable())) {
			firstFrame						 = false;
			const RawController * controller = dynamic_cast<RawController *>(Input::manager().controller());
			const int axesCount				 = int(controller->allAxes.size());
			const int buttonsCount			 = int(controller->allButtons.size());

			// Check if some elements were already modified.
			bool wereEmpty = true;
			for(int & button : buttonsMapping) {
				if(wereEmpty && button >= 0) {
					wereEmpty = false;
				}
				// Update mapping for extraneous button IDs.
				if(button >= buttonsCount) {
					button = -1;
				}
			}
			for(int & axe : axesMapping) {
				if(wereEmpty && axe >= 0) {
					wereEmpty = false;
				}
				if(axe >= axesCount) {
					axe = -1;
				}
			}

			// If everything was -1 (first launch), attribute the buttons and axes sequentially, just to help with the visualisation and assignment.
			if(wereEmpty) {
				for(int i = 0; i < std::min(int(buttonsMapping.size()), buttonsCount); ++i) {
					buttonsMapping[i] = i;
				}
				// Start from the end for the axes.
				for(int i = 0; i < std::min(int(axesMapping.size()), axesCount); ++i) {
					const int actionId = int(axesMapping.size()) - 1 - i;
					// Avoid double mappings.
					if(buttonsMapping[actionId] >= 0) {
						continue;
					}
					axesMapping[actionId] = i;
				}
			}
		}

		// Render nothing.
		const glm::ivec2 screenSize = Input::manager().size();
		Framebuffer::backbuffer()->bind();
		GLUtilities::setViewport(0, 0, screenSize[0], screenSize[1]);
		GLUtilities::clearColor({0.0f, 0.0f, 0.0f, 1.0f});
		Framebuffer::backbuffer()->unbind();

		// Set a fullscreen fixed window.
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		const ImGuiWindowFlags windowOptions = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar;

		if(ImGui::Begin("Controller", nullptr, windowOptions)) {
			if(!Input::manager().controllerAvailable()) {
				ImGui::Text("No controller connected.");
			} else {
				// Load/save configuration files.
				if(ImGui::Button("Load...")) {
					std::string inputPath;
					const bool res = System::showPicker(System::Picker::Load, "", inputPath);
					if(res && !inputPath.empty()) {
						const std::string settingsContent = Resources::loadStringFromExternalFile(inputPath);
						Controller::parseConfiguration(settingsContent, axesMapping, buttonsMapping);
					}
				}
				ImGui::SameLine();
				if(ImGui::Button("Save...")) {
					std::string outputPath;
					const bool res = System::showPicker(System::Picker::Save, "", outputPath);
					if(res && !outputPath.empty()) {
						const Controller * controller = Input::manager().controller();
						Controller::saveConfiguration(outputPath, controller->guid(), controller->name(), axesMapping, buttonsMapping);
					}
				}
				ImGui::Separator();

				// Infos on the controller.
				RawController * controller = dynamic_cast<RawController *>(Input::manager().controller());
				const int axesCount		   = int(controller->allAxes.size());
				const int buttonsCount	 = int(controller->allButtons.size());
				ImGui::Text("%s, id: %d, axes: %d, buttons: %d", controller->name().c_str(), controller->id(), axesCount, buttonsCount);

				// Display raw axes and buttons and update their display when the user interacts with them.
				if(ImGui::CollapsingHeader("Raw inputs##HEADER", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::Columns(2);

					for(int aid = 0; aid < axesCount; ++aid) {
						const std::string axisName = "A" + std::to_string(aid);
						ImGui::SliderFloat(axisName.c_str(), &controller->allAxes[aid], -1.0f, 1.0f);
						ImGui::NextColumn();
					}
					ImGui::Columns(1);
					ImGui::Separator();
					ImGui::Columns(10);
					for(int bid = 0; bid < buttonsCount; ++bid) {
						const std::string buttonName = "B" + std::to_string(bid);
						ImGui::RadioButton(buttonName.c_str(), controller->allButtons[bid].pressed);
						ImGui::NextColumn();
					}
					ImGui::Columns(1);
				}

				if(ImGui::CollapsingHeader("Assignment##HEADER", ImGuiTreeNodeFlags_DefaultOpen)) {
					// Display the controller layout, highlight pressed buttons.
					ImGui::BeginChild("##ControllerLayout", ImVec2(450, 300));
					// Get current rnedering position on screen.
					const ImVec2 pos	  = ImGui::GetCursorScreenPos();
					ImDrawList * drawList = ImGui::GetWindowDrawList();

					// Render the left pad first.
					const int aidLX   = axesMapping[Controller::PadLeftX];
					const int aidLY   = axesMapping[Controller::PadLeftY];
					const float magLX = aidLX >= 0 ? controller->allAxes[aidLX] : 0.0f;
					const float magLY = aidLY >= 0 ? controller->allAxes[aidLY] : 0.0f;
					if((aidLX >= 0 || aidLY >= 0) && (magLX * magLX + magLY * magLY > threshold)) {
						drawList->AddCircleFilled(ImVec2(pos.x + 154, pos.y + 179), 34, highlightColor);
						drawList->AddCircleFilled(ImVec2(pos.x + 154, pos.y + 179), 26, IM_COL32(0, 0, 0, 255));
					}

					// Then the right pad.
					const int aidRX   = axesMapping[Controller::PadRightX];
					const int aidRY   = axesMapping[Controller::PadRightY];
					const float magRX = aidRX >= 0 ? controller->allAxes[aidRX] : 0.0f;
					const float magRY = aidRY >= 0 ? controller->allAxes[aidRY] : 0.0f;
					if((aidRX >= 0 || aidRY >= 0) && (magRX * magRX + magRY * magRY > threshold)) {
						drawList->AddCircleFilled(ImVec2(pos.x + 296, pos.y + 179), 34, highlightColor);
						drawList->AddCircleFilled(ImVec2(pos.x + 296, pos.y + 179), 26, IM_COL32(0, 0, 0, 255));
					}

					// Render the left trigger (assuming its default value is -1.0).
					const int aidLT   = axesMapping[Controller::TriggerL2];
					const float magLT = aidLT >= 0 ? controller->allAxes[aidLT] * 0.5f + 0.5f : 0.0f;
					if(aidLT >= 0 && (magLT * magLT > threshold)) {
						drawButton(drawList, Controller::TriggerL2, pos, highlightColor);
					}
					// And the right trigger (assuming its default value is -1.0).
					const int aidRT   = axesMapping[Controller::TriggerR2];
					const float magRT = aidRT >= 0 ? controller->allAxes[aidRT] * 0.5f + 0.5f : 0.0f;
					if(aidRT >= 0 && (magRT * magRT > threshold)) {
						drawButton(drawList, Controller::TriggerR2, pos, highlightColor);
					}

					// Render each button if active.
					for(int bid = 0; bid < int(buttonsMapping.size()); ++bid) {
						const int bmid = buttonsMapping[bid];
						if(bmid >= 0 && controller->allButtons[bmid].pressed) {
							drawButton(drawList, Controller::Input(bid), pos, highlightColor);
						}
					}

					// Overlay the controller transparent texture.
					ImGui::Image(*controllerTex, ImVec2(450, 300), ImVec2(0, 1), ImVec2(1, 0));

					ImGui::EndChild();
					ImGui::SameLine();

					// Display combo selectors to assign raw input to each button.
					ImGui::BeginChild("##Layout selection", ImVec2(0, 300));
					ImGui::PushItemWidth(80);
					const float spacing = 160.0f;

					showCombo("A", buttonsCount, "B", buttonsMapping[Controller::ButtonA]);
					ImGui::SameLine(spacing);
					showCombo("B", buttonsCount, "B", buttonsMapping[Controller::ButtonB]);
					showCombo("X", buttonsCount, "B", buttonsMapping[Controller::ButtonX]);
					ImGui::SameLine(spacing);
					showCombo("Y", buttonsCount, "B", buttonsMapping[Controller::ButtonY]);

					showCombo("Up", buttonsCount, "B", buttonsMapping[Controller::ButtonUp]);
					ImGui::SameLine(spacing);
					showCombo("Left", buttonsCount, "B", buttonsMapping[Controller::ButtonLeft]);
					showCombo("Down", buttonsCount, "B", buttonsMapping[Controller::ButtonDown]);
					ImGui::SameLine(spacing);
					showCombo("Right", buttonsCount, "B", buttonsMapping[Controller::ButtonRight]);

					showCombo("L1", buttonsCount, "B", buttonsMapping[Controller::BumperL1]);
					ImGui::SameLine(spacing);
					showCombo("R1", buttonsCount, "B", buttonsMapping[Controller::BumperR1]);
					showCombo("L2", buttonsCount, "B", buttonsMapping[Controller::TriggerL2]);
					ImGui::SameLine(spacing);
					showCombo("R2", buttonsCount, "B", buttonsMapping[Controller::TriggerR2]);
					showCombo("L3", buttonsCount, "B", buttonsMapping[Controller::ButtonL3]);
					ImGui::SameLine(spacing);
					showCombo("R3", buttonsCount, "B", buttonsMapping[Controller::ButtonR3]);

					showCombo("Menu", buttonsCount, "B", buttonsMapping[Controller::ButtonMenu]);
					ImGui::SameLine(spacing);
					showCombo("View", buttonsCount, "B", buttonsMapping[Controller::ButtonView]);
					showCombo("Logo", buttonsCount, "B", buttonsMapping[Controller::ButtonLogo]);

					ImGui::Separator();
					showCombo("Left X", axesCount, "A", axesMapping[Controller::PadLeftX]);
					ImGui::SameLine(spacing);
					showCombo("Left Y", axesCount, "A", axesMapping[Controller::PadLeftY]);
					showCombo("Right X", axesCount, "A", axesMapping[Controller::PadRightX]);
					ImGui::SameLine(spacing);
					showCombo("Right Y", axesCount, "A", axesMapping[Controller::PadRightY]);
					showCombo("L. trigger", axesCount, "A", axesMapping[Controller::TriggerL2]);
					ImGui::SameLine(spacing);
					showCombo("R. trigger", axesCount, "A", axesMapping[Controller::TriggerR2]);

					ImGui::PopItemWidth();
					
					ImGui::EndChild();
				}
				// Display targets with the current axis positions.
				if(ImGui::CollapsingHeader("Calibration##HEADER", ImGuiTreeNodeFlags_DefaultOpen)) {
					
					const float threshRadius = sqrt(threshold) * 100;
					
					// Titles.
					ImGui::Text("Threshold");
					ImGui::SameLine(100);
					ImGui::Text("Left pad & trigger");
					ImGui::SameLine(300+100);
					ImGui::Text("Right pad & trigger");

					// Add threshold setup slider.
					ImGui::VSliderFloat("##threshold", ImVec2(50, 200), &threshold, 0.0f, 1.0f, "%.3f");
					ImGui::SameLine(100);
					
					// Left pad.
					ImGui::BeginChild("PadLeftTarget", ImVec2(200, 200));
					drawPadTarget(axesMapping[Controller::PadLeftX], axesMapping[Controller::PadLeftY], controller->allAxes, threshRadius);
					ImGui::EndChild();
					ImGui::SameLine(320);
					// Left trigger
					ImGui::BeginChild("TriggerL2", ImVec2(40, 200));
					drawTriggerTarget(axesMapping[Controller::TriggerL2], controller->allAxes, threshRadius);
					ImGui::EndChild();

					ImGui::SameLine(400);
					// Right pad.
					ImGui::BeginChild("PadRightTarget", ImVec2(200, 200));
					drawPadTarget(axesMapping[Controller::PadRightX], axesMapping[Controller::PadRightY], controller->allAxes, threshRadius);
					ImGui::EndChild();

					// Right trigger
					ImGui::SameLine(620);
					ImGui::BeginChild("TriggerR2", ImVec2(40, 200));
					drawTriggerTarget(axesMapping[Controller::TriggerR2], controller->allAxes, threshRadius);
					ImGui::EndChild();
					
					
				}
			}
		}
		ImGui::End();
	}

	// Clean up.
	Resources::manager().clean();
	
	return 0;
}

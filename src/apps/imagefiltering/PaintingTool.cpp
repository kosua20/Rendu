#include "PaintingTool.hpp"

#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"

PaintingTool::PaintingTool(unsigned int width, unsigned int height) {

	_brushShader = Resources::manager().getProgram("brush_color");
	_canvas		 = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, {Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, false));
	_visu		 = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, {Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, false));

	// Setup the mesh buffers.
	_brushes.resize(int(Shape::COUNT));

	// Generate a disk mesh.
	Mesh & disk				  = _brushes[0];
	const int diskResolution  = 360;
	const float radResolution = glm::two_pi<float>() / float(diskResolution);
	disk.positions.resize(diskResolution + 1);
	disk.indices.resize(diskResolution * 3);
	// Generate positions around the circle.
	disk.positions[0] = glm::vec3(0.0f);
	for(int i = 1; i <= diskResolution; ++i) {
		const float angle = float(i - 1) * radResolution;
		disk.positions[i] = glm::vec3(std::sin(angle), std::cos(angle), 0.0f);
	}
	// Generate faces forming a fan.
	for(int i = 1; i <= diskResolution; ++i) {
		const int baseId		 = 3 * (i - 1);
		disk.indices[baseId]	 = 0;
		disk.indices[baseId + 1] = i == diskResolution ? 1 : i + 1;
		disk.indices[baseId + 2] = i;
	}
	disk.upload();
	disk.clearGeometry();

	// Generate a square mesh.
	Mesh & square	= _brushes[1];
	square.positions = {{0.0f, 0.0f, 0.0f}, {-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}};
	square.indices   = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
	square.upload();
	square.clearGeometry();

	// Generate a diamond mesh.
	Mesh & diamond	= _brushes[2];
	diamond.positions = {{0.0f, 0.0f, 0.0f}, {-1.41f, 0.0f, 0.0f}, {0.0f, -1.41f, 0.0f}, {1.41f, 0.0f, 0.0f}, {0.0f, 1.41f, 0.0f}};
	diamond.indices   = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
	diamond.upload();
	diamond.clearGeometry();

	checkGLError();
}

void PaintingTool::draw() {

	glDisable(GL_DEPTH_TEST);
	_canvas->bind();
	_canvas->setViewport();

	// Clear if needed.
	if(_shoudClear) {
		_shoudClear = false;
		GLUtilities::clearColor(glm::vec4(_bgColor, 1.0f));
	}

	// Draw brush if needed.
	const float radiusF = float(_radius);
	if(_shouldDraw) {
		_shouldDraw			  = false;
		const glm::vec3 color = _mode == Mode::DRAW ? _fgColor : _bgColor;
		const glm::vec2 radii(radiusF / float(_canvas->width()), radiusF / float(_canvas->height()));

		_brushShader->use();
		_brushShader->uniform("position", _drawPos);
		_brushShader->uniform("radius", radii);
		_brushShader->uniform("outline", 0);
		_brushShader->uniform("color", color);
		GLUtilities::drawMesh(_brushes[int(_shape)]);
	}

	_canvas->unbind();

	// Copy the canvas to the visualisation framebuffer.
	GLUtilities::blit(*_canvas, *_visu, Filter::NEAREST);

	// Draw the brush outline.
	_visu->bind();
	_visu->setViewport();
	_brushShader->use();

	const glm::vec2 radii(radiusF / float(_canvas->width()), radiusF / float(_canvas->height()));
	const glm::vec3 white(1.0f);

	_brushShader->uniform("position", _drawPos);
	_brushShader->uniform("radius", radii);
	_brushShader->uniform("outline", 1);
	_brushShader->uniform("radiusPx", radiusF);
	_brushShader->uniform("color", white);
	GLUtilities::drawMesh(_brushes[int(_shape)]);
	_visu->unbind();
}

void PaintingTool::update() {

	// If right-pressing, read back the color under the cursor.
	if(Input::manager().pressed(Input::Mouse::Right)) {
		// Pixel position in the framebuffer.
		const unsigned int w	  = _canvas->width();
		const unsigned int h	  = _canvas->height();
		const glm::vec2 pos		  = Input::manager().mouse();
		glm::vec2 mousePositionGL = glm::floor(glm::vec2(pos.x * float(w), (1.0f - pos.y) * float(h)));
		mousePositionGL			  = glm::clamp(mousePositionGL, glm::vec2(0.0f), glm::vec2(w, h));
		// Read back from the framebuffer.
		_fgColor = _canvas->read(glm::ivec2(mousePositionGL));
	}

	// If left-pressing, draw to the canvas.
	_drawPos   = 2.0f * Input::manager().mouse() - 1.0f;
	_drawPos.y = -_drawPos.y;
	if(Input::manager().pressed(Input::Mouse::Left)) {
		_shouldDraw = true;
	}

	// If scroll, adjust radius.
	_radius = std::max(1, int(std::round(float(_radius) - Input::manager().scroll()[1])));

	// Interface window.
	if(ImGui::Begin("Canvas")) {
		// Brush mode.
		ImGui::RadioButton("Draw", reinterpret_cast<int *>(&_mode), 0);
		ImGui::SameLine();
		ImGui::RadioButton("Erase", reinterpret_cast<int *>(&_mode), 1);
		// Brush shape.
		ImGui::PushItemWidth(100);
		ImGui::Combo("Shape", reinterpret_cast<int *>(&_shape), "Circle\0Square\0Losange\0\0");
		// Brush radius.
		if(ImGui::InputInt("Radius", &_radius, 1, 5)) {
			_radius = std::max(1, _radius);
		}
		ImGui::PopItemWidth();
		ImGui::Separator();
		// Colors.
		ImGui::PushItemWidth(120);
		ImGui::ColorEdit3("Foreground", &_fgColor[0]);
		ImGui::ColorEdit3("Background", &_bgColor[0]);
		if(ImGui::Button("Clear")) {
			_shoudClear = true;
		}
		ImGui::PopItemWidth();
	}
	ImGui::End();
}

void PaintingTool::clean() const {
	_canvas->clean();
	_visu->clean();
}

void PaintingTool::resize(unsigned int width, unsigned int height) const {
	// We first copy the canvas to a temp framebuffer.
	const unsigned int w = _canvas->width();
	const unsigned int h = _canvas->height();
	Framebuffer tempCanvas(w, h, {Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, false);
	_canvas->bind(Framebuffer::Mode::READ);
	tempCanvas.bind(Framebuffer::Mode::WRITE);

	GLUtilities::blit(*_canvas, tempCanvas, Filter::NEAREST);

	// We can then resize the canvas.
	_canvas->resize(width, height);
	// Clean up the canvas.
	_canvas->bind();
	GLUtilities::clearColor(glm::vec4(_bgColor, 1.0f));
	_canvas->unbind();

	// Copy back the drawing.
	GLUtilities::blit(tempCanvas, *_canvas, Filter::NEAREST);
	tempCanvas.clean();

	// The content of the visualisation buffer will be cleaned at the next frame canvas copy.
	_visu->resize(width, height);

	checkGLError();
}

#include "PaintingTool.hpp"

#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"

PaintingTool::PaintingTool(unsigned int width, unsigned int height) {
	
	_bgColor = glm::vec3(0.0f);
	_fgColor = glm::vec3(1.0f);
	
	_brushShader = Resources::manager().getProgram("brush_color");
	_canvas = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, { Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, false));
	_visu = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, { Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, false));
	
	// Setup the mesh buffers.
	_brushes.resize(int(Shape::COUNT));
	
	// Generate a disk mesh.
	Mesh & disk = _brushes[0];
	const int diskResolution = 360;
	const float radResolution = float(M_PI) * 2.0f / float(diskResolution);
	disk.positions.resize(diskResolution+1);
	disk.indices.resize(diskResolution*3);
	// Generate positions around the circle.
	disk.positions[0] = glm::vec3(0.0f);
	for(int i = 1; i <= diskResolution; ++i){
		const float angle = float(i-1) * radResolution;
		disk.positions[i] = glm::vec3(std::sin(angle), std::cos(angle), 0.0f);
	}
	// Generate faces forming a fan.
	for(int i = 1; i <= diskResolution; ++i){
		const int baseId = 3*(i-1);
		disk.indices[baseId  ] = 0;
		disk.indices[baseId+1] = (i==diskResolution) ? 1 : (i+1);
		disk.indices[baseId+2] = i;
	}
	disk.upload();
	disk.clearGeometry();
	
	// Generate a square mesh.
	Mesh & square = _brushes[1];
	square.positions = {{0.0f, 0.0f, 0.0f}, {-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}};
	square.indices = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
	square.upload();
	square.clearGeometry();
	
	// Generate a diamond mesh.
	Mesh & diamond = _brushes[2];
	diamond.positions = {{0.0f, 0.0f, 0.0f}, {-1.41f, 0.0f, 0.0f}, {0.0f, -1.41f, 0.0f}, {1.41f, 0.0f, 0.0f}, {0.0f, 1.41f, 0.0f}};
	diamond.indices = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
	diamond.upload();
	diamond.clearGeometry();
	
	checkGLError();
}

void PaintingTool::draw() {
	
	glDisable(GL_DEPTH_TEST);
	_canvas->bind();
	_canvas->setViewport();
	
	// Clear if needed.
	if(_shoudClear){
		_shoudClear = false;
		glClearColor(_bgColor[0], _bgColor[1], _bgColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	
	// Draw brush if needed.
	const float radiusF = float(_radius);
	if(_shouldDraw){
		_shouldDraw = false;
		const glm::vec3 color = _mode == Mode::DRAW ? _fgColor : _bgColor;
		const glm::vec2 radii(radiusF/_canvas->width(), radiusF/_canvas->height());
		
		_brushShader->use();
		_brushShader->uniform("position", _drawPos);
		_brushShader->uniform("radius", radii);
		_brushShader->uniform("outline", 0);
		_brushShader->uniform("color", color);
		GLUtilities::drawMesh(_brushes[int(_shape)]);
		
	}
	
	_canvas->unbind();
	
	// Copy the canvas to the visualisation framebuffer.
	_canvas->bind(Framebuffer::Mode::READ);
	_visu->bind(Framebuffer::Mode::WRITE);
	glBlitFramebuffer(0, 0, _canvas->width(), _canvas->height(), 0, 0, _visu->width(), _visu->height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// Draw the brush outline.
	_visu->bind();
	_visu->setViewport();
	_brushShader->use();
	
	const glm::vec2 radii(radiusF/_canvas->width(), radiusF/_canvas->height());
	const glm::vec3 white(1.0f);
	
	_brushShader->uniform("position", _drawPos);
	_brushShader->uniform("radius", radii);
	_brushShader->uniform("outline", 1);
	_brushShader->uniform("radiusPx", radiusF);
	_brushShader->uniform("color", white);
	GLUtilities::drawMesh(_brushes[int(_shape)]);
	_visu->unbind();
}

void PaintingTool::update(){
	
	// If right-pressing, read back the color under the cursor.
	if(Input::manager().pressed(Input::Mouse::Right)){
		// Pixel position in the framebuffer.
		const unsigned int w = _canvas->width();
		const unsigned int h = _canvas->height();
		const glm::vec2 pos = Input::manager().mouse();
		glm::vec2 mousePositionGL = glm::floor(glm::vec2(pos.x * w, (1.0f-pos.y) * h));
		mousePositionGL = glm::clamp(mousePositionGL, glm::vec2(0.0f), glm::vec2(w, h));
		// Read back from the framebuffer.
		_canvas->bind(Framebuffer::Mode::READ);
		glReadPixels(int(mousePositionGL.x), int(mousePositionGL.y), 1, 1, GL_RGB, GL_FLOAT, &_fgColor[0]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	
	// If left-pressing, draw to the canvas.
	_drawPos = 2.0f * Input::manager().mouse() - 1.0f;
	_drawPos.y = -_drawPos.y;
	if(Input::manager().pressed(Input::Mouse::Left)){
		_shouldDraw = true;
	}
	
	// If scroll, adjust radius.
	_radius = std::max(1, int(std::round(_radius - Input::manager().scroll()[1])));
	
	// Interface window.
	if(ImGui::Begin("Canvas")){
		// Brush mode.
		ImGui::RadioButton("Draw", (int*)&_mode, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Erase", (int*)&_mode, 1);
		// Brush shape.
		ImGui::PushItemWidth(100);
		ImGui::Combo("Shape", (int*)&_shape, "Circle\0Square\0Losange\0\0");
		// Brush radius.
		if(ImGui::InputInt("Radius", &_radius, 1, 5)){
			_radius = std::max(1, _radius);
		}
		ImGui::PopItemWidth();
		ImGui::Separator();
		// Colors.
		ImGui::PushItemWidth(120);
		ImGui::ColorEdit3("Foreground", &_fgColor[0]);
		ImGui::ColorEdit3("Background", &_bgColor[0]);
		if(ImGui::Button("Clear")){
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


void PaintingTool::resize(unsigned int width, unsigned int height){
	// We first copy the canvas to a temp framebuffer.
	const unsigned int w = _canvas->width();
	const unsigned int h = _canvas->height();
	Framebuffer tempCanvas(w, h, { Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, false);
	_canvas->bind(Framebuffer::Mode::READ);
	tempCanvas.bind(Framebuffer::Mode::WRITE);
	glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
	// We can then resize the canvas.
	_canvas->resize(width, height);
	// Clean up the canvas.
	_canvas->bind();
	glClearColor(_bgColor[0], _bgColor[1], _bgColor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	_canvas->unbind();
	
	// Copy back the drawing.
	tempCanvas.bind(Framebuffer::Mode::READ);
	_canvas->bind(Framebuffer::Mode::WRITE);
	const unsigned int nw = std::min(w, _canvas->width());
	const unsigned int nh = std::min(h, _canvas->height());
	glBlitFramebuffer(0, 0, nw, nh, 0, 0, nw, nh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	tempCanvas.clean();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// The content of the visualisation buffer will be cleaned at the next frame canvas copy.
	_visu->resize(width, height);
	
	checkGLError();
}

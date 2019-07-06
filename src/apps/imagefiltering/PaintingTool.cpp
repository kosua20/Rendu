#include "PaintingTool.hpp"

#include "input/Input.hpp"
#include "helpers/System.hpp"

PaintingTool::PaintingTool(unsigned int width, unsigned int height) {
	
	_bgColor = glm::vec3(0.0f);
	_fgColor = glm::vec3(1.0f);
	
	_brushShader = Resources::manager().getProgram("brush_color");
	_canvas = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, {GL_RGB8, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE}, false));
	_visu = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, {GL_RGB8, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE}, false));
	
	// Generate a disk mesh.
	Mesh disk;
	const int diskResolution = 360;
	const float radResolution = M_PI * 2.0f / float(diskResolution);
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
	
	// Generate a square mesh.
	Mesh square;
	square.positions = {{0.0f, 0.0f, 0.0f}, {-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}};
	square.indices = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
	
	// Generate a diamond mesh.
	Mesh diamond;
	diamond.positions = {{0.0f, 0.0f, 0.0f}, {-1.41f, 0.0f, 0.0f}, {0.0f, -1.41f, 0.0f}, {1.41f, 0.0f, 0.0f}, {0.0f, 1.41f, 0.0f}};
	diamond.indices = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
	
	// Setup the mesh buffers.
	_brushes.resize(int(Shape::COUNT));
	_brushes[0] = GLUtilities::setupBuffers(disk);
	_brushes[1] = GLUtilities::setupBuffers(square);
	_brushes[2] = GLUtilities::setupBuffers(diamond);
	
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
		
		glUseProgram(_brushShader->id());
		glUniform2fv(_brushShader->uniform("position"), 1, &_drawPos[0]);
		glUniform2f(_brushShader->uniform("radius"), radiusF/_canvas->width(), radiusF/_canvas->height());
		glUniform1i(_brushShader->uniform("outline"), 0);
		glUniform3fv(_brushShader->uniform("color"), 1, &color[0]);
		GLUtilities::drawMesh(_brushes[int(_shape)]);
		glUseProgram(0);
	}
	
	_canvas->unbind();
	
	// Copy the canvas to the visualisation framebuffer.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _canvas->id());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _visu->id());
	glBlitFramebuffer(0, 0, _canvas->width(), _canvas->height(), 0, 0, _visu->width(), _visu->height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// Draw the brush outline.
	_visu->bind();
	_visu->setViewport();
	glUseProgram(_brushShader->id());
	glUniform2fv(_brushShader->uniform("position"), 1, &_drawPos[0]);
	glUniform2f(_brushShader->uniform("radius"), radiusF/_canvas->width(), radiusF/_canvas->height());
	glUniform1i(_brushShader->uniform("outline"), 1);
	glUniform1f(_brushShader->uniform("radiusPx"), radiusF);
	glUniform3f(_brushShader->uniform("color"), 1.0f, 1.0f, 1.0f);
	GLUtilities::drawMesh(_brushes[int(_shape)]);
	glUseProgram(0);
	_visu->unbind();
}

void PaintingTool::update(){
	
	// If right-pressing, read back the color under the cursor.
	if(Input::manager().pressed(Input::MouseRight)){
		// Pixel position in the framebuffer.
		const unsigned int w = _canvas->width();
		const unsigned int h = _canvas->height();
		const glm::vec2 pos = Input::manager().mouse();
		glm::vec2 mousePositionGL = glm::floor(glm::vec2(pos.x * w, (1.0f-pos.y) * h));
		mousePositionGL = glm::clamp(mousePositionGL, glm::vec2(0.0f), glm::vec2(w, h));
		// Read back from the framebuffer.
		glBindFramebuffer(GL_READ_FRAMEBUFFER, _canvas->id());
		glReadPixels(int(mousePositionGL.x), int(mousePositionGL.y), 1, 1, GL_RGB, GL_FLOAT, &_fgColor[0]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	
	// If left-pressing, draw to the canvas.
	_drawPos = 2.0f * Input::manager().mouse() - 1.0f;
	_drawPos.y = -_drawPos.y;
	if(Input::manager().pressed(Input::MouseLeft)){
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
	Framebuffer tempCanvas(w, h,  {GL_RGB8, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE}, false);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _canvas->id());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempCanvas.id());
	glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
	// We can then resize the canvas.
	_canvas->resize(width, height);
	// Clean up the canvas.
	_canvas->bind();
	glClearColor(_bgColor[0], _bgColor[1], _bgColor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	_canvas->unbind();
	
	// Copy back the drawing.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tempCanvas.id());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _canvas->id());
	const unsigned int nw = std::min(w, _canvas->width());
	const unsigned int nh = std::min(h, _canvas->height());
	glBlitFramebuffer(0, 0, nw, nh, 0, 0, nw, nh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	tempCanvas.clean();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// The content of the visualisation buffer will be cleaned at the next frame canvas copy.
	_visu->resize(width, height);
	
	checkGLError();
}

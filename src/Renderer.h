#include <GLFW/glfw3.h> // to set up the OpenGL context and manage window lifecycle and inputs

class Renderer {

public:

Renderer();

~Renderer();

/// Init function
void init(int width, int height);

/// Draw function
void draw();

/// Clean function
void clean();

/// Handle screen resizing
void resize(int width, int height);

/// Handle keyboard inputs
void keyPressed(int key, int action);

/// Handle mouse inputs
void buttonPressed(int button, int action);

private:

int _width;
int _height;

};



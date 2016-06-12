#include <stdio.h>
#include <iostream>

#include "helpers/ProgramUtilities.h"
#include "Framebuffer.h"

Framebuffer::Framebuffer() : _width(0),  _height(0){}

Framebuffer::Framebuffer(int width, int height) : _width(width),  _height(height){}

Framebuffer::~Framebuffer(){ clean(); }

void Framebuffer::bind(){
	
}

void Framebuffer::unbind(){
	
}

void Framebuffer::setup(){
	
}

void Framebuffer::resize(int width, int height){
	
}

void Framebuffer::clean(){
	
}


#ifndef METAL_WINDOW_H
#define METAL_WINDOW_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>

#include "RenderDoos/metal/Metal.hpp"

class Fl_Metal_Window : public Fl_Window
{
public:
  explicit Fl_Metal_Window(int x, int y, int w, int h, const char* t);
  virtual ~Fl_Metal_Window();
  
  void prepare();
  
  void draw();
  
  virtual void redraw();
  
  virtual void flush();
  
private:
  
  virtual void _draw(MTL::Device* device, CA::MetalDrawable* drawable) = 0;
  
private:
  bool _initialised;
};

#endif

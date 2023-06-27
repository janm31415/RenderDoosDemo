#import <Cocoa/Cocoa.h>
#include <FL/x.H>
#include <FL/Fl_Window.H>

#import <MetalKit/MetalKit.h>

#include "Fl_Metal_Window.h"

Fl_Metal_Window::Fl_Metal_Window(int x, int y, int w, int h, const char* t) : Fl_Window(x, y, w, h, t),
_initialised(false)
{
  box(FL_NO_BOX);
  end();
}

Fl_Metal_Window::~Fl_Metal_Window()
{
}

void Fl_Metal_Window::prepare()
{
  NSView *view = [fl_xid(this) contentView];
  NSRect f = view.frame;
  view.layer = [CAMetalLayer layer];
  view.layer.frame = f;
  CAMetalLayer* metal_layer = (CAMetalLayer*)view.layer;
  metal_layer.device = ::MTLCreateSystemDefaultDevice();
  MTL::Device* device = ( __bridge MTL::Device* )metal_layer.device;
  _initialised = true;
}

void Fl_Metal_Window::draw()
{
  if (!_initialised)
    prepare();
  NSView *view = [fl_xid(this) contentView];
  CAMetalLayer* metal_layer = (CAMetalLayer*)view.layer;
  id<CAMetalDrawable> currentDrawable = [metal_layer nextDrawable];
  MTL::Drawable* pMetalCppDrawable = (__bridge MTL::Drawable*) currentDrawable;
  id<MTLTexture> metalTexture = [currentDrawable texture];
  MTL::Texture* pMetalCppTexture = (__bridge MTL::Texture*) metalTexture;
  _draw(( __bridge MTL::Device* )metal_layer.device, pMetalCppDrawable, pMetalCppTexture);
}

void Fl_Metal_Window::redraw()
  {
  damage(FL_DAMAGE_ALL);
  }

void Fl_Metal_Window::flush()
{
  draw();
}

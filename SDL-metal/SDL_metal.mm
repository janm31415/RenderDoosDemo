#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

#include "SDL_metal.h"


void assign_device(void* layer, MTL::Device* device)
  {
  CAMetalLayer* metalLayer = (CAMetalLayer*)layer;
  metalLayer.device = (__bridge id<MTLDevice>)(device);
  }
  
MTLData next_drawable(void* layer)
  {
  MTLData d;
  CAMetalLayer* metalLayer = (CAMetalLayer*)layer;
  id<CAMetalDrawable> metalDrawable = [metalLayer nextDrawable];
  MTL::Drawable* pMetalCppDrawable = (__bridge MTL::Drawable*) metalDrawable;
  d.drawable = pMetalCppDrawable;
  id<MTLTexture> metalTexture = [metalDrawable texture];
  MTL::Texture* pMetalCppTexture = (__bridge MTL::Texture*) metalTexture;
  d.texture = pMetalCppTexture;
  return d;
  }

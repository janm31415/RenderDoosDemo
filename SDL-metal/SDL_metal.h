#pragma once

#include "metal/Metal.hpp"

void assign_device(void* layer, MTL::Device* device);

struct MTLData
  {
  MTL::Drawable* drawable;
  MTL::Texture* texture;
  };
  
MTLData next_drawable(void* layer);

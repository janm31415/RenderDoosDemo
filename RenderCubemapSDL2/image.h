#pragma once
#include <stdint.h>
#include <string>

struct rgba_image
  {
  rgba_image();
  ~rgba_image();

  int w, h;
  uint32_t* im;
  };


bool read_image_from_file(rgba_image& rgba, const std::string& filename);
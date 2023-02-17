#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

rgba_image::rgba_image() : im(nullptr)
  {
  }

rgba_image::~rgba_image()
  {  
  delete [] im;
  }

bool read_image_from_file(rgba_image& rgba, const std::string& filename)
  {
  int imw, imh, nr_of_channels;
  unsigned char* im = stbi_load(filename.c_str(), &imw, &imh, &nr_of_channels, 4);
  if (im == nullptr)
    return false;
  rgba.w = imw;
  rgba.h = imh;
  rgba.im = new uint32_t[imw * imh];
  memcpy(rgba.im, im, imw * imh * 4);  
  stbi_image_free(im);
  return true;
  }
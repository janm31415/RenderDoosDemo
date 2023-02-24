#pragma once

#include "RenderDoos/material.h"

#include "ft2build.h"
#include FT_FREETYPE_H

typedef struct text_vert_t {
  float x;
  float y;
  float s;
  float t;
  float r;
  float g;
  float b;
  } text_vert_t;

// Structure to hold cache glyph information
typedef struct char_info_t {
  float ax; // advance.x
  float ay; // advance.y

  float bw; // bitmap.width
  float bh; // bitmap.height

  float bl; // bitmap left
  float bt; // bitmap top

  float tx; // x offset of glyph in texture coordinates
  float ty; // y offset of glyph in texture coordinates
  } char_info_t;

class font_material : public RenderDoos::material
  {
  public:
    font_material();
    virtual ~font_material();

    virtual void compile(RenderDoos::render_engine* engine);
    virtual void bind(RenderDoos::render_engine* engine);
    virtual void destroy(RenderDoos::render_engine* engine);

    void render_text(RenderDoos::render_engine* engine, const char* text, float x, float y, float sx, float sy, uint32_t clr);

  private:

    void _init_font(RenderDoos::render_engine* engine);

  private:
    int32_t vs_handle, fs_handle;
    int32_t shader_program_handle;
    int32_t width_handle, height_handle;
    int32_t geometry_id;

    int32_t atlas_texture_id;

    int32_t atlas_width;
    int32_t atlas_height;
    char_info_t char_info[128];

    FT_Library _ft;
    FT_Face _face;
  };

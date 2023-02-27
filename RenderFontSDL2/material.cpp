#include "material.h"
#include "RenderDoos/types.h"
#include "RenderDoos/render_context.h"
#include "RenderDoos/render_engine.h"

#include <vector>

#define MAX_WIDTH 2048 // Maximum texture width on pi

static std::string get_font_material_vertex_shader()
  {
  return std::string(R"(#version 330 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 color;

out vec2 frag_tex_coord;
out vec3 text_color;

void main() {
    frag_tex_coord = uv;
    //frag_tex_coord.y = 1-frag_tex_coord.y;
    text_color = color;
    gl_Position = vec4(pos, 0, 1);
}
)");
  }

static std::string get_font_material_fragment_shader()
  {
  return std::string(R"(#version 430 core
in vec2 frag_tex_coord;
in vec3 text_color;

out vec4 outColor;

uniform int width;
uniform int height;

layout(r8ui, binding = 0) readonly uniform uimage2D font_texture;

void main() {
    int x = int(frag_tex_coord.x * float(width));
    int y = int(frag_tex_coord.y * float(height));
    uint a = imageLoad(font_texture, ivec2(x, y)).r;
    outColor = vec4(1, 1, 1, float(a)/255.0)*vec4(text_color, 1);
}
)");
  }

font_material::font_material()
  {
  vs_handle = -1;
  fs_handle = -1;
  shader_program_handle = -1;
  width_handle = -1;
  height_handle = -1;
  geometry_id = -1;
  atlas_texture_id = -1;
  }

font_material::~font_material()
  {
  FT_Done_Face(_face);
  FT_Done_FreeType(_ft);
  }

void font_material::_init_font(RenderDoos::render_engine* engine)
  {
  if (FT_Init_FreeType(&_ft)) {
    printf("Error initializing FreeType library\n");
    exit(EXIT_FAILURE);
    }

  if (FT_New_Face(_ft, "data/Karla-Regular.ttf", 0, &_face)) {
    printf("Error loading font face\n");
    exit(EXIT_FAILURE);
    }

  // Set pixel size
  FT_Set_Pixel_Sizes(_face, 0, 48);

  // Get atlas dimensions
  FT_GlyphSlot g = _face->glyph;
  int w = 0; // full texture width
  int h = 0; // full texture height
  int row_w = 0; // current row width
  int row_h = 0; // current row height

  int i;
  for (i = 32; i < 128; ++i)
    {
    if (FT_Load_Char(_face, i, FT_LOAD_RENDER))
      {
      printf("Loading Character %d failed\n", i);
      exit(EXIT_FAILURE);
      }

    // If the width will be over max texture width
    // Go to next row
    if (row_w + g->bitmap.width + 1 >= MAX_WIDTH)
      {
      w = std::max(w, row_w);
      h += row_h;
      row_w = 0;
      row_h = 0;
      }
    row_w += g->bitmap.width + 1;
    row_h = std::max<unsigned int>(row_h, g->bitmap.rows);
    }

  // final texture dimensions
  w = std::max(row_w, w);
  h += row_h;

  atlas_width = w;
  atlas_height = h;

  uint8_t* raw_bitmap = new uint8_t[w * h];

  // Fill texture with glyph bitmaps and cache placements
  int offset_x = 0;
  int offset_y = 0;
  row_h = 0;

  for (i = 32; i < 128; ++i)
    {
    if (FT_Load_Char(_face, i, FT_LOAD_RENDER))
      {
      printf("Loading Character %d failed\n", i);
      exit(EXIT_FAILURE);
      }

    // Set correct row
    if (offset_x + g->bitmap.width + 1 >= MAX_WIDTH)
      {
      offset_y += row_h;
      row_h = 0;
      offset_x = 0;
      }

    // fill raw bitmap with glyph
    for (unsigned int y = 0; y < g->bitmap.rows; ++y)
      {
      const unsigned char* p_bitmap = g->bitmap.buffer + y * g->bitmap.width;
      uint8_t* p_tex = raw_bitmap + (y + offset_y)*w + offset_x;
      for (unsigned int x = 0; x < g->bitmap.width; ++x)
        {
        *p_tex++ = *p_bitmap++;
        }
      }

    // Cache values
    char_info[i].ax = g->advance.x >> 6;
    char_info[i].ay = g->advance.y >> 6;
    char_info[i].bw = g->bitmap.width;
    char_info[i].bh = g->bitmap.rows;
    char_info[i].bl = g->bitmap_left;
    char_info[i].bt = g->bitmap_top;
    char_info[i].tx = offset_x / (float)w;
    char_info[i].ty = offset_y / (float)h;

    // Update current position
    row_h = std::max<unsigned int>(row_h, g->bitmap.rows);
    offset_x += g->bitmap.width + 1;
    }

  atlas_texture_id = engine->add_texture(w, h, RenderDoos::texture_format_r8ui, (const uint8_t*)raw_bitmap, TEX_USAGE_READ | TEX_USAGE_RENDER_TARGET);
  delete[] raw_bitmap;
  }

void font_material::compile(RenderDoos::render_engine* engine)
  {
  if (engine->get_renderer_type() == RenderDoos::renderer_type::METAL)
    {
    vs_handle = engine->add_shader(nullptr, SHADER_VERTEX, "font_material_vertex_shader");
    fs_handle = engine->add_shader(nullptr, SHADER_FRAGMENT, "font_material_fragment_shader");
    }
  else if (engine->get_renderer_type() == RenderDoos::renderer_type::OPENGL)
    {
    vs_handle = engine->add_shader(get_font_material_vertex_shader().c_str(), SHADER_VERTEX, nullptr);
    fs_handle = engine->add_shader(get_font_material_fragment_shader().c_str(), SHADER_FRAGMENT, nullptr);
    }
  shader_program_handle = engine->add_program(vs_handle, fs_handle);
  width_handle = engine->add_uniform("width", RenderDoos::uniform_type::integer, 1);
  height_handle = engine->add_uniform("height", RenderDoos::uniform_type::integer, 1);
  _init_font(engine);
  }

void font_material::bind(RenderDoos::render_engine* engine)
  {
  engine->set_blending_enabled(true);
  engine->set_blending_function(RenderDoos::blending_type::src_alpha, RenderDoos::blending_type::one_minus_src_alpha);
  engine->set_blending_equation(RenderDoos::blending_equation_type::add);

  engine->bind_program(shader_program_handle);

  engine->set_uniform(width_handle, (void*)&atlas_width);
  engine->set_uniform(height_handle, (void*)&atlas_height);
  
  engine->bind_texture_to_channel(atlas_texture_id, 0, TEX_FILTER_NEAREST | TEX_WRAP_REPEAT);

  engine->bind_uniform(shader_program_handle, width_handle);
  engine->bind_uniform(shader_program_handle, height_handle);
  }

void font_material::destroy(RenderDoos::render_engine* engine)
  {
  engine->remove_shader(vs_handle);
  engine->remove_shader(fs_handle);
  engine->remove_program(shader_program_handle);
  engine->remove_uniform(width_handle);
  engine->remove_uniform(height_handle);
  engine->remove_geometry(geometry_id);
  }

namespace
  {
  text_vert_t make_text_vert(float x, float y, float s, float t, float r, float g, float b)
    {
    text_vert_t out;
    out.x = x;
    out.y = y;
    out.s = s;
    out.t = t;
    out.r = r;
    out.g = g;
    out.b = b;
    return out;
    }
  }

void font_material::render_text(RenderDoos::render_engine* engine, const char* text, float x, float y, float sx, float sy, uint32_t clr)
  {
  if (geometry_id >= 0)
    engine->remove_geometry(geometry_id);

  const float x_orig = x;

  std::vector<text_vert_t> verts(6 * strlen(text));
  int n = 0;

  char_info_t* c = char_info;

  float red = (clr & 255)/255.f;
  float green = ((clr>>8) & 255) / 255.f;
  float blue = ((clr>>16) & 255) / 255.f;

  const char* p;
  for (p = text; *p; ++p) 
    {
    if (*p == 10)
      {
      y -= c['@'].bh * sy;
      x = x_orig;
      continue;
      }

    float x2 = x + c[*p].bl * sx;
    float y2 = -y - c[*p].bt * sy;
    float w = c[*p].bw * sx;
    float h = c[*p].bh * sy;

    // Advance cursor to start of next char
    x += c[*p].ax * sx;
    y += c[*p].ay * sy;

    // Skip 0 pixel glyphs
    if (!w || !h)
      continue;

    verts[n++] = (text_vert_t)make_text_vert(x2, -y2, c[*p].tx, c[*p].ty, red, green, blue);
    verts[n++] = (text_vert_t)make_text_vert(x2 + w, -y2, c[*p].tx + c[*p].bw / atlas_width, c[*p].ty, red, green, blue);
    verts[n++] = (text_vert_t)make_text_vert(x2, -y2 - h, c[*p].tx, c[*p].ty + c[*p].bh / atlas_height, red, green, blue);
    verts[n++] = (text_vert_t)make_text_vert(x2 + w, -y2, c[*p].tx + c[*p].bw / atlas_width, c[*p].ty, red, green, blue);
    verts[n++] = (text_vert_t)make_text_vert(x2, -y2 - h, c[*p].tx, c[*p].ty + c[*p].bh / atlas_height, red, green, blue);
    verts[n++] = (text_vert_t)make_text_vert(x2 + w, -y2 - h, c[*p].tx + c[*p].bw / atlas_width, c[*p].ty + c[*p].bh / atlas_height, red, green, blue);
    }

  geometry_id = engine->add_geometry(VERTEX_2_2_3);

  text_vert_t* vp;
  uint32_t* ip;

  engine->geometry_begin(geometry_id, verts.size(), verts.size()*6, (float**)&vp, (void**)&ip);
  memcpy(vp, verts.data(), sizeof(float)*7*verts.size());
  for (uint32_t i = 0; i < verts.size(); ++i)
    {
    *ip++ = i * 6;
    *ip++ = i * 6 + 1;
    *ip++ = i * 6 + 2;
    *ip++ = i * 6 + 3;
    *ip++ = i * 6 + 4;
    *ip++ = i * 6 + 5;    
    }
  engine->geometry_end(geometry_id);
  engine->geometry_draw(geometry_id);
  }

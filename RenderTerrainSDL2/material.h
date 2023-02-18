#pragma once

#include "RenderDoos/material.h"

class terrain_material : public RenderDoos::material
  {
  public:
    terrain_material();
    virtual ~terrain_material();

    virtual void compile(RenderDoos::render_engine* engine);
    virtual void bind(RenderDoos::render_engine* engine);
    virtual void destroy(RenderDoos::render_engine* engine);

    void set_texture_heightmap(int32_t id);
    void set_texture_normalmap(int32_t id);
    void set_texture_colormap(int32_t id);

  private:
    int32_t vs_handle, fs_handle;
    int32_t shader_program_handle;
    int32_t proj_handle, res_handle, cam_handle;
    int32_t texture_heightmap, texture_normalmap, texture_colormap;
    int32_t heightmap_handle, normalmap_handle, colormap_handle;
  };
#pragma once

#include "RenderDoos/material.h"

class cube_material : public RenderDoos::material
  {
  public:
    cube_material();
    virtual ~cube_material();

    void set_cubemap(int32_t handle, int32_t flags);

    virtual void compile(RenderDoos::render_engine* engine);
    virtual void bind(RenderDoos::render_engine* engine);
    virtual void destroy(RenderDoos::render_engine* engine);

  private:
    int32_t vs_handle, fs_handle;
    int32_t shader_program_handle;
    int32_t tex_handle;    
    int32_t texture_flags;
    int32_t projection_handle, cam_handle, tex0_handle; // uniforms
  };
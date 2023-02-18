#ifndef _SDL_main_h
#define _SDL_main_h
#endif

#include "SDL.h"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(RENDERDOOS_METAL)
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "SDL_metal.h"
#include "metal/Metal.hpp"
#include "../SDL-metal/SDL_metal.h"
#include "SDL_metal.h"
#else
#include <GL/glew.h>
#include <glew/GL/glew.h>
#endif

#include "RenderDoos/render_engine.h"
#include "RenderDoos/material.h"

#include "RenderDoos/types.h"

#include <iostream>

extern "C"
  {
#include "trackball.h"
  }

struct mouse_data
  {
  float mouse_x;
  float mouse_y;
  float prev_mouse_x;
  float prev_mouse_y;
  bool dragging;
  };

namespace
  {
  static uint32_t random_seed = 0x74382381;

  uint32_t get_random()
    {
    uint32_t eax = random_seed;
    eax = eax * 0x343fd + 0x269ec3;
    uint32_t ebx = eax;
    eax = eax * 0x343fd + 0x269ec3;
    random_seed = eax;
    eax = (eax >> 10) & 0x0000ffff;
    ebx = (ebx << 6) & 0xffff0000;
    return eax | ebx;
    }

  uint32_t get_random(uint32_t maximum)
    {
    return get_random() % maximum;
    }

  }


int _main(int argc, char** argv)
  {
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE);

  uint32_t w = 800;
  uint32_t h = 450;
  RenderDoos::render_engine engine;
  if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
    {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Initilizated SDL Failed: %s", SDL_GetError());
    return -1;
    }
#if defined(RENDERDOOS_METAL)
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");

  SDL_Window* window = SDL_CreateWindow("RenderCubeSDL2",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

  SDL_MetalView metalView = SDL_Metal_CreateView(window);
  void* layer = SDL_Metal_GetLayer(metalView);
  MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
  assign_device(layer, metalDevice);
  if (!window)
    throw std::runtime_error("SDL can't create a window");


  engine.init(metalDevice, nullptr, RenderDoos::renderer_type::METAL);
#else
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_Window* window = SDL_CreateWindow("RenderCubeSDL2",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    w, h,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

  if (!window)
    throw std::runtime_error("SDL can't create a window");

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_SetSwapInterval(1); // Enable vsync


  glewExperimental = true;
  GLenum err = glewInit();
  if (GLEW_OK != err)
    throw std::runtime_error("GLEW initialization failed");
  glGetError(); // hack https://stackoverflow.com/questions/36326333/openglglfw-glgenvertexarrays-returns-gl-invalid-operation


  SDL_GL_MakeCurrent(window, gl_context);

  engine.init(nullptr, nullptr, RenderDoos::renderer_type::OPENGL);
#endif 

  mouse_data md;
  md.mouse_x = 0.f;
  md.mouse_y = 0.f;
  md.prev_mouse_x = 0.f;
  md.prev_mouse_y = 0.f;
  md.dragging = false;
  float zoom = 1.f;

  RenderDoos::model_view_properties mv_props;
  mv_props.init(w, h);
  mv_props.camera_space.col[3][2] = 5.f;
  mv_props.zoom_x = 1.f;
  mv_props.zoom_y = 1.f * h / w;
  mv_props.light_dir = RenderDoos::normalize(RenderDoos::float4(0.2f, 0.3f, 0.4f, 0.f));

  uint16_t* tex = new uint16_t[16 * 16 * 4];
  for (int ii = 0; ii < 256; ++ii)
    {
    tex[ii * 4 + 0] = (uint16_t)get_random(0x7fff);
    tex[ii * 4 + 1] = (uint16_t)get_random(0x7fff);
    tex[ii * 4 + 2] = (uint16_t)get_random(0x7fff);
    tex[ii * 4 + 3] = 0x7fff;
    }
  uint32_t texture_id = engine.add_texture(16, 16, RenderDoos::texture_format_rgba8, tex);
  delete[] tex;
  RenderDoos::simple_material simple_mat;
  simple_mat.set_texture(texture_id, TEX_WRAP_REPEAT | TEX_FILTER_LINEAR);
  simple_mat.compile(&engine);
  uint32_t geometry_id = engine.add_geometry(VERTEX_STANDARD);
  uint32_t depth_id = engine.add_texture(mv_props.viewport_width, mv_props.viewport_height, RenderDoos::texture_format_depth, (const uint16_t*)nullptr);

  RenderDoos::vertex_standard* vp;
  uint32_t* ip;

  engine.geometry_begin(geometry_id, 24, 6 * 6, (float**)&vp, (void**)&ip);
  // drawing
  static float planes[10][3] =
    {
      {  1,0,0 },
      { -1,0,0 },
      { 0, 1,0 },
      { 0,-1,0 },
      { 0,0, 1 },
      { 0,0,-1 },
      {  1,0,0 },
      { -1,0,0 },
      { 0, 1,0 },
      { 0,-1,0 },
    };
  for (int ii = 0; ii < 6; ii++)
    {
    for (int j = 0; j < 4; j++)
      {
      int s0 = ((j + 0) & 2) ? -1 : 1;
      int s1 = ((j + 1) & 2) ? -1 : 1;
      if (ii & 1)
        s0 *= -1;
      vp->x = planes[ii][0] + s0 * planes[ii + 2][0] + s1 * planes[ii + 4][0];
      vp->y = planes[ii][1] + s0 * planes[ii + 2][1] + s1 * planes[ii + 4][1];
      vp->z = planes[ii][2] + s0 * planes[ii + 2][2] + s1 * planes[ii + 4][2];
#ifdef USE_VERTEX_COMPACT
      vp->c0 = 0xff000000 | (gen() & 0x00ffffff);
#else
      vp->nx = planes[ii][0];
      vp->ny = planes[ii][1];
      vp->nz = planes[ii][2];
      vp->u = ((j + 0) & 2) ? 1.f : 0.f;
      vp->v = ((j + 1) & 2) ? 1.f : 0.f;
#endif
      vp++;
      }
    ip[0] = ii * 4 + 3;
    ip[1] = ii * 4 + 2;
    ip[2] = ii * 4 + 1;
    ip[3] = ii * 4 + 3;
    ip[4] = ii * 4 + 1;
    ip[5] = ii * 4 + 0;
    ip += 6;
    }
  engine.geometry_end(geometry_id);



  bool quit = false;

  while (!quit)
    {
    SDL_Event event;
    while (SDL_PollEvent(&event))
      {
      switch (event.type)
        {
        case SDL_QUIT:
        {
        quit = true;
        break;
        }
        case SDL_KEYDOWN:
        {
        switch (event.key.keysym.sym)
          {
          case SDLK_ESCAPE:
          {
          quit = true;
          break;
          }
          }
        }
        case SDL_MOUSEMOTION:
        {
        md.prev_mouse_x = md.mouse_x;
        md.prev_mouse_y = md.mouse_y;
        md.mouse_x = float(event.motion.x);
        md.mouse_y = float(event.motion.y);
        break;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
        if (event.button.button == 1)
          md.dragging = true;
        break;
        }
        case SDL_MOUSEBUTTONUP:
        {
        if (event.button.button == 1)
          md.dragging = false;
        break;
        }
        case SDL_MOUSEWHEEL:
        {
        if (event.wheel.y > 0)
          zoom *= 1.1f;
        else if (event.wheel.y < 0)
          zoom *= 1.0f / 1.1f;
        mv_props.zoom_x = zoom;
        mv_props.zoom_y = zoom * h / w;

        engine.set_model_view_properties(mv_props);
        break;
        }
        }
      } // while (SDL_PollEvent(&event))

    if (md.mouse_x != md.prev_mouse_x || md.mouse_y != md.prev_mouse_y)
      {
      if (md.dragging)
        {
        RenderDoos::float4 spin_quat;
        float wf = float(w);
        float hf = float(h);
        float x = float(md.mouse_x);
        float y = float(md.mouse_y);
        trackball(&spin_quat[0],
          -(wf - 2.0f * x) / wf,
          -(2.0f * y - hf) / hf,
          -(wf - 2.0f * md.prev_mouse_x) / wf,
          -(2.0f * md.prev_mouse_y - hf) / hf);
        auto rot = RenderDoos::quaternion_to_rotation(spin_quat);

        RenderDoos::float4 c;
        c[0] = 0.f;
        c[1] = 0.f;
        c[2] = 0.f;
        c[3] = 1.f;

        RenderDoos::float4x4 camera_position_inv = RenderDoos::invert_orthonormal(mv_props.camera_space);
        auto center_cam = matrix_vector_multiply(camera_position_inv, c);

        auto t1 = RenderDoos::make_translation(center_cam[0], center_cam[1], center_cam[2]);
        auto t2 = RenderDoos::make_translation(-center_cam[0], -center_cam[1], -center_cam[2]);

        auto tmp1 = RenderDoos::matrix_matrix_multiply(t2, camera_position_inv);
        auto tmp2 = RenderDoos::matrix_matrix_multiply(t1, rot);
        camera_position_inv = RenderDoos::matrix_matrix_multiply(tmp2, tmp1);
        mv_props.camera_space = RenderDoos::invert_orthonormal(camera_position_inv);


        md.prev_mouse_x = md.mouse_x;
        md.prev_mouse_y = md.mouse_y;
        }
      }

    RenderDoos::render_drawables drawables;
#if defined(RENDERDOOS_METAL)
    void* layer = SDL_Metal_GetLayer(metalView);
    CA::MetalDrawable* drawable = next_drawable(layer);
    drawables.metal_drawable = (void*)drawable;
    drawables.metal_screen_texture = (void*)drawable->texture();
#endif
    engine.frame_begin(drawables);

    RenderDoos::renderpass_descriptor descr;
    descr.clear_color = 0xff203040;
    descr.clear_flags = CLEAR_COLOR | CLEAR_DEPTH;
    descr.w = mv_props.viewport_width;
    descr.h = mv_props.viewport_height;
    descr.depth_texture_handle = depth_id;
    engine.renderpass_begin(descr);

    engine.set_model_view_properties(mv_props);
    simple_mat.bind(&engine);
    engine.geometry_draw(geometry_id);

    engine.renderpass_end();
    engine.frame_end();
    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(10.0));

#if defined(RENDERDOOS_OPENGL)
    SDL_GL_SwapWindow(window);
#endif

    } //while (!quit)

  SDL_Quit();
  return 0;
  }

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/)
  {
  return _main(0, nullptr);
  }
#else
int main(int argc, char** argv)
  {
  return _main(argc, argv);
  }
#endif

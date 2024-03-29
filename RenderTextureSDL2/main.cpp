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

  SDL_Window* window = SDL_CreateWindow("RenderTextureSDL2",
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
  SDL_Window* window = SDL_CreateWindow("RenderTextureSDL2",
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
  RenderDoos::model_view_properties mv_props;
  mv_props.init(w, h);
  mv_props.orthogonal = 1;
  mv_props.near_clip = 1.f;
  mv_props.zoom_x = 1.f;
  mv_props.zoom_y = 1.f;
  mv_props.light_dir = RenderDoos::normalize(RenderDoos::float4(0, 0, 1, 0));
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
  simple_mat.set_texture(texture_id, TEX_WRAP_REPEAT | TEX_FILTER_NEAREST);
  simple_mat.compile(&engine);
  uint32_t geometry_id = engine.add_geometry(VERTEX_STANDARD);
  RenderDoos::vertex_standard* vp;
  uint32_t* ip;

  engine.geometry_begin(geometry_id, 4, 6, (float**)&vp, (void**)&ip);
  // make a quad for drawing the texture

  vp->x = -1.f;
  vp->y = -1.f;
  vp->z = 0.f;
  vp->nx = 0.f;
  vp->ny = 0.f;
  vp->nz = 1.f;
  vp->u = 0.f;
  vp->v = 0.f;
  ++vp;
  vp->x = 1.f;
  vp->y = -1.f;
  vp->z = 0.f;
  vp->nx = 0.f;
  vp->ny = 0.f;
  vp->nz = 1.f;
  vp->u = 1.f;
  vp->v = 0.f;
  ++vp;
  vp->x = 1.f;
  vp->y = 1.f;
  vp->z = 0.f;
  vp->nx = 0.f;
  vp->ny = 0.f;
  vp->nz = 1.f;
  vp->u = 1.f;
  vp->v = 1.f;
  ++vp;
  vp->x = -1.f;
  vp->y = 1.f;
  vp->z = 0.f;
  vp->nx = 0.f;
  vp->ny = 0.f;
  vp->nz = 1.f;
  vp->u = 0.f;
  vp->v = 1.f;

  ip[0] = 0;
  ip[1] = 1;
  ip[2] = 2;
  ip[3] = 0;
  ip[4] = 2;
  ip[5] = 3;

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
        }
      } // while (SDL_PollEvent(&event))

    RenderDoos::render_drawables drawables;
#if defined(RENDERDOOS_METAL)
    void* layer = SDL_Metal_GetLayer(metalView);
    auto drawable = next_drawable(layer);
    drawables.metal_drawable = (void*)drawable.drawable;
    drawables.metal_screen_texture = (void*)drawable.texture;
#endif
    engine.frame_begin(drawables);

    RenderDoos::renderpass_descriptor descr;
    descr.clear_color = 0xff203040;
    descr.clear_flags = CLEAR_COLOR | CLEAR_DEPTH;
    descr.w = mv_props.viewport_width;
    descr.h = mv_props.viewport_height;

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

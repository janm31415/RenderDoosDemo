#ifndef _SDL_main_h
#define _SDL_main_h
#endif

#include "SDL.h"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

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
#include <vector>
#include <array>

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

  std::vector<std::array<float, 3>> generate_spherical_pointcloud()
    {
    std::vector<std::array<float, 3>> pc;

    uint32_t max_x = 20;
    uint32_t max_y = 20;

    pc.reserve(max_x * max_y);

    for (uint32_t x = 0; x < max_x; ++x)
      {
      for (uint32_t y = 0; y < max_y; ++y)
        {
        double theta = x / (double)(max_x - 1) * 2.0 * 3.1415926535;
        double phi = y / (double)(max_y - 1) * 3.1415926535;
        std::array<float, 3> pt({ (float)(std::cos(theta) * std::sin(phi)), (float)(std::sin(theta) * std::sin(phi)), (float)(std::cos(phi)) });
        pc.push_back(pt);
        }
      }


    return pc;
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

  SDL_Window* window = SDL_CreateWindow("RenderPointcloudSDL2",
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
  SDL_Window* window = SDL_CreateWindow("RenderPointcloudSDL2",
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

  std::vector<std::array<float, 3>> pointcloud = generate_spherical_pointcloud();
  std::vector<uint32_t> vertex_colors;
  vertex_colors.reserve(pointcloud.size());
  for (uint32_t j = 0; j < (uint32_t)pointcloud.size(); ++j)
    {
    vertex_colors.push_back(0xff000000 | get_random(0x00ffffff));
    }

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

  RenderDoos::vertex_colored_material vertex_colored_mat;
  vertex_colored_mat.compile(&engine);
  uint32_t geometry_id = engine.add_geometry(VERTEX_COLOR);
  uint32_t depth_id = engine.add_texture(mv_props.viewport_width, mv_props.viewport_height, RenderDoos::texture_format_depth, (const uint16_t*)nullptr);

  RenderDoos::vertex_color* vp;
  uint32_t* ip;

  engine.geometry_begin(geometry_id, (int32_t)pointcloud.size() * 4, (int32_t)pointcloud.size() * 6, (float**)&vp, (void**)&ip);
  // drawing

  for (uint32_t j = 0; j < pointcloud.size(); ++j)
    {
    const auto& pt = pointcloud[j];
    std::array<float, 3> axis({ 0,0,0 });
    int smallest_component = 0;
    if (std::abs(pt[1]) < std::abs(pt[smallest_component]))
      smallest_component = 1;
    if (std::abs(pt[2]) < std::abs(pt[smallest_component]))
      smallest_component = 2;
    axis[smallest_component] = 1;
    std::array<float, 3> cross, cross2;
    cross[0] = pt[1] * axis[2] - pt[2] * axis[1];
    cross[1] = pt[2] * axis[0] - pt[0] * axis[2];
    cross[2] = pt[0] * axis[1] - pt[1] * axis[0];
    cross2[0] = pt[1] * cross[2] - pt[2] * cross[1];
    cross2[1] = pt[2] * cross[0] - pt[0] * cross[2];
    cross2[2] = pt[0] * cross[1] - pt[1] * cross[0];
    float eps = 0.05;

    vp->nx = pt[0];
    vp->ny = pt[1];
    vp->nz = pt[2];
    vp->x = pt[0] + eps * cross[0];
    vp->y = pt[1] + eps * cross[1];
    vp->z = pt[2] + eps * cross[2];
    vp->c0 = vertex_colors[j];
    ++vp;
    vp->nx = pt[0];
    vp->ny = pt[1];
    vp->nz = pt[2];
    vp->x = pt[0] - eps * cross[0];
    vp->y = pt[1] - eps * cross[1];
    vp->z = pt[2] - eps * cross[2];
    vp->c0 = vertex_colors[j];
    ++vp;
    vp->nx = pt[0];
    vp->ny = pt[1];
    vp->nz = pt[2];
    vp->x = pt[0] + eps * cross2[0];
    vp->y = pt[1] + eps * cross2[1];
    vp->z = pt[2] + eps * cross2[2];
    vp->c0 = vertex_colors[j];
    ++vp;
    vp->nx = pt[0];
    vp->ny = pt[1];
    vp->nz = pt[2];
    vp->x = pt[0] - eps * cross2[0];
    vp->y = pt[1] - eps * cross2[1];
    vp->z = pt[2] - eps * cross2[2];
    vp->c0 = vertex_colors[j];
    ++vp;
    ip[0] = j * 4 + 0;
    ip[1] = j * 4 + 2;
    ip[2] = j * 4 + 3;
    ip[3] = j * 4 + 1;
    ip[4] = j * 4 + 3;
    ip[5] = j * 4 + 2;
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

    if (md.mouse_x != md.prev_mouse_x || md.mouse_y == md.prev_mouse_y)
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
    descr.clear_color = 0xff403020;
    descr.clear_flags = CLEAR_COLOR | CLEAR_DEPTH;
    descr.w = mv_props.viewport_width;
    descr.h = mv_props.viewport_height;
    descr.depth_texture_handle = depth_id;
    engine.renderpass_begin(descr);

    engine.set_model_view_properties(mv_props);
    vertex_colored_mat.bind(&engine);
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

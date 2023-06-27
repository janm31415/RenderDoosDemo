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
#include "material.h"
#include "image.h"
#include "keyboard.h"

#include "RenderDoos/types.h"

#include <iostream>

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

  SDL_Window* window = SDL_CreateWindow("RenderTerrainSDL2",
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
  SDL_Window* window = SDL_CreateWindow("RenderTerrainSDL2",
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

  rgba_image heightmap, normalmap, colormap;
  if (!read_image_from_file(heightmap, "assets/heightmap.png")
    || !read_image_from_file(normalmap, "assets/normalmap.png")
    || !read_image_from_file(colormap, "assets/colormap.png")
    )
    {
    std::cout << "Could not read asset\n";
    exit(1);
    }

  uint32_t heightmap_id = engine.add_texture(heightmap.w, heightmap.h, RenderDoos::texture_format_rgba8, (const uint8_t*)heightmap.im);
  uint32_t normalmap_id = engine.add_texture(normalmap.w, normalmap.h, RenderDoos::texture_format_rgba8, (const uint8_t*)normalmap.im);
  uint32_t colormap_id = engine.add_texture(colormap.w, colormap.h, RenderDoos::texture_format_rgba8, (const uint8_t*)colormap.im);

  terrain_material terrain_mat;
  terrain_mat.set_texture_heightmap(heightmap_id);
  terrain_mat.set_texture_normalmap(normalmap_id);
  terrain_mat.set_texture_colormap(colormap_id);
  terrain_mat.compile(&engine);
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

  float euler_x = 0.f;
  float euler_y = 0.f;
  float euler_z = 0.f;

  engine.geometry_end(geometry_id);
  bool quit = false;
  keyboard_handler keyb;

  while (!quit)
    {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    int mouse_rel_x = 0;
    int mouse_rel_y = 0;
    SDL_GetRelativeMouseState(&mouse_rel_x, &mouse_rel_y);

    bool move_forward = false;
    bool move_backward = false;
    bool strafe_left = false;
    bool strafe_right = false;
    bool move_up = false;
    bool move_down = false;
    bool turn_left = false;
    bool turn_right = false;
    bool turn_up = false;
    bool turn_down = false;

    SDL_Event event;
    while (SDL_PollEvent(&event))
      {
      keyb.handle_event(event);
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
        case SDL_MOUSEWHEEL:
        {
        if (event.wheel.y > 0)
          {
          move_backward = true;
          }
        else if (event.wheel.y < 0)
          {
          move_forward = true;
          }
        break;
        }
        }
      } // while (SDL_PollEvent(&event))

    float angle_x = 0.f;
    float angle_y = 0.f;

    if (mouse_rel_x || mouse_rel_y)
      {
      float diff_x = (float)mouse_rel_x;
      float diff_y = (float)mouse_rel_y;

      if (diff_x < 0.f)
        {
        turn_left = true;
        angle_x = std::abs(diff_x / (float)w);
        }
      if (diff_x > 0.f)
        {
        turn_right = true;
        angle_x = std::abs(diff_x / (float)w);
        }
      if (diff_y < 0.f)
        {
        turn_up = true;
        angle_y = std::abs(diff_y / (float)h);
        }
      if (diff_y > 0.f)
        {
        turn_down = true;
        angle_y = std::abs(diff_y / (float)h);
        }
      }
    if (turn_left)
      {
      euler_y -= angle_x;
      }
    if (turn_right)
      {
      euler_y += angle_x;
      }
    if (turn_up)
      {
      euler_x -= angle_y;
      }
    if (turn_down)
      {
      euler_x += angle_y;
      }
    RenderDoos::float4x4 camera_position_inv = RenderDoos::invert_orthonormal(mv_props.camera_space);
    RenderDoos::float4 position(camera_position_inv[12], camera_position_inv[13], camera_position_inv[14], 1);
    auto rot_y = RenderDoos::make_rotation(position, RenderDoos::float4(0, 1, 0, 0), euler_y);
    auto rot_x = RenderDoos::make_rotation(position, RenderDoos::float4(1, 0, 0, 0), euler_x);

    auto flip_rot = RenderDoos::make_rotation(position, RenderDoos::float4(1, 0, 0, 0), 3.1415926535897f / 2.f);

    camera_position_inv = matrix_matrix_multiply(rot_y, rot_x);
    camera_position_inv[12] = position[0];
    camera_position_inv[13] = position[1];
    camera_position_inv[14] = position[2];

    mv_props.camera_space = RenderDoos::invert_orthonormal(camera_position_inv);
    
    bool shift = keyb.is_down(SDLK_LSHIFT) || keyb.is_down(SDLK_RSHIFT);
    
    if (keyb.is_down(SDLK_w))
      {
      if (shift)
        move_up = true;
      else
        move_forward = true;
      }
    if (keyb.is_down(SDLK_s))
      {
      if (shift)
        move_down = true;
      else
        move_backward = true;
      }
    if (keyb.is_down(SDLK_a))
      {
      strafe_left = true;
      }
    if (keyb.is_down(SDLK_d))
      {
      strafe_right = true;
      }

    const float step_size = 0.1;
    if (move_forward)
      {
      RenderDoos::float4x4 camera_position_inv = RenderDoos::invert_orthonormal(mv_props.camera_space);
      RenderDoos::float4 dir = RenderDoos::get_z_axis(camera_position_inv);
      RenderDoos::float4 tr = RenderDoos::get_translation(camera_position_inv);
      tr = tr + step_size * dir;
      RenderDoos::set_translation(camera_position_inv, tr);
      mv_props.camera_space = RenderDoos::invert_orthonormal(camera_position_inv);
      }
    if (move_backward)
      {
      RenderDoos::float4x4 camera_position_inv = RenderDoos::invert_orthonormal(mv_props.camera_space);
      RenderDoos::float4 dir = RenderDoos::get_z_axis(camera_position_inv);
      RenderDoos::float4 tr = RenderDoos::get_translation(camera_position_inv);
      tr = tr - step_size * dir;
      RenderDoos::set_translation(camera_position_inv, tr);
      mv_props.camera_space = RenderDoos::invert_orthonormal(camera_position_inv);
      }
    if (strafe_left)
      {
      RenderDoos::float4x4 camera_position_inv = RenderDoos::invert_orthonormal(mv_props.camera_space);
      RenderDoos::float4 dir = RenderDoos::get_x_axis(camera_position_inv);
      RenderDoos::float4 tr = RenderDoos::get_translation(camera_position_inv);
      tr = tr - step_size * dir;
      RenderDoos::set_translation(camera_position_inv, tr);
      mv_props.camera_space = RenderDoos::invert_orthonormal(camera_position_inv);
      }
    if (strafe_right)
      {
      RenderDoos::float4x4 camera_position_inv = RenderDoos::invert_orthonormal(mv_props.camera_space);
      RenderDoos::float4 dir = RenderDoos::get_x_axis(camera_position_inv);
      RenderDoos::float4 tr = RenderDoos::get_translation(camera_position_inv);
      tr = tr + step_size * dir;
      RenderDoos::set_translation(camera_position_inv, tr);
      mv_props.camera_space = RenderDoos::invert_orthonormal(camera_position_inv);
      }
    if (move_up)
      {
      mv_props.camera_space[13] -= step_size;
      }
    if (move_down)
      {
      mv_props.camera_space[13] += step_size;
      }

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
    terrain_mat.bind(&engine);
    engine.geometry_draw(geometry_id);

    engine.renderpass_end();
    engine.frame_end();
    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(10.0));

#if defined(RENDERDOOS_OPENGL)
    SDL_GL_SwapWindow(window);
#endif

      } //while (!quit)

  engine.remove_texture(heightmap_id);
  engine.remove_texture(normalmap_id);
  engine.remove_texture(colormap_id);

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

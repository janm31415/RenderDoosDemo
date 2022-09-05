#include <stdio.h>
#include <string.h>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>

#if defined(RENDERDOOS_METAL)
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "metal/Metal.hpp"
#endif

//#undef RENDERDOOS_METAL // if you want to use opengl on macos instead of metal, uncomment this line

#if defined(RENDERDOOS_METAL)
#include "../fltk-metal/Fl_Metal_Window.h"
#else
#include <GL/glew.h>
#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>
#include <glew/GL/glew.h>
#endif

#include "RenderDoos/render_engine.h"
#include "RenderDoos/material.h"

#include "RenderDoos/types.h"

#include <iostream>
#include <chrono>

#if defined(RENDERDOOS_METAL)
class canvas : public Fl_Metal_Window
{
public:
  canvas(int x, int y, int w, int h, const char* t) : Fl_Metal_Window(x, y, w, h, t), _material(nullptr)
  {
#else
    class canvas : public Fl_Gl_Window
    {
    public:
      canvas(int x, int y, int w, int h, const char* t) : Fl_Gl_Window(x, y, w, h, t), _material(nullptr)
      {
        mode(FL_RGB8 | FL_DOUBLE | FL_OPENGL3 | FL_DEPTH);
#endif
        resizable(this);
        
        _mv_props.init(w, h);
        _mv_props.orthogonal = 1;
        _mv_props.near_clip = 1.f;
        _mv_props.zoom_x = 1.f;
        _mv_props.zoom_y = 1.f;
        _mv_props.light_dir = RenderDoos::normalize(RenderDoos::float4(0, 0, 1, 0));
        _st_props.frame = 0;
        _st_props.time = 0;
        _st_props.time_delta = 0;
        
        _start = std::chrono::high_resolution_clock::now();
        _last_tic = _start;
      }
      
      virtual ~canvas()
      {
      }
      
      virtual void resize(int X, int Y, int W, int H)
      {
        _mv_props.init(W, H);
        _mv_props.orthogonal = 1;
        _mv_props.near_clip = 1.f;
        _mv_props.zoom_x = 1.f;
        _mv_props.zoom_y = 1.f;
        _mv_props.light_dir = RenderDoos::float4(0, 0, 1, 0);
        
#if defined(RENDERDOOS_METAL)
        Fl_Metal_Window::resize(X, Y, W, H);
#else
        Fl_Gl_Window::resize(X, Y, W, H);
#endif
      }

      virtual void redraw()
      {
      flush();
      }
      
      virtual void hide()
      {
        _engine.destroy();
        delete _material;
        _material = nullptr;
#if defined(RENDERDOOS_METAL)
        Fl_Metal_Window::hide();
#else
        Fl_Gl_Window::hide();
#endif
      }
      
    private:
      
#if defined(RENDERDOOS_METAL)
      void _init_engine(MTL::Device* device, CA::MetalDrawable* drawable)
      {
        _engine.init(device, RenderDoos::renderer_type::METAL);
#else
        void _init_engine()
        {
          glewExperimental = true;
          GLenum err = glewInit();
          if (GLEW_OK != err)
            throw std::runtime_error("GLEW initialization failed");
          glGetError(); // hack
          _engine.init(nullptr, RenderDoos::renderer_type::OPENGL);
#endif
          _material = new RenderDoos::shadertoy_material();
          
#if defined(RENDERDOOS_METAL)
          std::string script = std::string(R"(void mainImage(thread float4& fragColor, float2 fragCoord, float iTime, float3 iResolution)
{
  float2 uv = fragCoord / iResolution.xy;
  float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0, 2, 4));
  
  fragColor = float4(col[0], col[1], col[2], 1);
})");
#else
          std::string script = std::string(R"(void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));

    // Output to screen
    fragColor = vec4(col,1.0);
})");
#endif
          _material->set_script(script);
          _material->compile(&_engine);
          _geometry_id = _engine.add_geometry(VERTEX_STANDARD);
          
          RenderDoos::vertex_standard* vp;
          uint32_t* ip;
          
          _engine.geometry_begin(_geometry_id, 4, 6, (float**)&vp, (void**)&ip);
          // drawing
          
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
          
          _engine.geometry_end(_geometry_id);
        }
        
#if defined(RENDERDOOS_METAL)
        virtual void _draw(MTL::Device* device, CA::MetalDrawable* drawable)
        {
          if (!_engine.is_initialized())
            _init_engine(device, drawable);
#else
          virtual void draw()
          {
            if (!_engine.is_initialized())
              _init_engine();
#endif
            RenderDoos::render_drawables drawables;
#if defined(RENDERDOOS_METAL)
            drawables.metal_drawable = (void*)drawable;
            drawables.metal_screen_texture = (void*)drawable->texture();
#endif
            _engine.frame_begin(drawables);
            auto tic = std::chrono::high_resolution_clock::now();
            _st_props.time_delta = (float)(std::chrono::duration_cast<std::chrono::microseconds>(tic - _last_tic).count()) / 1000000.f;
            _last_tic = tic;
            _st_props.time = (float)(std::chrono::duration_cast<std::chrono::microseconds>(tic - _start).count()) / 1000000.f;
            _material->set_shadertoy_properties(_st_props);
            
            
            RenderDoos::renderpass_descriptor descr;
            descr.clear_color = 0xff203040;
            descr.clear_flags = CLEAR_COLOR | CLEAR_DEPTH;
            descr.w = _mv_props.viewport_width;
            descr.h = _mv_props.viewport_height;
            
            _engine.renderpass_begin(descr);
            
            _engine.set_model_view_properties(_mv_props);
            _material->bind(&_engine);
            
            _engine.geometry_draw(_geometry_id);
            
            _engine.renderpass_end();
            _engine.frame_end();
            ++_st_props.frame;
          }
          
        private:
          RenderDoos::render_engine _engine;
          int32_t _geometry_id;
          RenderDoos::shadertoy_material* _material;
          RenderDoos::model_view_properties _mv_props;
          std::chrono::steady_clock::time_point _start, _last_tic;
          RenderDoos::shadertoy_material::properties _st_props;
        };
        
        
        class render_window : public Fl_Double_Window
        {
        public:
          render_window(int x, int y, int w, int h, const char* t) : Fl_Double_Window(x, y, w, h, t)
          {
            box(FL_NO_BOX);
            _background = new Fl_Box(0, 0, this->w(), this->h());
            _background->box(FL_DOWN_BOX);
            _background->color(19);
            _background->labelsize(18);
            _background->align(FL_ALIGN_CLIP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
            resizable(*_background);
            _canvas = new canvas(0, 0, this->w(), this->h(), "");
          }
          
          virtual ~render_window()
          {
            delete _background;
            delete _canvas;
          }
          
          virtual void redraw()
          {
            _canvas->redraw();
            Fl_Double_Window::redraw();
          }
          
        private:
          Fl_Box* _background;
          canvas* _canvas;
        };
        
        render_window* new_view(int w = 800, int h = 450)
        {
          int x = 100;
          int y = 100;
#ifdef _WIN32
          RECT desktop;
          // Get a handle to the desktop window
          const HWND hDesktop = GetDesktopWindow();
          // Get the size of screen to the variable desktop
          GetWindowRect(hDesktop, &desktop);
          x = ((desktop.right - desktop.left) - w) / 2;
          y = ((desktop.bottom - desktop.top) - h) / 2;
#endif
          render_window* window = new render_window(x, y, w, h, "RenderShadertoy");
          
          return window;
        }
        
        int _main(int argc, char** argv)
        {
          int result = -1;
          render_window* window = nullptr;
          try
          {
            window = new_view();
            Fl_Color c = (Fl_Color)FL_MAGENTA;
            uchar buffer[32 * 32 * 3];
            Fl_RGB_Image icon(buffer, 32, 32, 3);
            icon.color_average(c, 0.0);
            window->icon(&icon);
            
            window->end();
            window->show(argc, argv);
            while (Fl::check())
            {
              window->redraw();
              Fl::wait(0.016);
            }
            result = 0;
          }
          catch (std::runtime_error& e)
          {
#ifdef _WIN32
            std::string message(e.what());
            std::wstring wmessage(message.begin(), message.end());
            MessageBox(NULL, wmessage.c_str(), L"Error!", MB_OK);
#else
            std::cout << e.what() << "\n";
#endif
          }
          catch (...) {}
          delete window;
          return result;
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

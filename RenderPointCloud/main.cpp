#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>

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
      _pointcloud = generate_spherical_pointcloud();
      _vertex_colors.reserve(_pointcloud.size());
      for (uint32_t j = 0; j < (uint32_t)_pointcloud.size(); ++j)
        {
        _vertex_colors.push_back(0xff000000 | get_random(0x00ffffff));
        }
      _mouse_data.mouse_x = 0.f;
      _mouse_data.mouse_y = 0.f;
      _mouse_data.prev_mouse_x = 0.f;
      _mouse_data.prev_mouse_y = 0.f;
      _mouse_data.dragging = false;
      _zoom = 1.f;

      _mv_props.init(w, h);
      _mv_props.camera_space.col[3][2] = 5.f;
      _mv_props.zoom_x = 1.f;
      _mv_props.zoom_y = 1.f * h / w;
      _mv_props.light_dir = RenderDoos::normalize(RenderDoos::float4(0.2f, 0.3f, 0.4f, 0.f));
      }

    virtual ~canvas()
      {

      }

    virtual void resize(int X, int Y, int W, int H)
      {
      _zoom = 1.f;
      _mv_props.init(W, H);
      _mv_props.camera_space.col[3][2] = 5.f;
      _mv_props.zoom_x = _zoom;
      _mv_props.zoom_y = _zoom * H / W;
      _mv_props.light_dir = RenderDoos::normalize(RenderDoos::float4(0.2f, 0.3f, 0.4f, 0.f));
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
      _material->destroy(&_engine);
      _engine.destroy();
      delete _material;
      _material = nullptr;
#if defined(RENDERDOOS_METAL)
      Fl_Metal_Window::hide();
#else
      Fl_Gl_Window::hide();
#endif
      }

    virtual int handle(int event)
      {
      switch (event)
        {
        case FL_DRAG:
        case FL_MOVE:
          _mouse_data.prev_mouse_x = _mouse_data.mouse_x;
          _mouse_data.prev_mouse_y = _mouse_data.mouse_y;
          _mouse_data.mouse_x = (float)Fl::event_x();
          _mouse_data.mouse_y = (float)Fl::event_y();
          break;
        case FL_PUSH:
          if (Fl::event_button() == 1)
            {
            _mouse_data.dragging = true;
            return 1;
            }
          break;
        case FL_RELEASE:
          if (Fl::event_button() == 1)
            {
            _mouse_data.dragging = false;
            return 1;
            }
        case FL_MOUSEWHEEL:
          if (Fl::event_dy() < 0)
            _zoom *= 1.1f;
          else if (Fl::event_dy() > 0)
            _zoom *= 1.0f / 1.1f;
          redraw();
          break;
        default:
          break;
        }
      _do_mouse();
#if defined(RENDERDOOS_METAL)
      return Fl_Metal_Window::handle(event);
#else
      return Fl_Gl_Window::handle(event);
#endif
      }

  protected:

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
      RenderDoos::vertex_color* vp;
      uint32_t* ip;

      _mv_props.zoom_x = _zoom;
      _mv_props.zoom_y = _zoom * h() / w();

      _engine.set_model_view_properties(_mv_props);

      RenderDoos::renderpass_descriptor descr;
      descr.clear_color = 0xff403020;
      descr.clear_flags = CLEAR_COLOR | CLEAR_DEPTH;
      descr.w = _mv_props.viewport_width;
      descr.h = _mv_props.viewport_height;
      descr.depth_texture_handle = _depth_id;
      _engine.renderpass_begin(descr);

      _material->bind(&_engine);

      _engine.geometry_begin(_geometry_id, (int32_t)_pointcloud.size() * 4, (int32_t)_pointcloud.size() * 6, (float**)&vp, (void**)&ip);
      // drawing

      for (uint32_t j = 0; j < _pointcloud.size(); ++j)
        {
        const auto& pt = _pointcloud[j];
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
        vp->c0 = _vertex_colors[j];
        ++vp;
        vp->nx = pt[0];
        vp->ny = pt[1];
        vp->nz = pt[2];
        vp->x = pt[0] - eps * cross[0];
        vp->y = pt[1] - eps * cross[1];
        vp->z = pt[2] - eps * cross[2];
        vp->c0 = _vertex_colors[j];
        ++vp;
        vp->nx = pt[0];
        vp->ny = pt[1];
        vp->nz = pt[2];
        vp->x = pt[0] + eps * cross2[0];
        vp->y = pt[1] + eps * cross2[1];
        vp->z = pt[2] + eps * cross2[2];
        vp->c0 = _vertex_colors[j];
        ++vp;
        vp->nx = pt[0];
        vp->ny = pt[1];
        vp->nz = pt[2];
        vp->x = pt[0] - eps * cross2[0];
        vp->y = pt[1] - eps * cross2[1];
        vp->z = pt[2] - eps * cross2[2];
        vp->c0 = _vertex_colors[j];
        ++vp;
        ip[0] = j * 4 + 0;
        ip[1] = j * 4 + 2;
        ip[2] = j * 4 + 3;
        ip[3] = j * 4 + 1;
        ip[4] = j * 4 + 3;
        ip[5] = j * 4 + 2;
        ip += 6;
        }      
      _engine.geometry_end(_geometry_id);
      _engine.geometry_draw(_geometry_id);
      _engine.renderpass_end();
      _engine.frame_end();
      }

  private:

    void _do_mouse()
      {
      if (_mouse_data.mouse_x == _mouse_data.prev_mouse_x &&
        _mouse_data.mouse_y == _mouse_data.prev_mouse_y)
        return;

      if (_mouse_data.dragging)
        {
        RenderDoos::float4 spin_quat;
        float wf = float(w());
        float hf = float(h());
        float x = float(_mouse_data.mouse_x);
        float y = float(_mouse_data.mouse_y);
        trackball(&spin_quat[0],
          -(wf - 2.0f * x) / wf,
          -(2.0f * y - hf) / hf,
          -(wf - 2.0f * _mouse_data.prev_mouse_x) / wf,
          -(2.0f * _mouse_data.prev_mouse_y - hf) / hf);
        auto rot = RenderDoos::quaternion_to_rotation(spin_quat);

        RenderDoos::float4 c;
        c[0] = 0.f;
        c[1] = 0.f;
        c[2] = 0.f;
        c[3] = 1.f;

        RenderDoos::float4x4 camera_position_inv = RenderDoos::invert_orthonormal(_mv_props.camera_space);
        auto center_cam = matrix_vector_multiply(camera_position_inv, c);

        auto t1 = RenderDoos::make_translation(center_cam[0], center_cam[1], center_cam[2]);
        auto t2 = RenderDoos::make_translation(-center_cam[0], -center_cam[1], -center_cam[2]);

        auto tmp1 = RenderDoos::matrix_matrix_multiply(t2, camera_position_inv);
        auto tmp2 = RenderDoos::matrix_matrix_multiply(t1, rot);
        camera_position_inv = RenderDoos::matrix_matrix_multiply(tmp2, tmp1);
        _mv_props.camera_space = RenderDoos::invert_orthonormal(camera_position_inv);


        _mouse_data.prev_mouse_x = _mouse_data.mouse_x;
        _mouse_data.prev_mouse_y = _mouse_data.mouse_y;
        redraw();
      }
  }

#if defined(RENDERDOOS_METAL)
    void _init_engine(MTL::Device * device, CA::MetalDrawable * drawable)
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

      RenderDoos::vertex_colored_material* mat = new RenderDoos::vertex_colored_material();     
      _material = mat;
      _material->compile(&_engine);
      _geometry_id = _engine.add_geometry(VERTEX_COLOR);
      _depth_id = _engine.add_texture(_mv_props.viewport_width, _mv_props.viewport_height, RenderDoos::texture_format_depth, (const uint16_t*)nullptr);
      }

  private:
    RenderDoos::render_engine _engine;
    RenderDoos::material* _material;
    int32_t _geometry_id;
    int32_t _depth_id;
    mouse_data _mouse_data;
    RenderDoos::model_view_properties _mv_props;
    float _zoom;
    std::vector<std::array<float, 3>> _pointcloud;
    std::vector<uint32_t> _vertex_colors;
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
  render_window* window = new render_window(x, y, w, h, "RenderPointCloud");

  return window;
  }

int _main(int argc, char** argv)
  {
  int result = -1;
  render_window* window = nullptr;
  try
    {
    window = new_view();
    Fl_Color c = (Fl_Color)FL_CYAN;
    uchar buffer[32 * 32 * 3];
    Fl_RGB_Image icon(buffer, 32, 32, 3);
    icon.color_average(c, 0.0);
    window->icon(&icon);

    window->end();
    window->show(argc, argv);
    result = Fl::run();
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

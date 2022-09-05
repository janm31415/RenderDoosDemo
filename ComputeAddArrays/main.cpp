#include <iostream>

#if defined(RENDERDOOS_METAL)
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "metal/Metal.hpp"
#endif

#if defined(RENDERDOOS_METAL)
#include "../fltk-metal/Fl_Metal_Window.h"
#else
#include <GL/glew.h>
#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>
#endif

#include "RenderDoos/render_engine.h"

#include <random>

#include <vector>


#define NO_METAL_WINDOW   // for metal you don't need a window to create a device. For opengl currently you need a window

namespace
  {

  class xorshift32
    {
    public:
      typedef uint32_t result_type;

      xorshift32() : _state(0x74382381) {}
      ~xorshift32() {}

      uint32_t operator()()
        {
        _state ^= (_state << 13);
        _state ^= (_state >> 17);
        _state ^= (_state << 5);
        return _state;
        }

      void seed(uint32_t s)
        {
        _state = s + s * 17 + s * 121 + (s * 121 / 17);
        this->operator()();
        _state ^= s + s * 17 + s * 121 + (s * 121 / 17);
        this->operator()();
        _state ^= s + s * 17 + s * 121 + (s * 121 / 17);
        this->operator()();
        _state ^= s + s * 17 + s * 121 + (s * 121 / 17);
        this->operator()();
        }

      static constexpr uint32_t(max)()
        {
        return 0xffffffff;
        }

      static constexpr uint32_t(min)()
        {
        return 0;
        }

    private:
      uint32_t _state;
    };

  }

#if defined(RENDERDOOS_METAL)
class canvas : public Fl_Metal_Window
{
  public:
    canvas(int x, int y, int w, int h, const char* t) : Fl_Metal_Window(x, y, w, h, t)
    {
    }
#else
class canvas : public Fl_Gl_Window
  {
  public:
    canvas(int x, int y, int w, int h, const char* t) : Fl_Gl_Window(x, y, w, h, t)
      {
      mode(FL_RGB8 | FL_DOUBLE | FL_OPENGL3 | FL_DEPTH);
      }
#endif

    virtual ~canvas()
      {
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
      perform_computation();
      }

    std::string get_compute_shader_gl()
      {
      return std::string(R"(
#version 430
layout (local_size_x = 64) in;

layout(std430, binding = 0) buffer _inA
{
    float inA[];
};
layout(std430, binding = 1) buffer _inB
{
    float inB[];
};
layout(std430, binding = 2) buffer _result
{
    float result[];
};

void main()
{
int index = int(gl_GlobalInvocationID.x);
result[index] = inA[index] + inB[index];
}
)");
      }

    void perform_computation()
      {
#if defined(RENDERDOOS_METAL)
      int32_t sh = _engine.add_shader(nullptr, SHADER_COMPUTE, "add_arrays");
#else
      int32_t sh = _engine.add_shader(get_compute_shader_gl().c_str(), SHADER_COMPUTE, nullptr);
#endif
      int32_t ph = _engine.add_program(-1, -1, sh);
      std::vector<float> A, B, result;
      const int max_size = 1024;
      xorshift32 gen;
      std::uniform_real_distribution<float> dist(0.0, 1.0);
      A.reserve(max_size);
      B.reserve(max_size);
      result.reserve(max_size);
      for (int ii = 0; ii < max_size; ++ii)
        {
        A.push_back(dist(gen));
        B.push_back(dist(gen));
        result.push_back(A.back()+B.back());
        }
      int32_t hA = _engine.add_buffer_object(A.data(), (int32_t)A.size() * sizeof(float));
      int32_t hB = _engine.add_buffer_object(B.data(), (int32_t)B.size() * sizeof(float));
      int32_t hres = _engine.add_buffer_object(nullptr, (int32_t)B.size() * sizeof(float));
      
      RenderDoos::render_drawables drawables;
      _engine.frame_begin(drawables);
      
      RenderDoos::renderpass_descriptor descr;
      descr.compute_shader = true;
      
      _engine.renderpass_begin(descr);

      _engine.bind_program(ph);

      _engine.bind_buffer_object(hA, 0);
      _engine.bind_buffer_object(hB, 1);
      _engine.bind_buffer_object(hres, 2);

      _engine.dispatch_compute((max_size + 63)/64, 1, 1, 64, 1, 1);
      
      _engine.renderpass_end();
      _engine.frame_end(true);

      std::vector<float> computed_result(max_size, 0.f);
      _engine.get_data_from_buffer_object(hres, computed_result.data(), max_size * sizeof(float));

      int nr_errors = 0;
      for (int ii = 0; ii < max_size; ++ii)
        {
        if (result[ii] != computed_result[ii])
          {
          std::cout << "Error: got " << computed_result[ii] << " but expected " << result[ii] << std::endl;
          ++nr_errors;
          }
        }
      std::cout << "ERRORS FOUND: " << nr_errors << std::endl;
      }

  private:
#if defined(RENDERDOOS_METAL)
    void _init_engine(MTL::Device* device, CA::MetalDrawable* drawable)
      {
      _engine.init(device, RenderDoos::renderer_type::METAL);
      }
#else
    void _init_engine()
      {
      glewExperimental = true;
      GLenum err = glewInit();
      if (GLEW_OK != err)
        throw std::runtime_error("GLEW initialization failed");
      glGetError(); // hack

      _engine.init(nullptr, RenderDoos::renderer_type::OPENGL);
      }
#endif
      

  private:
    RenderDoos::render_engine _engine;
  };
  
#if defined(RENDERDOOS_METAL)
void perform_metal_computation_without_window()
  {
  MTL::Device* p_device = MTL::CreateSystemDefaultDevice();
  RenderDoos::render_engine _engine;
  _engine.init(p_device, RenderDoos::renderer_type::METAL);
  int32_t sh = _engine.add_shader(nullptr, SHADER_COMPUTE, "add_arrays");
  int32_t ph = _engine.add_program(-1, -1, sh);
  std::vector<float> A, B, result;
  const int max_size = 1024;
  jtk::xorshift32 gen;
  std::uniform_real_distribution<float> dist(0.0, 1.0);
  A.reserve(max_size);
  B.reserve(max_size);
  result.reserve(max_size);
  for (int i = 0; i < max_size; ++i)
    {
    A.push_back(dist(gen));
    B.push_back(dist(gen));
    result.push_back(A.back()+B.back());
    }
  int32_t hA = _engine.add_buffer_object(A.data(), A.size() * sizeof(float));
  int32_t hB = _engine.add_buffer_object(B.data(), B.size() * sizeof(float));
  int32_t hres = _engine.add_buffer_object(nullptr, B.size() * sizeof(float));
  
  RenderDoos::render_drawables drawables;
  _engine.frame_begin(drawables);
  
  RenderDoos::renderpass_descriptor descr;
  descr.compute_shader = true;
  
  _engine.renderpass_begin(descr);

  _engine.bind_buffer_object(hA, 0);
  _engine.bind_buffer_object(hB, 1);
  _engine.bind_buffer_object(hres, 2);

  _engine.bind_program(ph);

  _engine.dispatch_compute((max_size + 63)/64, 1, 1, 64, 1, 1);
  
  _engine.renderpass_end();
  _engine.frame_end(true);

  std::vector<float> computed_result(max_size, 0.f);
  _engine.get_data_from_buffer_object(hres, computed_result.data(), max_size * sizeof(float));

  int nr_errors = 0;
  for (int i = 0; i < max_size; ++i)
    {
    if (result[i] != computed_result[i])
      {
      std::cout << "Error: got " << computed_result[i] << " but expected " << result[i] << std::endl;
      ++nr_errors;
      }
    }
  std::cout << "ERRORS FOUND: " << nr_errors << std::endl;
  _engine.destroy();
  p_device->release();
  }
#endif

int main(int argc, char** argv)
  {
#if defined(RENDERDOOS_METAL) && defined(NO_METAL_WINDOW)
  perform_metal_computation_without_window();
#else
  canvas* window = new canvas(100, 100, 100, 100, "canvas");
  window->end();
  window->show(argc, argv);
  Fl::wait(0.16);
#endif
  return 0;
  }

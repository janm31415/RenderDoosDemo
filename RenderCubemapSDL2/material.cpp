#include "material.h"
#include "RenderDoos/types.h"
#include "RenderDoos/render_context.h"
#include "RenderDoos/render_engine.h"

static std::string get_cubemap_material_vertex_shader()
  {
  return std::string(R"(#version 330 core
layout (location = 0) in vec3 vPosition;

uniform mat4 Projection; // columns
uniform mat4 Camera; // columns

out vec3 localPos;

void main()
{
    localPos = vPosition;
    mat4 rotView = mat4(mat3(Camera)); // remove translation from the view matrix
    vec4 clipPos = Projection * rotView * vec4(localPos, 1.0);

    gl_Position = clipPos;
    //gl_Position.y = -gl_Position.y;
}
)");
  }

static std::string get_cubemap_material_fragment_shader()
  {
  return std::string(R"(#version 330 core
out vec4 FragColor;

in vec3 localPos;
  
uniform samplerCube environmentMap;
//uniform sampler2D environmentMap;
  
void main()
{
    vec3 envColor = texture(environmentMap, localPos).rgb;
    
    //envColor = envColor / (envColor + vec3(1.0));
    //envColor = pow(envColor, vec3(1.0/2.2)); 
  
    FragColor = vec4(envColor, 1.0);
}
)");
  }

cube_material::cube_material()
  {
  vs_handle = -1;
  fs_handle = -1;
  shader_program_handle = -1;
  tex_handle = -1;
  projection_handle = -1;
  cam_handle = -1;
  tex0_handle = -1;
  }

cube_material::~cube_material()
  {
  }

void cube_material::set_cubemap(int32_t handle, int32_t flags)
  {
  if (handle >= 0 && handle < MAX_TEXTURE)
    tex_handle = handle;
  else
    tex_handle = -1;
  texture_flags = flags;
  }

void cube_material::destroy(RenderDoos::render_engine* engine)
  {
  engine->remove_program(shader_program_handle);
  engine->remove_shader(vs_handle);
  engine->remove_shader(fs_handle);
  engine->remove_texture(tex_handle);
  engine->remove_uniform(projection_handle);
  engine->remove_uniform(cam_handle);
  engine->remove_uniform(tex0_handle);
  }

void cube_material::compile(RenderDoos::render_engine* engine)
  {
  using namespace RenderDoos;
  if (engine->get_renderer_type() == renderer_type::METAL)
    {
    vs_handle = engine->add_shader(nullptr, SHADER_VERTEX, "cube_material_vertex_shader");
    fs_handle = engine->add_shader(nullptr, SHADER_FRAGMENT, "cube_material_fragment_shader");
    }
  else if (engine->get_renderer_type() == renderer_type::OPENGL)
    {
    vs_handle = engine->add_shader(get_cubemap_material_vertex_shader().c_str(), SHADER_VERTEX, nullptr);
    fs_handle = engine->add_shader(get_cubemap_material_fragment_shader().c_str(), SHADER_FRAGMENT, nullptr);
    }
  shader_program_handle = engine->add_program(vs_handle, fs_handle);
  projection_handle = engine->add_uniform("Projection", uniform_type::mat4, 1);
  cam_handle = engine->add_uniform("Camera", uniform_type::mat4, 1);
  tex0_handle = engine->add_uniform("environmentMap", uniform_type::sampler, 1);
  }

void cube_material::bind(RenderDoos::render_engine* engine)
  {
  engine->bind_program(shader_program_handle);

  engine->set_uniform(projection_handle, (void*)&engine->get_projection());
  engine->set_uniform(cam_handle, (void*)&engine->get_camera_space());
  int32_t tex_0 = 0;
  engine->set_uniform(tex0_handle, (void*)&tex_0);

  engine->bind_uniform(shader_program_handle, projection_handle);
  engine->bind_uniform(shader_program_handle, cam_handle);
  engine->bind_uniform(shader_program_handle, tex0_handle);
  //const RenderDoos::texture* tex = engine->get_texture(tex_handle);  
  engine->bind_texture_to_channel(tex_handle, 0, texture_flags);  
  }
#include "material.h"
#include "RenderDoos/types.h"
#include "RenderDoos/render_context.h"
#include "RenderDoos/render_engine.h"


static std::string get_terrain_material_vertex_shader()
  {
  return std::string(R"(#version 330 core
layout (location = 0) in vec3 vPosition;
uniform mat4 Projection; // columns

void main() 
  {   
  gl_Position = Projection*vec4(vPosition.xyz,1); 
  }
)");
  }

static std::string get_terrain_material_fragment_shader()
  {
  return std::string(R"(#version 330 core
uniform mat4 Camera;
uniform vec3 iResolution;
uniform sampler2D Heightmap;
uniform sampler2D Normalmap;
uniform sampler2D Colormap;

out vec4 FragColor;

#define M_PI 3.1415926535897932384626433832795

vec2 scalePosition(in vec2 p)
{
  p = p*0.05;
  p = p+vec2(0.5);
  return p;
}

float terrain( in vec2 p)
{
    /*
    const float heightScale = 1.0;
   	const bool flip = true;
    
    p = scalePosition(p);
    float scale = 1.0;
    //float scale = max(0.0, mix(1.0, 0.0, max(1.0, 2.0*max(abs(p.x), abs(p.y))) - 1.0));
    //scale = -2.0*scale*scale*scale + 3.0*scale*scale;
    if (p.x < 0.0 || p.x >= 1.0 || p.y < 0.0 || p.y >= 1.0)
        return 0.0;
   	p = mix(vec2(0.5, 0.0), vec2(1.0, 0.5), p);
    float val = texture( Heightmap, p, 0.0 ).x;
    if (flip)
        val = 1.0 - val;
   	//val = (val-0.5)*5.0;
	return val * scale * heightScale;
*/
   p = scalePosition(p);
   if (p.x < 0.0 || p.x >= 1.0 || p.y < 0.0 || p.y >= 1.0)
     return 0.0;
   float val = texture( Heightmap, p).x;
   return val;
}

float map( in vec3 p )
{
    return p.y - terrain(p.xz);
}

vec3 getColor( in vec3 pos )
{
  vec2 p = scalePosition(pos.xz);
  if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
    return vec3(0,0,0);
  return texture( Colormap, p).rgb;
}

vec3 calcNormal( in vec3 pos, float t )
{
#if 1
  vec2 p = scalePosition(pos.xz);
  if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
    return vec3(0,1,0);
  return texture( Normalmap, p).rgb;
#else
	float e = 0.001;
	e = 0.0001*t;
    vec3  eps = vec3(e,0.0,0.0);
    vec3 nor;
    nor.x = map(pos+eps.xyy) - map(pos-eps.xyy);
    nor.y = map(pos+eps.yxy) - map(pos-eps.yxy);
    nor.z = map(pos+eps.yyx) - map(pos-eps.yyx);
    return normalize(nor);
#endif
}

float intersect( in vec3 ro, in vec3 rd )
{
    const float maxd = 40.0;
    const float precis = 0.001;
    float t = 0.0;
    for( int i=0; i<256; i++ )
    {
        float h = map( ro+rd*t );
        if( abs(h)<precis || t>maxd ) break;
        t += h*0.5;
    }
    return (t>maxd)?-1.0:t;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
/*
	float time = iTime;
    float rotTimeScale = 0.3;
    
    //vec4 m = iMouse / iResolution.xxxx;
    
    float mousePos = -iMouse.x/iResolution.x * 2.0 * M_PI * 8.0;
    time = mousePos;
	*/
  float time = 0.f;
  float rotTimeScale = 0.3;
	// Normal set up code for ray trace.
	vec2 xy = fragCoord.xy / iResolution.xy;
	vec2 s = (-1.0 + 2.0* xy) * vec2(iResolution.x/iResolution.y, 1.0);
    
    vec3 planeNormal = normalize(vec3(0.0, 1.0, 0.0));
    vec3 planeUp = abs(planeNormal.y) < 0.5
        ? vec3(0.0, 1.0, 0.0)
        : vec3(0.0, 0.0, 1.0);
   	vec3 planeSide = normalize(cross(planeNormal, planeUp));
    planeUp = cross(planeSide, planeNormal);
	
    float dist = 8.0;
    float height = 2.0;
    
	//vec3 ro = vec3(sin(time * rotTimeScale) * dist, height, cos(time * rotTimeScale) * dist);
  //vec3 ro = (Camera*vec4(0,1,0,0)).xyz;
  //ro.y += (planeSide*ro.x + planeNormal*ro.y + planeUp*ro.z).y;
	//vec3 cd = normalize(-ro);
	//vec3 cu = vec3(0.0, 1.0, 0.0);
	//vec3 cr = normalize(cross(cd, cu));
	//cu = normalize(cross(cr, cd));

	//vec3 rd = normalize( s.x*cr + s.y*cu + 2.0*cd );

	vec3 col = vec3(0.7, 0.7, 0.7);
    	
  vec3 sunDir = normalize(vec3(-1, -1, -1));
  vec3 ambientLight = vec3(0.3);
  vec3 sunLight = vec3(0.9);
    
  vec3 ro = (Camera*vec4(0,0,0,1)).xyz;
  ro.y += 1;
  vec3 rx = (Camera*vec4(1,0,0,0)).xyz;
  vec3 ry = (Camera*vec4(0,1,0,0)).xyz;
  vec3 rz = (Camera*vec4(0,0,1,0)).xyz;
  vec3 rd = normalize( s.x*rx + s.y*ry + 2.0*rz );

    // transform ray
    ro = vec3(
        dot(ro, planeSide),
        dot(ro, planeNormal),
        dot(ro, planeUp)
    );
    rd = vec3(
        dot(rd, planeSide),
        dot(rd, planeNormal),
        dot(rd, planeUp)
    );
    
    
    float t = intersect(ro, rd);
    
    if(t > 0.0)
    {	
		// Get some information about our intersection
		vec3 pos = ro + t * rd;
		vec3 normal = calcNormal(pos, t);
        
    float shadow = intersect(pos, -sunDir) > 0.0 ? 0.0 : 1.0;
		
		vec2 uvp = vec2(dot(pos, planeUp), dot(pos, planeSide));
		
		vec3 texCol = getColor(pos);
		
		col = texCol * clamp(-dot(normal, sunDir), 0.0f, 1.0f) * shadow * 1.2 + 0.2 * texCol; 
	}
	
	fragColor = vec4(pow(col*1.2, vec3(2.2)), 1.0);
}

void main() 
  {
  mainImage(FragColor, gl_FragCoord.xy);
  }
)");
  }

terrain_material::terrain_material()
  {
  vs_handle = -1;
  fs_handle = -1;
  shader_program_handle = -1;
  proj_handle = -1;
  cam_handle = -1;
  res_handle = -1;  
  texture_heightmap = -1;
  texture_normalmap = -1;
  texture_colormap = -1;
  heightmap_handle = -1;
  normalmap_handle = -1;
  colormap_handle = -1;
  }

terrain_material::~terrain_material()
  {
  }

void terrain_material::destroy(RenderDoos::render_engine* engine)
  {
  engine->remove_shader(vs_handle);
  engine->remove_shader(fs_handle);
  engine->remove_program(shader_program_handle);
  engine->remove_uniform(proj_handle);
  engine->remove_uniform(cam_handle);
  engine->remove_uniform(res_handle);
  engine->remove_uniform(heightmap_handle);
  engine->remove_uniform(normalmap_handle);
  engine->remove_uniform(colormap_handle);
  }

void terrain_material::set_texture_heightmap(int32_t id)
  {
  texture_heightmap = id;
  }

void terrain_material::set_texture_normalmap(int32_t id)
  {
  texture_normalmap = id;
  }

void terrain_material::set_texture_colormap(int32_t id)
  {
  texture_colormap = id;
  }

void terrain_material::compile(RenderDoos::render_engine* engine)
  {
  if (engine->get_renderer_type() == RenderDoos::renderer_type::METAL)
    {  
    vs_handle = engine->add_shader(nullptr, SHADER_VERTEX, "terrain_material_vertex_shader");
    fs_handle = engine->add_shader(nullptr, SHADER_FRAGMENT, "terrain_material_fragment_shader");
    }
  else if (engine->get_renderer_type() == RenderDoos::renderer_type::OPENGL)
    {
    vs_handle = engine->add_shader(get_terrain_material_vertex_shader().c_str(), SHADER_VERTEX, nullptr);
    fs_handle = engine->add_shader(get_terrain_material_fragment_shader().c_str(), SHADER_FRAGMENT, nullptr);
    }
  shader_program_handle = engine->add_program(vs_handle, fs_handle);
  proj_handle = engine->add_uniform("Projection", RenderDoos::uniform_type::mat4, 1);
  cam_handle = engine->add_uniform("Camera", RenderDoos::uniform_type::mat4, 1);
  res_handle = engine->add_uniform("iResolution", RenderDoos::uniform_type::vec3, 1);
  heightmap_handle = engine->add_uniform("Heightmap", RenderDoos::uniform_type::sampler, 1);
  normalmap_handle = engine->add_uniform("Normalmap", RenderDoos::uniform_type::sampler, 1);
  colormap_handle = engine->add_uniform("Colormap", RenderDoos::uniform_type::sampler, 1);
  }

void terrain_material::bind(RenderDoos::render_engine* engine)
  {
  engine->bind_program(shader_program_handle);
  engine->set_uniform(proj_handle, (void*)(&engine->get_projection()));
  RenderDoos::float4x4 cam = (engine->get_camera_space());
  engine->set_uniform(cam_handle, (void*)(&cam));
  const auto& mv = engine->get_model_view_properties();
  float res[3] = { (float)mv.viewport_width, (float)mv.viewport_height, 1.f };
  engine->set_uniform(res_handle, (void*)res);
  int32_t tex = 0;
  engine->set_uniform(heightmap_handle, (void*)&tex);
  tex = 1;
  engine->set_uniform(normalmap_handle, (void*)&tex);
  tex = 2;
  engine->set_uniform(colormap_handle, (void*)&tex);

  engine->bind_texture_to_channel(texture_heightmap, 0, TEX_WRAP_REPEAT | TEX_FILTER_LINEAR);
  engine->bind_texture_to_channel(texture_normalmap, 1, TEX_WRAP_REPEAT | TEX_FILTER_LINEAR);
  engine->bind_texture_to_channel(texture_colormap, 2, TEX_WRAP_REPEAT | TEX_FILTER_LINEAR);

  engine->bind_uniform(shader_program_handle, proj_handle);
  engine->bind_uniform(shader_program_handle, cam_handle);
  engine->bind_uniform(shader_program_handle, res_handle);
  engine->bind_uniform(shader_program_handle, heightmap_handle);
  engine->bind_uniform(shader_program_handle, normalmap_handle);
  engine->bind_uniform(shader_program_handle, colormap_handle);
  }
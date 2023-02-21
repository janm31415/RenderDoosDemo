 #include <metal_stdlib>
using namespace metal;

struct VertexIn {
  packed_float3 position;
  packed_float3 normal;
  packed_float2 textureCoordinates;
};

struct TerrainMaterialUniforms {
  float4x4 projection_matrix;
  float4x4 camera_matrix;
  float3 resolution;
  int heightmap_handle;
  int normalmap_handle;
  int colormap_handle;
};

struct VertexOut {
  float4 position [[position]];
};

vertex VertexOut terrain_material_vertex_shader(const device VertexIn *vertices [[buffer(0)]], uint vertexId [[vertex_id]], constant TerrainMaterialUniforms& input [[buffer(10)]]) {
  VertexOut out;
  float4 pos(vertices[vertexId].position, 1);
  out.position = input.projection_matrix * pos;
  return out;
}


float2 scalePosition(float2 p)
{
  p = p*0.02;
  p = p+float2(0.5);
  p = mix(float2(0.5, 0.0), float2(1.0, 0.5), p);
  return p;
}

float terrain( float2 pos, texture2d<float> Heightmap, sampler sampler2d)
{
  float2 p = scalePosition(pos);
  if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
    return 0.0;
  return Heightmap.sample(sampler2d, p).r*5.0;
}

float map( float3 p,  texture2d<float> Heightmap, sampler sampler2d)
{
    return p.y - terrain(p.xz, Heightmap, sampler2d);
}

float intersect( float3 ro, float3 rd, texture2d<float> Heightmap, sampler sampler2d)
{
    const float maxd = 40.0;
    const float precis = 0.001;
    float t = 0.0;
    for( int i=0; i<256; i++ )
    {
        float h = map( ro+rd*t, Heightmap, sampler2d);
        if( abs(h)<precis || t>maxd ) break;
        t += h*0.5;
    }
    return (t>maxd)?-1.0:t;
}

float3 calcNormal( float3 pos, float t, texture2d<float> Normalmap, sampler sampler2d)
{
  float2 p = scalePosition(pos.xz);
  if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
    return float3(0,1,0);
  return Normalmap.sample(sampler2d, p).rgb;
}

float4 getColor(float3 pos, texture2d<float> Colormap, sampler sampler2d)
{
  float2 p = scalePosition(pos.xz);
  if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
    return float4(0,0,0,0);
  return Colormap.sample(sampler2d, p);
}

fragment float4 terrain_material_fragment_shader(const VertexOut vertexIn [[stage_in]], texture2d<float> heightmap [[texture(0)]], texture2d<float> normalmap [[texture(1)]], texture2d<float> colormap [[texture(2)]], sampler sampler2d [[sampler(0)]], constant TerrainMaterialUniforms& input [[buffer(10)]]) {
  //return colormap.sample(sampler2d, vertexIn.position.xy/input.resolution.xy);
  float2 xy = vertexIn.position.xy / input.resolution.xy;
  xy.y = 1-xy.y;
	float2 s = (-1.0 + 2.0* xy) * float2(input.resolution.x/input.resolution.y, 1.0);
    
  float3 planeNormal = normalize(float3(0.0, 1.0, 0.0));
  float3 planeUp = abs(planeNormal.y) < 0.5
      ? float3(0.0, 1.0, 0.0)
      : float3(0.0, 0.0, 1.0);
  float3 planeSide = normalize(cross(planeNormal, planeUp));
  planeUp = cross(planeSide, planeNormal);
	
	float3 col = float3(0.7, 0.7, 0.7);
    
  float3 ro = (input.camera_matrix*float4(0,0,0,1)).xyz;
  ro.y += 3;
  float3 rx = (input.camera_matrix*float4(1,0,0,0)).xyz;
  float3 ry = (input.camera_matrix*float4(0,1,0,0)).xyz;
  float3 rz = (input.camera_matrix*float4(0,0,1,0)).xyz;
  float3 rd = normalize( s.x*rx + s.y*ry + 2.0*rz );

  // transform ray
  ro = float3(
      dot(ro, planeSide),
      dot(ro, planeNormal),
      dot(ro, planeUp)
  );
  rd = float3(
      dot(rd, planeSide),
      dot(rd, planeNormal),
      dot(rd, planeUp)
  );
    
    
  float t = intersect(ro, rd, heightmap, sampler2d);
    
  if(t > 0.0)
    {
		// Get some information about our intersection
		float3 pos = ro + t * rd;
		float3 normal = calcNormal(pos, t, normalmap, sampler2d);
		float4 texCol = getColor(pos, colormap, sampler2d);
    if (texCol.a > 0)
      {
      float3 terraincol = float3(pow(texCol.rgb, float3(0.5)));
		  float3 sunDir = normalize(float3(0, 0.5, -1));
      terraincol = terraincol * clamp(-dot(normal, sunDir), 0.0f, 1.0f) * 0.9 + terraincol * 0.1;
      terraincol = pow(terraincol*1.2, float3(2.2));
      col = terraincol*texCol.a + col*(1-texCol.a);
      }
	  }
	
	return float4(col, 1.0);
}

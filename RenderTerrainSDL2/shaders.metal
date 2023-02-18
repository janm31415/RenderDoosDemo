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

fragment float4 terrain_material_fragment_shader(const VertexOut vertexIn [[stage_in]], texture2d<float> heightmap [[texture(0)]], texture2d<float> normalmap [[texture(1)]], texture2d<float> colormap [[texture(2)]], sampler sampler2d [[sampler(0)]], constant TerrainMaterialUniforms& input [[buffer(10)]]) {
  return colormap.sample(sampler2d, vertexIn.position.xy/input.resolution.xy);
}

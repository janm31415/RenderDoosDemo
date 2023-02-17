#include <metal_stdlib>
using namespace metal;


struct VertexIn {
  packed_float3 position;
  packed_float3 normal;
  packed_float2 textureCoordinates;
};

struct CubeMaterialUniforms {
  float4x4 projection_matrix;
  float4x4 camera_matrix;
};

struct VertexOut {
  float4 position [[position]];
  float3 localpos;
};

vertex VertexOut cube_material_vertex_shader(const device VertexIn *vertices [[buffer(0)]], uint vertexId [[vertex_id]], constant CubeMaterialUniforms& input [[buffer(10)]]) {
  VertexOut out;
  out.localpos = vertices[vertexId].position;
  float4 pos(vertices[vertexId].position, 1);
  out.position = input.projection_matrix * input.camera_matrix * pos;
  return out;
}

fragment float4 cube_material_fragment_shader(const VertexOut vertexIn [[stage_in]], texturecube<float> texture [[texture(0)]], sampler cubesampler [[sampler(0)]], constant CubeMaterialUniforms& input [[buffer(10)]]) {
  return texture.sample(cubesampler, vertexIn.localpos);
}

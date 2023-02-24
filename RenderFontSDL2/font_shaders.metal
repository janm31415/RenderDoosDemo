#include <metal_stdlib>
using namespace metal;

struct FontVertexIn {
  packed_float2 position;
  packed_float2 textureCoordinates;
  packed_float3 color;
};

struct FontMaterialUniforms {
  int width;
  int height;
};

struct FontVertexOut {
  float4 position [[position]];
  float2 texcoord;
  float3 color;
};

vertex FontVertexOut font_material_vertex_shader(const device FontVertexIn *vertices [[buffer(0)]], uint vertexId [[vertex_id]], constant FontMaterialUniforms& input [[buffer(10)]]) {
  float4 pos(vertices[vertexId].position, 1);
  FontVertexOut out;
  out.position = pos;
  out.texcoord = vertices[vertexId].textureCoordinates;
  out.color = vertices[vertexId].color;
  return out;
}

fragment float4 font_material_fragment_shader(const FontVertexOut vertexIn [[stage_in]], texture2d<uint> texture [[texture(0)]], constant FontMaterialUniforms& input [[buffer(10)]]) {
  int x = int(vertexIn.texcoord.x * float(input.width));
  int y = int(vertexIn.texcoord.y * float(input.height));
  float a = texture.read(uint2(x,y)).r/255.0;
  return float4(1, 1, 1, a)*float4(vertexIn.color, 1);
}

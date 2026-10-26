cbuffer UBO : register(b0, space1)
{
  float4x4 g_modelViewProj;
};

struct VSInput {
  float3 position : POSITION;
};

struct VSOutput {
  float4 position : SV_Position;
};

VSOutput main(VSInput input) {
  VSOutput output;
  output.position = mul(g_modelViewProj, float4(input.position, 1));
  return output;
}

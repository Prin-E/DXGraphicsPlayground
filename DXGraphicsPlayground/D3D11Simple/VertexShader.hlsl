cbuffer cbObjectInfo : register(b0)
{
	float4x4 model;
	float4x4 view;
	float4x4 projection;
	float time;
};

struct VertexOut {
	float4 pos : SV_POSITION;
	float3 color : COLOR;
	float2 uv : TEXCOORD;
};

VertexOut main(float3 pos : POSITION, float3 color : COLOR, uint iid : SV_InstanceID)
{
	VertexOut result;
	result.pos = float4(pos, 1.0);
	result.pos = mul(result.pos, model);
	result.pos = mul(result.pos, view);
	//result.pos.y += iid * 0.125;
	//result.pos.z += iid * 0.5;
	result.pos = mul(result.pos, projection);
	result.color = color;
	result.uv = (0.5 * (pos + 1.0)).xz;
	return result;
}
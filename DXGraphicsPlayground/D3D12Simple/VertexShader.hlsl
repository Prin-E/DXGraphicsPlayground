cbuffer cbObjectInfo : register(b0)
{
	float4x4 rotation;
	float4x4 projection;
};

struct FragmentInput {
	float4 pos : SV_POSITION;
	float3 color : COLOR;
};

FragmentInput main(float3 pos : POSITION, float3 color : COLOR)
{
	FragmentInput result;
	result.pos = float4(pos, 1.0);
	result.pos = mul(result.pos, rotation);
	result.pos = mul(result.pos, projection);
	result.color = color;
	return result;
}
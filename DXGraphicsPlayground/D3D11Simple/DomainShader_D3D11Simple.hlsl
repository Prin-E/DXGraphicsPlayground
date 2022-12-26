struct DomainOut
{
	float4 pos : SV_POSITION;
	float3 color : COLOR;
	float2 uv : TEXCOORD;
};

struct HullControlPointOut
{
	float4 pos : SV_POSITION;
	float3 color : COLOR;
	float2 uv : TEXCOORD;
};

struct HullConstantOut
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

Texture2D tex : register(t0);
SamplerState s : register(s0);

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DomainOut main(
	HullConstantOut input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HullControlPointOut, 3> patch)
{
	DomainOut Output;

	float4 pos = patch[0].pos * domain.x + patch[1].pos * domain.y + patch[2].pos * domain.z;
	float3 color = patch[0].color * domain.x + patch[1].color * domain.y + patch[2].color * domain.z;
	float2 uv = patch[0].uv * domain.x + patch[1].uv * domain.y + patch[2].uv * domain.z;
	float4 tc = tex.SampleLevel(s, uv, 0);
	pos /= pos.w;
	pos.y += tc.r * 0.5;

	Output.pos = pos;
	Output.color = color;
	Output.uv = uv;
	return Output;
}

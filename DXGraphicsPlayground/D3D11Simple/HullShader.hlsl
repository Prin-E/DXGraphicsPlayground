struct VertexOut
{
	float4 pos : POSITION;
	float3 color : COLOR;
	float2 uv : TEXCOORD;
};

struct HullControlPointOut
{
	float4 pos : POSITION;
	float3 color : COLOR;
	float2 uv : TEXCOORD;
};

struct HullConstantOut
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

cbuffer cbObjectInfo : register(b0)
{
	float4x4 model;
	float4x4 view;
	float4x4 projection;
	float time;
};

HullConstantOut CalcHSPatchConstants(
	InputPatch<VertexOut, 3> ip,
	uint PatchID : SV_PrimitiveID)
{
	HullConstantOut Output;

	[unroll]
	for (uint i = 0; i < 3; i++) {
		Output.EdgeTessFactor[i] = clamp(3.0f - ip[i].pos.z / ip[i].pos.w, 0.5f, 3.0f) * 10.0f;
	}
	Output.InsideTessFactor = 0.333f * (Output.EdgeTessFactor[0] + Output.EdgeTessFactor[1] + Output.EdgeTessFactor[2]);

	return Output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HullControlPointOut main(
	InputPatch<VertexOut, 3> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HullControlPointOut Output;

	Output.pos = ip[i].pos;
	Output.color = ip[i].color;
	Output.uv = ip[i].uv;

	return Output;
}

struct FragmentInput {
	float4 pos : SV_POSITION;
	float3 color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState s : register(s0);

float4 main(FragmentInput input) : SV_TARGET
{
	float4 tc = tex.Sample(s, input.uv);
	float4 outColor = float4(input.color, 1.0);
	return tc;
}
struct FragmentInput {
	float4 pos : SV_POSITION;
	float3 color : COLOR;
};

float4 main(FragmentInput input) : SV_TARGET
{
	float4 outColor = float4(input.color, 1.0);
	return outColor;
}
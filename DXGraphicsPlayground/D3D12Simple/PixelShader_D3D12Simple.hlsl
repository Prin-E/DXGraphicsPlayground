cbuffer CommonProps : register(b0) {
	float normalizedSDRWhiteLevel;
	bool isST2084Output;
}

struct FragmentInput {
	float4 pos : SV_POSITION;
	float3 color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState s : register(s0);

// https://github.com/microsoft/DirectX-Graphics-Samples
float3 LinearToST2084(float3 color)
{
	float m1 = 2610.0 / 4096.0 / 4;
	float m2 = 2523.0 / 4096.0 * 128;
	float c1 = 3424.0 / 4096.0;
	float c2 = 2413.0 / 4096.0 * 32;
	float c3 = 2392.0 / 4096.0 * 32;
	float3 cp = pow(abs(color), m1);
	return pow((c1 + c2 * cp) / (1 + c3 * cp), m2);
}

// https://github.com/microsoft/DirectX-Graphics-Samples
float3 Rec709ToRec2020(float3 color)
{
	static const float3x3 conversion =
	{
		0.627402, 0.329292, 0.043306,
		0.069095, 0.919544, 0.011360,
		0.016394, 0.088028, 0.895578
	};
	return mul(conversion, color);
}

// https://github.com/microsoft/DirectX-Graphics-Samples
float3 LinearToSRGB(float3 color)
{
	// Approximately pow(color, 1.0 / 2.2)
	return color < 0.0031308 ? 12.92 * color : 1.055 * pow(abs(color), 1.0 / 2.4) - 0.055;
}

float4 main(FragmentInput input) : SV_TARGET
{
	float4 tc = tex.Sample(s, input.uv);
	if (isST2084Output) {
		tc.rgb = Rec709ToRec2020(tc.rgb);
		tc.rgb = LinearToST2084(tc.rgb * normalizedSDRWhiteLevel);
	}
	else {
		tc.rgb = LinearToSRGB(tc.rgb);
	}
	return tc;
}
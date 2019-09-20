#pragma once

cbuffer CameraProps : register(b1) {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float4x4 rotation;
	float4x4 viewInverse;
	float4x4 projectionInverse;
	float4x4 viewProjectionInverse;
	float4x4 rotationInverse;
};

cbuffer InstanceProps : register(b2) {
	float4x4 model;
}

struct VertexInput {
	float3 pos : POSITION;
	float3 uv : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct PixelInput {
	float4 clipPos : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL0;
	float3 tangent : NORMAL1;
	float3 bitangent : NORMAL2;
};
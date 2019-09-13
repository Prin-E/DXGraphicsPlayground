#pragma once

#include <string>
#include <d3dcommon.h>

inline std::string FeatureLevelToString(D3D_FEATURE_LEVEL featureLevel) {
	switch (featureLevel) {
	case D3D_FEATURE_LEVEL_9_1:
		return "9.1";
	case D3D_FEATURE_LEVEL_9_2:
		return "9.2";
	case D3D_FEATURE_LEVEL_9_3:
		return "9.3";
	case D3D_FEATURE_LEVEL_10_0:
		return "10.0";
	case D3D_FEATURE_LEVEL_10_1:
		return "10.1";
	case D3D_FEATURE_LEVEL_11_0:
		return "11.0";
	case D3D_FEATURE_LEVEL_11_1:
		return "11.1";
	case D3D_FEATURE_LEVEL_12_0:
		return "12.0";
	case D3D_FEATURE_LEVEL_12_1:
		return "12.1";
	default:
		return "unknown";
	}
}
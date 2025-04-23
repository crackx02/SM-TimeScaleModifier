#pragma once

#include <SmSdk/Gui/OptionsSubMenuBase.hpp>

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

class GraphicsOptionsMenu : public OptionsSubMenuBase
{
public:
	constexpr static float TimeScaleMin = 0.0f;
	constexpr static float TimeScaleMax = 20.0f;
	inline static float TimeScale = 1.0f;

	inline static __int64 (*o_CreateWidgets)(GraphicsOptionsMenu*, MyGUI::Widget* parent) = nullptr;
	static __int64 h_CreateWidgets(GraphicsOptionsMenu* self, MyGUI::Widget* parent);
}; // Size: 0x248
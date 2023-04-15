#pragma once

#include <Windows.h>

#include <d3d9.h>
#include <D3dx9tex.h>

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imstb_textedit.h"

#include "../imgui/pig.h"
#include "../imgui/pigstars.h"
#include "../imgui/syb.h"
#include "../imgui/stars.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#include "../imgui/Museo.h"
#include "../imgui/Museo700.h"
#include "../imgui/Museo900.h"

#define museo1 museo900
#define museo2 museo700
#define museo3 museo

#ifndef _DEBUG
//wtf
const ImVec2 operator+(const ImVec2& rv, const ImVec2& lv)
{
	return ImVec2(lv.x + rv.x, lv.y + rv.y);
}
ImVec2 operator-(const ImVec2& rv, const ImVec2& lv)
{
	return ImVec2(rv.x - lv.x, rv.y - lv.y);
}
const ImVec2 operator*(const ImVec2& rv, const ImVec2& lv)
{
	return ImVec2(lv.x * rv.x, lv.y * rv.y);
}
const ImVec2 operator/(const ImVec2& rv, const ImVec2& lv)
{
	return ImVec2(rv.x / lv.x, rv.y / lv.y);
}
#endif

#include "imstb.h"

enum e_animation_type : uint8_t
{
	animation_static = 0u,
	animation_dynamic,
	animation_interp
};

enum e_tex_id : uint8_t
{
	tex_pig = 0u,
	tex_syb,
	tex_max
};

enum e_font_id : uint8_t
{
	font_big = 0u,
	font_small,
	font_default,
	font_middle,
	font_giant,
	font_max
};

class c_oink_ui
{
public:
	c_oink_ui( ) : m_menu_opened{ false }, m_active_tab{ 0 }, m_dpi_changed{ false }, m_dpi_scaling{ 1.f }, m_theme_colour{ 47.f / 255.f, 70.f / 255.f, 154.f / 255.f }, m_gap{ 12.f }
	{
		memset(m_textures, 0, sizeof(m_textures));
		memset(m_fonts, 0, sizeof(m_fonts));

		m_theme_colour_backup = m_theme_colour;
		m_dpi_scaling_backup = m_dpi_scaling;
	};

	~c_oink_ui( )
	{
	};
private:
	const char* m_key_names[166] =
	{
	"Unknown",
	"Left mouse",
	"Right mouse",
	"Cancel",
	"Middle mouse",
	"X1 mouse",
	"X2 mouse",
	"Unknown",
	"Back",
	"Tab",
	"Unknown",
	"Unknown",
	"Clear",
	"Return",
	"Unknown",
	"Unknown",
	"Shift",
	"Ctrl",
	"Menu",
	"Pause",
	"Capital",
	"KANA",
	"Unknown",
	"VK_JUNJA",
	"VK_FINAL",
	"VK_KANJI",
	"Unknown",
	"Escape",
	"Convert",
	"NonConvert",
	"Accept",
	"VK_MODECHANGE",
	"Space",
	"Prior",
	"Next",
	"End",
	"Home",
	"Left",
	"Up",
	"Right",
	"Down",
	"Select",
	"Print",
	"Execute",
	"Snapshot",
	"Insert",
	"Delete",
	"Help",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"Win left",
	"Win right",
	"Apps",
	"Unknown",
	"Sleep",
	"Numpad 0",
	"Numpad 1",
	"Numpad 2",
	"Numpad 3",
	"Numpad 4",
	"Numpad 5",
	"Numpad 6",
	"Numpad 7",
	"Numpad 8",
	"Numpad 9",
	"Multiply",
	"Add",
	"Seperator",
	"Subtract",
	"Decimal",
	"Devide",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"F13",
	"F14",
	"F15",
	"F16",
	"F17",
	"F18",
	"F19",
	"F20",
	"F21",
	"F22",
	"F23",
	"F24",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Numlock",
	"Scroll",
	"VK_OEM_NEC_EQUAL",
	"VK_OEM_FJ_MASSHOU",
	"VK_OEM_FJ_TOUROKU",
	"VK_OEM_FJ_LOYA",
	"VK_OEM_FJ_ROYA",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Shift left",
	"Shift right",
	"Ctrl left",
	"Ctrl right",
	"Left menu",
	"Right menu"
	};
	IDirect3DTexture9* m_textures[tex_max];
	ImFont* m_fonts[font_max];
private:
	std::unordered_map<ImGuiID, float> m_animations;
	float m_gap;
	int m_active_tab;
	bool m_menu_opened;
	bool m_dpi_changed;
	float m_dpi_scaling;
	float m_dpi_scaling_backup;
	float m_border_alpha;
	ImColor m_theme_colour;
	ImColor m_theme_colour_backup;
public:
	void textures_create(IDirect3DDevice9* device);
	void fonts_create(bool invalidate = false);
	void draw_menu( );
	void pre_draw_menu( );
	void terminate_menu( );
private:

	__forceinline float CalcMaxPopupHeight(int items_count)
	{
		ImGuiContext* g = GImGui;
		return (g && items_count > 0) ? ((g->FontSize + g->Style.ItemSpacing.y) * items_count - g->Style.ItemSpacing.y + (g->Style.WindowPadding.y * 2)) : FLT_MAX;
	}

	__forceinline void ImRotateStart(int& rotation_start_index)
	{
		rotation_start_index = ImGui::GetBackgroundDrawList( )->VtxBuffer.Size;
	};

	__forceinline ImVec2 ImRotationCenter(int rotation_start_index)
	{
		ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

		const auto& buf = ImGui::GetBackgroundDrawList( )->VtxBuffer;

		for (int i = rotation_start_index; i < buf.Size; i++)
			l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

		return { (l.x + u.x) / 2, (l.y + u.y) / 2 }; // or use _ClipRectStack?
	};

	__forceinline void ImRotateEnd(float rad, int rotation_start_index)
	{
		ImVec2 center = ImRotationCenter(rotation_start_index);
		float s = ImSin(rad), c = ImCos(rad);
		center = ImRotate(center, s, c) - center;

		auto& buf = ImGui::GetBackgroundDrawList( )->VtxBuffer;

		for (int i = rotation_start_index; i < buf.Size; i++)
			buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
	};

public:
	float process_animation(const char* label, unsigned int seed, bool if_, float v_max, float percentage_speed = 1.0f, e_animation_type type = e_animation_type::animation_static);
	float process_animation(ImGuiID id, bool if_, float v_max, float percentage_speed = 1.0f, e_animation_type type = e_animation_type::animation_static);
	ImGuiID generate_unique_id(const char* label, unsigned int seed);
private:
	void configure(ImDrawList* bg_drawlist, ImVec2& m_menu_pos, ImVec2& m_menu_size, bool main = true);

	//buttons
	bool sub_button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, int this_tab, int opened_tab);
	bool tab_button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, int this_tab, int opened_tab);

	bool button(const char* label, const ImVec2& size_arg);

	bool checkbox(const char* label, bool* v);

	bool slider_int(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
	bool slider_float(const char* label, float* v, float v_min, float v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
	bool slider_scalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, const ImColor& theme);

	bool combo_box(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items);
	bool combo_box(const char* label, int* current_item, void* const items, int items_count, int height_in_items = -1);

	void multi_box(const char* title, bool selection[ ], const char* text[ ], int size);

	bool selectable(const char* label, bool* selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = ImVec2(0, 0));

	void text(const char* text);
	void text_colored(const char* text);

	bool begin_child(const char* label, int number_of_child);
	void end_child( );

	bool input_text(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = NULL, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	bool temp_input_text(const ImRect& bb, ImGuiID id, const char* label, char* buf, int buf_size, ImGuiInputTextFlags flags);
	bool temp_input_scalar(const ImRect& bb, ImGuiID id, const char* label, ImGuiDataType data_type, void* p_data, const char* format, const void* p_clamp_min, const void* p_clamp_max);

	bool hotkey(const char* label, int* k, bool* controlled_value = NULL);

	bool color_picker(const char* sz, float* col, bool alpha_bar = true);
	bool color_picker_button(const char* label, float* col, bool draw_on_same_line = false, bool alpha_bar = true);
};

inline c_oink_ui g_ui;
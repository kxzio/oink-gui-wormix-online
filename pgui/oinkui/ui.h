#pragma once

#include <Windows.h>

#include <d3d9.h>
#include <D3dx9tex.h>

#include "../imgui/imgui.h"
#include "../imgui/imgui_freetype.h"
#include "../imgui/imgui_internal.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imstb_textedit.h"

#include "pig_image.h"

#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>

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

enum e_keybind_mode : uint8_t
{
	keybind_mode_always_on = 0,
	keybind_mode_onpress,
	keybind_mode_toggle,
};

enum e_tex_id : uint8_t
{
	tex_pig = 0u,
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

struct pig_data_t
{
	pig_data_t( )
	{
	};

	pig_data_t(const ImVec2& position, float rotation, float speed_x, float speed_y, int rotation_index) :
		m_position{ position },
		m_rotation{ rotation },
		m_rotation_index{ rotation_index },
		m_speed{ speed_x, speed_y }
	{
	};

	ImVec2 m_position;
	float m_rotation;
	int m_rotation_index;
	ImVec2 m_speed;
};

struct keybind_t
{
	static constexpr uint16_t keybind_unbound = UINT16_MAX;
	static constexpr uint16_t keybind_key_wait = ImGuiMouseButton_COUNT;

	keybind_t( ) :
		m_keycode{ keybind_unbound },
		m_activation_mode{ e_keybind_mode::keybind_mode_onpress },
		m_active{ false }
	{
	};

	bool    m_active;
	uint16_t m_keycode;
	e_keybind_mode m_activation_mode;
};

inline float get_random_number(float min, float max)
{
	float random = min + static_cast <float> (rand( )) / (static_cast <float> (RAND_MAX / (max - min)));
	return random;
}

class c_oink_ui
{
public:
	c_oink_ui( )
	{
	};

	c_oink_ui(IDirect3DDevice9* device) :
		m_menu_opened{ false },
		m_dpi_changed{ false },
		m_dpi_scaling{ 1.f },
		m_theme_hsv_backup{ 227.f / 360.f, 69.f / 100.f, 59.f / 100.f },
		m_gap{ 12.f },
		m_active_tab{ 0 }
	{
		ZeroMemory(m_textures, sizeof(m_textures));
		ZeroMemory(m_fonts, sizeof(m_fonts));

		m_dpi_scaling_backup = m_dpi_scaling;
		m_theme_colour_primary.Value.w = 1.f;
		m_theme_colour_secondary.Value.w = 1.f;

		for (size_t i = 0u; i < m_pigs_data.size( ); ++i)
		{
			m_pigs_data[i] = pig_data_t(
			ImVec2(get_random_number(0, 100.f), get_random_number(0.f, 100.f)),
			get_random_number(0.f, 360.f),
			get_random_number(-50.f, 50.f),
			get_random_number(-50.f, 50.f),
			i);
		};

		textures_create(device);
		fonts_create(false);
	};

	~c_oink_ui( )
	{
		ZeroMemory(m_fonts, sizeof(m_fonts));
		ZeroMemory(m_textures, sizeof(m_textures));
	};
public:
	void draw_menu( );
	void pre_draw_menu( );
private:
	__forceinline float calc_max_popup_height(int items_count)
	{
		ImGuiContext* g = GImGui;
		return (g && items_count > 0) ? ((g->FontSize + g->Style.ItemSpacing.y) * items_count - g->Style.ItemSpacing.y + (g->Style.WindowPadding.y * 2.f)) : FLT_MAX;
	};

	__forceinline void rotate_start(ImDrawList* draw_list, int& rotation_start_index)
	{
		rotation_start_index = draw_list->VtxBuffer.Size;
	};

	__forceinline ImVec2 rotation_center(ImDrawList* draw_list, int rotation_start_index)
	{
		ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

		const auto& buf = draw_list->VtxBuffer;

		for (int i = rotation_start_index; i < buf.Size; i++)
			l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

		return { (l.x + u.x) / 2, (l.y + u.y) / 2 }; // or use _ClipRectStack?
	};

	__forceinline void rotate_end(ImDrawList* draw_list, float rad, int rotation_start_index)
	{
		ImVec2 center = rotation_center(draw_list, rotation_start_index);
		float s = ImSin(rad), c = ImCos(rad);
		center = ImRotate(center, s, c) - center;

		auto& buf = draw_list->VtxBuffer;

		for (int i = rotation_start_index; i < buf.Size; i++)
			buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
	};
public:
	float process_animation(const char* label, unsigned int seed, bool if_, float v_max, float percentage_speed = 1.0f, e_animation_type type = e_animation_type::animation_static);
	float process_animation(ImGuiID id, bool if_, float v_max, float percentage_speed = 1.0f, e_animation_type type = e_animation_type::animation_static);
	ImGuiID generate_unique_id(const char* label, unsigned int seed);
private:
	void textures_create(IDirect3DDevice9* device);
	void fonts_create(bool invalidate = false);
private:

	bool begin_window(const char* name, bool* p_open, ImGuiWindowFlags flags, bool p = false);

	void render_cursor(ImDrawList* fg_drawlist);

	void configure(ImDrawList* bg_drawlist, ImVec2& m_menu_pos, ImVec2& m_menu_size, bool main = true);

	//buttons
	bool sub_button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, int this_tab, int opened_tab);
	bool tab_button(const char* label, ImVec2 size_arg, ImGuiButtonFlags flags, bool tab_active);

	bool button(const char* label, const ImVec2& size_arg = ImVec2(0.f, 0.f));
	bool button_ex(const char* label, const ImVec2& size_arg = ImVec2(0.f, 0.f), const ImGuiButtonFlags& flags = ImGuiButtonFlags_None);

	bool checkbox(const char* label, bool* v);

	bool slider_int(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
	bool slider_float(const char* label, float* v, float v_min, float v_max, const char* format = "%.2f", ImGuiSliderFlags flags = 0);
	bool slider_scalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, const ImColor& theme);

	bool combo_box(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items);
	bool combo_box(const char* label, int* current_item, void* const items, int items_count, int height_in_items = -1);
	bool combo_box(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items = -1);
	void multi_box(const char* title, bool selection[ ], const char* text[ ], int size);

	bool selectable(const char* label, bool* selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = ImVec2(0, 0));

	void text(const char* text);
	void text_colored(const char* text, const ImColor& color);

	void same_line(const float offset_x = 0.f, const float spacing = 1.f);

	void set_cursor_pos(const ImVec2& v);
	void set_cursor_pos_x(const float x);
	void set_cursor_pos_y(const float y);
	float get_cursor_pos_x( );
	float get_cursor_pos_y( );

	bool begin_child(const char* label, int number_of_child);
	bool begin_child_ex(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags);
	bool create_child(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags);
	void end_child( );

	bool begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
	void end( );

	void separator_ex(ImGuiSeparatorFlags flags);
	void separator( );
	bool input_text(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = NULL, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	bool temp_input_text(const ImRect& bb, ImGuiID id, const char* label, char* buf, int buf_size, ImGuiInputTextFlags flags);
	bool temp_input_scalar(const ImRect& bb, ImGuiID id, const char* label, ImGuiDataType data_type, void* p_data, const char* format, const void* p_clamp_min, const void* p_clamp_max);

	bool hotkey(const char* label, keybind_t* keybind, const ImVec2& size_arg = ImVec2(0.f, 0.f));

	bool color_picker(const char* sz, float* col, bool alpha_bar = true);
	bool color_picker_button(const char* label, float* col, bool draw_on_same_line = false, bool alpha_bar = true);
	bool color_picker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col, const float& dpi_scale);
	bool color_edit4(const char* label, float col[4], ImGuiColorEditFlags flags, const float& dpi_scale);

private:
	std::unordered_map<ImGuiID, float> m_animations;
	std::array<pig_data_t, 20> m_pigs_data;

	IDirect3DTexture9* m_textures[tex_max];
	ImFont* m_fonts[font_max];

	int m_active_tab;
	float m_gap;
	bool m_menu_opened;
	bool m_dpi_changed;
	float m_dpi_scaling;
	float m_dpi_scaling_backup;
	float m_border_alpha;
public:
	ImColor m_theme_colour_primary;
	ImColor m_theme_colour_secondary;
	float m_theme_hsv_backup[3];
};

inline c_oink_ui g_ui;
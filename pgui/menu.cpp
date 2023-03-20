#define _CRT_SECURE_NO_WARNING
#pragma warning(disable : 4996)
#include <stdio.h>
#include "menu.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include <dinput.h>
#include <tchar.h>
#include <d3d9.h>
#include <winuser.h>
#include <string>
#include <sstream>
#include <iostream>
#include <ctime>
#include <map>
#include <vector>
#include <dinput.h>
#include <tchar.h>
#include "imgui/Museo.h"
#include "imgui/Museo900.h"
#include "imgui/Museo700.h"
#include <unordered_map>

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

static float theme_col[4];
static bool first_open = true;
static bool change_first_open = false;
static float dpi_scale_slider = 1.f;

inline std::tm localtime_xp(std::time_t timer)
{
	std::tm bt{};
#if defined(__unix__)
	localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
	localtime_s(&bt, &timer);
#else
	static std::mutex mtx;
	std::lock_guard<std::mutex> lock(mtx);
	bt = *std::localtime(&timer);
#endif
	return bt;
}

// default = "YYYY-MM-DD HH:MM:SS"
inline std::string time_stamp(const std::string& fmt = "%F %T")
{
	auto bt = localtime_xp(std::time(0));
	char buf[64];
	return { buf, std::strftime(buf, sizeof(buf), fmt.c_str( ), &bt) };
}

static float calc_max_popupheight(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

const static int child_x_size_const = 206;

int gap = 10;

int get_random_number(int min, int max)
{
	int num = min + rand( ) % (max - min + 1);
	return num;
}

namespace UGui
{
	HDC hDCScreen;
	int Horres;
	int Vertres;

	bool Checkbox(const char* label, bool* v)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		return ImGui::Checkbox(label, v);
	}

	bool SliderInt(const char* label, int* v, int v_min, int v_max)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		ImGui::SliderInt(label, v, v_min, v_max);
		return true;
	}

	bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		ImGui::SliderFloat(label, v, v_min, v_max, format);
		return true;
	}

	bool ComboBox(const char* label, int* current_item, const char* const items[ ], int items_count)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		ImGui::Combo(label, current_item, items, items_count);
		return true;
	}

	bool TextColored(const char* text)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(47 / 255.f, 70 / 255.f, 154 / 255.f, 1.f));
		ImGui::Text(text);
		ImGui::PopStyleColor( );
		return true;
	}

	bool Text(const char* text)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		ImGui::Text(text);
		return true;
	}

	bool BeginChild(const char* label, int number_of_child)
	{
		ImGui::SetCursorPos(ImVec2(10 * cc_menu::get().dpi_scale + ((child_x_size_const * cc_menu::get().dpi_scale + 10 * cc_menu::get().dpi_scale) * (number_of_child - 1)), 105 * cc_menu::get().dpi_scale));

		ImGuiStyle& style = ImGui::GetStyle( );

		ImGui::BeginChild(label, ImVec2(child_x_size_const, 393));

		style.ItemSpacing = ImVec2(10 * cc_menu::get().dpi_scale, 5 * cc_menu::get().dpi_scale);

		ImGui::SetCursorPos(ImVec2(gap * cc_menu::get().dpi_scale, 10 * cc_menu::get().dpi_scale));
		TextColored(label);

		ImGui::Separator( );
		ImGui::SetCursorPosY(ImGui::GetCursorPosY( ) + 2 * cc_menu::get().dpi_scale);

		return true;
	}

	bool ColorPickerButton(const char* label, float* col, bool draw_on_same_line = false)
	{
		ImGui::SameLine( );
		ImGui::SetCursorPosX(draw_on_same_line ? 160 * cc_menu::get().dpi_scale : 180 * cc_menu::get().dpi_scale);
		ImGui::ColorEdit4(std::string(label + std::string("__color")).c_str( ), col, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_NoInputs);
		return true;
	}

	bool EndChild( )
	{
		ImGuiStyle& style = ImGui::GetStyle( );
		style.ItemSpacing = ImVec2(20 * cc_menu::get().dpi_scale, 0 * cc_menu::get().dpi_scale);
		ImGui::Dummy(ImVec2(10 * cc_menu::get().dpi_scale, 10 * cc_menu::get().dpi_scale));
		ImGui::EndChild( );
		return true;
	}

	void MultiBox(const char* title, bool selection[ ], const char* text[ ], int size)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);

		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, calc_max_popupheight(-1)));

		std::string combo = "";

		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, calc_max_popupheight(-1)));

		std::vector <std::string> vec;

		for (size_t i = 0; i < size; i++)
		{
			if (selection[i])
			{
				combo += text[i];
				combo += ", ";
			}
		}

		if (combo.length( ) > 2)
			combo.erase(combo.length( ) - 2, combo.length( ));

		if (ImGui::BeginCombo(title, combo.c_str( ), ImGuiComboFlags_NoArrowButton))
		{
			std::vector<std::string> vec;
			for (size_t i = 0; i < size; i++)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));

				ImGui::Selectable(text[i], &selection[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
				if (selection[i])
					vec.push_back(text[i]);

				ImGui::PopStyleVar( );
			}

			ImGui::EndCombo( );
		}
	}

	void ColorPicker(const char* text, float* col)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		ImGui::Text(text);
		ImGui::SameLine( );
		ImGui::SetCursorPosX(180 * cc_menu::get().dpi_scale);
		ImGui::ColorEdit4(std::string(text + std::string("__color")).c_str( ), col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder);
	}

	bool Button(const char* text, ImVec2 sz)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		return ImGui::Button(text, sz);
	}

	const char* const KeyNames[ ] = {
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

	bool Hotkey(const char* label, int* k, bool* controlled_value = NULL)
	{
		ImGui::SameLine( );
		ImGui::SetCursorPosX(140 * cc_menu::get().dpi_scale);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY( ) - 2 * cc_menu::get().dpi_scale);

		ImGuiWindow* window = ImGui::GetCurrentWindow( );
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		ImGuiIO& io = g.IO;
		const ImGuiStyle& style = g.Style;

		const ImVec2 stored_cursor_pos = window->DC.CursorPos;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
		ImVec2 size = ImVec2(56, 20);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
		const ImRect total_bb(window->DC.CursorPos, frame_bb.Max);

		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id))
			return false;

		const bool hovered = ImGui::ItemHoverable(frame_bb, id);

		auto ID = id;

		if (hovered)
		{
			ImGui::SetHoveredID(id);
			g.MouseCursor = ImGuiMouseCursor_TextInput;
		}

		const bool user_clicked = hovered && io.MouseClicked[0];

		if (user_clicked)
		{
			if (g.ActiveId != id)
			{
				memset(io.MouseDown, 0, sizeof(io.MouseDown));
				memset(io.KeysDown, 0, sizeof(io.KeysDown));
				*k = 0;
			}
			ImGui::SetActiveID(id, window);
			ImGui::FocusWindow(window);
		}
		else if (io.MouseClicked[0])
		{
			if (g.ActiveId == id)
				ImGui::ClearActiveID( );
		}

		bool value_changed = false;
		int key = *k;

		static std::map <ImGuiID, bool> changemode_open;
		auto change_mode_open_map = changemode_open.find(ID);

		if (change_mode_open_map == changemode_open.end( ))
		{
			changemode_open.insert({ ID, false });
			change_mode_open_map = changemode_open.find(ID);
		}

		static std::map <ImGuiID, int> mode;
		auto mode_map = mode.find(ID);

		if (mode_map == mode.end( ))
		{
			mode.insert({ ID, 1 });
			mode_map = mode.find(ID);
		}

		if (g.ActiveId == id)
		{
			for (auto i = 0; i < 5; i++)
			{
				if (io.MouseDown[i])
				{
					switch (i)
					{
						case 0:
							key = VK_LBUTTON;
							break;
						case 1:
							key = VK_RBUTTON;
							break;
						case 2:
							key = VK_MBUTTON;
							break;
						case 3:
							key = VK_XBUTTON1;
							break;
						case 4:
							key = VK_XBUTTON2;
							break;
					}
					value_changed = true;
					ImGui::ClearActiveID( );
				}
			}
			if (!value_changed)
			{
				for (auto i = VK_BACK; i <= VK_RMENU; i++)
				{
					if (io.KeysDown[i])
					{
						key = i;
						value_changed = true;
						ImGui::ClearActiveID( );
					}
				}
			}

			if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
			{
				*k = 0;
				ImGui::ClearActiveID( );
			}
			else
			{
				*k = key;
			}
		}
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hovered)
		{
			change_mode_open_map->second = !change_mode_open_map->second;
		}
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && change_mode_open_map->second)
		{
			change_mode_open_map->second = false;
		}

		if (change_mode_open_map->second)
		{
			float theme_col[4] = { 47 / 255.f, 70 / 255.f, 154 / 255.f };
			ImVec4 theme = ImVec4(ImColor(int(theme_col[0] * 255), int(theme_col[1] * 255), int(theme_col[2] * 255), int(theme_col[3] * 255)));

			static int m_flags2 = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

			ImGui::SetNextWindowPos(ImVec2(stored_cursor_pos.x + 10 * cc_menu::get().dpi_scale, stored_cursor_pos.y + 10 * cc_menu::get().dpi_scale));
			ImGui::SetNextWindowSize(ImVec2(67, 82));

			ImGui::Begin("bind_settings", NULL, m_flags2);
			{
				if (ImGui::SubButton("On press", ImVec2(0, 0), 0, 1, mode_map->second))
				{
					mode_map->second = 1;
				}
				if (ImGui::SubButton("Toggle", ImVec2(0, 0), 0, 2, mode_map->second))
				{
					mode_map->second = 2;
				}
				if (ImGui::SubButton("Always", ImVec2(0, 0), 0, 3, mode_map->second))
				{
					mode_map->second = 3;
				}

				if (!ImGui::IsWindowFocused( ))
				{
					change_mode_open_map->second = false;
				}
			}
			ImGui::End( );
		}

		// Render
		// Select which buffer we are going to display. When ImGuiInputTextFlags_NoLiveEdit is Set 'buf' might still be the old value. We Set buf to NULL to prevent accidental usage from now on.

		char buf_display[64] = "None";

		int auto_red = cc_menu::get( ).theme_col[0] * 255;
		int auto_green = cc_menu::get( ).theme_col[1] * 255;
		int auto_blue = cc_menu::get( ).theme_col[2] * 255;

		float theme_col[4] = { 47 / 255.f, 70 / 255.f, 154 / 255.f };
		ImVec4 theme = ImVec4(ImColor(int(theme_col[0] * 255), int(theme_col[1] * 255), int(theme_col[2] * 255), int(theme_col[3] * 255)));

		static std::map <ImGuiID, int> pValue;
		auto ItPLibrary = pValue.find(ID);

		if (ItPLibrary == pValue.end( ))
		{
			pValue.insert({ ID, 0 });
			ItPLibrary = pValue.find(ID);
		}

		static std::map <ImGuiID, int> pValue2;
		auto ItPLibrary2 = pValue2.find(ID);

		if (ItPLibrary2 == pValue2.end( ))
		{
			pValue2.insert({ ID, 0 });
			ItPLibrary2 = pValue2.find(ID);
		}

		int alpha_active = ImGui::Animate(label, "act", true, ItPLibrary2->second, 0.05, ImGui::INTERP);

		if (g.ActiveId == id || value_changed)
		{
			ItPLibrary->second = true;
		}
		if (ItPLibrary->second)
		{
			ItPLibrary2->second = 255;
			if (alpha_active > 200)
			{
				ItPLibrary->second = false;
			}
		}
		else
		{
			ItPLibrary2->second = 0;
		}

		//background
		window->DrawList->AddRectFilled(frame_bb.Min + ImVec2(1, 1), frame_bb.Max - ImVec2(1, 1), ImColor(auto_red, auto_green, auto_blue, 20 + alpha_active), style.FrameRounding);
		//outline

		window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, ImColor(auto_red, auto_green, auto_blue, 150), style.FrameRounding);

		if (*k != 0 && g.ActiveId != id)
		{
			strcpy(buf_display, KeyNames[*k]);
		}
		else if (g.ActiveId == id)
		{
			strcpy(buf_display, "None");
		}

		const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size
		ImVec2 render_pos = frame_bb.Min + style.FramePadding;

		float add_pos = ImGui::Animate(label, "add_pos", g.ActiveId == id, 18, 0.05, ImGui::DYNAMIC);
		ImGui::PushClipRect(frame_bb.Min, frame_bb.Max, false);
		ImGui::RenderTextClipped(frame_bb.Min + style.FramePadding + ImVec2(add_pos, 0), frame_bb.Max - style.FramePadding + ImVec2(add_pos, 0), buf_display, NULL, NULL);
		ImGui::PopClipRect( );
		//draw_window->DrawList->AddText(g.Font, g.FontSize, render_pos, GetColorU32(ImGuiCol_Text), buf_display, NULL, 0.0f, &clip_rect);

		if (mode_map->second == 1)
		{
			if (GetAsyncKeyState(*k))
			{
				*controlled_value = true;

				return true;
			}
		}
		else
			if (mode_map->second == 2)
			{
				if (GetKeyState(*k) & 1)
				{
					*controlled_value = true;

					return true;
				}
			}
			else
				if (mode_map->second == 3)
				{
					*controlled_value = true;
					return true;
				}

		if (*k != 0)
			*controlled_value = false;

		return false;
	}

	bool InputText(const char* label, char* buf, size_t buf_size)
	{
		ImGui::SetCursorPosX(gap * cc_menu::get().dpi_scale);
		ImGui::InputText(label, buf, buf_size);
		return true;
	}

	void ImRotateStart(int* rotation_start_index)
	{
		*rotation_start_index = ImGui::GetBackgroundDrawList( )->VtxBuffer.Size;
	}

	float easeInOutBack(float x)
	{
		const float c1 = 1.70158;
		const float c2 = c1 * 1.525;

		return x < 0.5
			? (pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
			: (pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
	}

	ImVec2 ImRotationCenter(int rotation_start_index)
	{
		ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

		const auto& buf = ImGui::GetBackgroundDrawList( )->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

		return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2); // or use _ClipRectStack?
	}

	void ImRotateEnd(float rad, int rotation_start_index)
	{
		ImVec2 center = ImRotationCenter(rotation_start_index);
		float s = sin(rad), c = cos(rad);
		center = ImRotate(center, s, c) - center;

		auto& buf = ImGui::GetBackgroundDrawList( )->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
	}
	void DrawCrosshair(ImDrawList* m_background_drawlist)
	{
		if (Horres == 0)
		{
			hDCScreen = GetDC(NULL);
			Horres = GetDeviceCaps(hDCScreen, HORZRES);
			Vertres = GetDeviceCaps(hDCScreen, VERTRES);
		}

		int c_x = Horres / 2;
		int c_y = Vertres / 2;

		ImColor theme_color = ImColor(int(theme_col[0] * 255), int(theme_col[1] * 255), int(theme_col[2] * 255), int(theme_col[3] * 255));

		m_background_drawlist->AddTriangle(ImVec2(c_x - 5, c_y + 5), ImVec2(c_x + 5, c_y + 5), ImVec2(c_x, c_y), ImColor(255, 255, 255));
		m_background_drawlist->AddTriangle(ImVec2(c_x - 8, c_y + 8), ImVec2(c_x + 8, c_y + 8), ImVec2(c_x, c_y + 3), theme_color);
	}

	void Configure(ImDrawList* m_background_drawlist, ImVec2 m_menu_pos, ImVec2 m_menu_size, bool main = true)
	{
		m_background_drawlist->AddRectFilled(m_menu_pos, m_menu_pos + m_menu_size, ImColor(5, 5, 5), 0);

		m_background_drawlist->AddRectFilled(m_menu_pos, m_menu_pos + ImVec2(m_menu_size.x, 70), ImColor(0, 0, 0), 0);

		m_background_drawlist->AddRectFilledMultiColor(m_menu_pos, m_menu_pos + ImVec2(m_menu_size.x, 100), ImColor(25, 25, 25, 150), ImColor(25, 25, 25, 150), ImColor(25, 25, 25, 0), ImColor(25, 25, 25, 0));

		if (main)
		{
			//flying pigs
			{
				for (int i = 0; i < 10; i++)
				{
					static std::unordered_map <int, ImVec2> pValue;
					auto ItPLibrary = pValue.find(i);
					if (ItPLibrary == pValue.end( ))
					{
						pValue.insert({ i, ImVec2(get_random_number(0, m_menu_size.x), get_random_number(0, 80))});
						ItPLibrary = pValue.find(i);
					}

					static std::unordered_map <int, int> pValue2;
					auto Rotation = pValue2.find(i);
					if (Rotation == pValue2.end( ))
					{
						pValue2.insert({ i, get_random_number(0, 360)});
						Rotation = pValue2.find(i);
					}

					static std::unordered_map <int, float> pValue3;
					auto Rotation_value = pValue3.find(i);
					if (Rotation_value == pValue3.end( ))
					{
						pValue3.insert({ i, get_random_number(-3, 3) });
						Rotation_value = pValue3.find(i);
					}

					static std::unordered_map <int, ImVec2> pValue4;
					auto Move_speed = pValue4.find(i);
					if (Move_speed == pValue4.end( ))
					{
						pValue4.insert({ i, ImVec2(get_random_number(-3, 3), get_random_number(-3, 3)) });
						Move_speed = pValue4.find(i);
					}

					ItPLibrary->second.x += Move_speed->second.x / 200;
					ItPLibrary->second.y += Move_speed->second.y / 200;

					Rotation_value->second += 0.000002 / ImGui::GetIO( ).DeltaTime;

					if (ItPLibrary->second.x + m_menu_pos.x > m_menu_pos.x + m_menu_size.y)
					{
						Move_speed->second.x *= -1;
					}

					if (ItPLibrary->second.y + m_menu_pos.y > m_menu_pos.y + 70)
					{
						Move_speed->second.y *= -1;
					}

					if (ItPLibrary->second.x + m_menu_pos.x < m_menu_pos.x)
					{
						Move_speed->second.x *= -1;
					}

					if (ItPLibrary->second.y + m_menu_pos.y < m_menu_pos.y)
					{
						Move_speed->second.y *= -1;
					}


					m_background_drawlist->PushClipRect(ImVec2(m_menu_pos), ImVec2(m_menu_pos + ImVec2(m_menu_size.x, 70)));
					ImRotateStart(&Rotation->second);
					m_background_drawlist->AddImage(cc_menu::get( ).pig, m_menu_pos + ItPLibrary->second, m_menu_pos + ItPLibrary->second + ImVec2(35, 35), ImVec2(0, 0), ImVec2(1, 1), ImColor(125, 143, 212, 30));
					ImRotateEnd(Rotation_value->second, Rotation->second);
					m_background_drawlist->PopClipRect( );	
				}
			}

			m_background_drawlist->AddText(cc_menu::get( ).giant_font, 100, m_menu_pos + ImVec2(100, 1), ImColor(50, 74, 168, 200), "Industries");
		}

		m_background_drawlist->AddText(cc_menu::get( ).small_font, 12, m_menu_pos + ImVec2(4, 3), ImColor(255, 255, 255, 100), main ? "Oink.industries | beta | v1.01 | User" : "Player list");

		m_background_drawlist->AddRectFilled(m_menu_pos + ImVec2(0, 70), m_menu_pos + m_menu_size, ImColor(10, 10, 10), 0);

		m_background_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 70), m_menu_pos + ImVec2(m_menu_size.x, 71), ImColor(50, 74, 168), ImColor(50, 74, 168, 0), ImColor(50, 74, 168, 0), ImColor(50, 74, 168));
		m_background_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 0), m_menu_pos + ImVec2(m_menu_size.x, 71), ImColor(50, 74, 168, 20), ImColor(50, 74, 168, 20), ImColor(50, 74, 168, 0), ImColor(50, 74, 168, 0));
		m_background_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 70), m_menu_pos + ImVec2(m_menu_size.x, m_menu_size.y), ImColor(50, 74, 168, 25), ImColor(50, 74, 168, 20), ImColor(50, 74, 168, 0), ImColor(50, 74, 168, 0));
		m_background_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 96), m_menu_pos + ImVec2(m_menu_size.x, m_menu_size.y), ImColor(9, 10, 15, 100), ImColor(9, 10, 15, 100), ImColor(21, 25, 38, 0), ImColor(21, 25, 38, 0));
		m_background_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 96), m_menu_pos + ImVec2(m_menu_size.x, m_menu_size.y / 2), ImColor(0, 0, 0, 100), ImColor(0, 0, 0, 100), ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0));

		m_background_drawlist->AddRectFilled(m_menu_pos + ImVec2(0, 95), m_menu_pos + ImVec2(m_menu_size.x, 96), ImColor(51, 53, 61, 150), 0);

		m_background_drawlist->AddRect(m_menu_pos, m_menu_pos + m_menu_size, ImColor(50, 74, 168, 50), 0);
	}
}

std::string current_help_tip = "";
void draw_cursor( )
{
	//get general drawlist
	ImDrawList* m_overlay_drawlist = ImGui::GetForegroundDrawList( );

	ShowCursor(false);
	POINT p;
	GetCursorPos(&p);

	m_overlay_drawlist->AddTriangleFilled(ImVec2(p.x - 2, p.y), ImVec2(p.x - 3, p.y + 10), ImVec2(p.x + 5, p.y + 7), ImColor(0, 0, 0));
	m_overlay_drawlist->AddTriangle(ImVec2(p.x - 2, p.y), ImVec2(p.x - 3, p.y + 10), ImVec2(p.x + 5, p.y + 7), ImColor(255, 255, 255));
}
void cc_menu::init_fonts( )
{
	ImGuiIO& io = ImGui::GetIO( ); (void) io;
	io.Fonts->Clear( );
	cc_menu::get( ).giant_font   = io.Fonts->AddFontFromMemoryTTF(museo1, sizeof(museo1), 100.0f * cc_menu::get( ).dpi_scale, NULL, io.Fonts->GetGlyphRangesCyrillic( ));
	cc_menu::get( ).default_font = io.Fonts->AddFontFromMemoryTTF(museo2, sizeof(museo2), 50.0f * cc_menu::get( ).dpi_scale, NULL, io.Fonts->GetGlyphRangesCyrillic( ));
	cc_menu::get( ).middle_font  = io.Fonts->AddFontFromMemoryTTF(museo3, sizeof(museo3), 13.0f * cc_menu::get( ).dpi_scale, NULL, io.Fonts->GetGlyphRangesCyrillic( ));
	cc_menu::get( ).small_font   = io.Fonts->AddFontFromMemoryTTF(museo3, sizeof(museo3), 12.0f * cc_menu::get( ).dpi_scale, NULL, io.Fonts->GetGlyphRangesCyrillic( ));
	io.Fonts->Build( );
	ImGui_ImplDX9_CreateDeviceObjects( );
}
void cc_menu::draw_menu( )
{
	draw_cursor( );
	//get general drawlist
	ImDrawList* m_overlay_drawlist = ImGui::GetForegroundDrawList( );
	ImDrawList* m_background_drawlist = ImGui::GetBackgroundDrawList( );
	ImDrawList* m_window_drawlist = nullptr;

	ImColor theme_color = ImColor(int(theme_col[0] * 255), int(theme_col[1] * 255), int(theme_col[2] * 255), int(theme_col[3] * 255));
	ImColor theme_color_no_alpha = ImColor(int(theme_col[0] * 255), int(theme_col[1] * 255), int(theme_col[2] * 255), 0);

	int theme_r = int(theme_col[0] * 255);
	int theme_g = int(theme_col[1] * 255);
	int theme_b = int(theme_col[2] * 255);
	int theme_a = int(theme_col[3] * 255);

	//toogle menu var
	static bool g_prepared = false;

	if (!g_prepared)
	{
		g_menu_opened = false;
		ImGui::SetNextWindowPos(ImVec2(100, 100));

		g_prepared = true;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Insert))
	{
		g_menu_opened = !g_menu_opened;
	}

	m_background_drawlist->AddRectFilled(ImVec2(0, 0), ImVec2(3000, 3000), ImColor(0, 0, 0), 0.0f);

	UGui::DrawCrosshair(m_background_drawlist);

	int backgrnd = ImGui::Animate("menu", "bckrg", g_menu_opened, 13, 0.15, ImGui::animation_types::STATIC);

	//window vars
	ImVec2 m_menu_pos, m_menu_size;

	//window flags
	static int m_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

	

	if (!g_menu_opened)
		return;

	ImGui::SetNextWindowSize(ImVec2(658 * dpi_scale, 510 * dpi_scale));

	//menu code
	if (ImGui::Begin("main window", &g_menu_opened, m_flags))
	{
		first_open = false;

		ImGuiStyle& style = ImGui::GetStyle( );

		//load vars
		m_menu_pos = ImGui::GetWindowPos( );
		m_menu_size = ImGui::GetWindowSize( );
		m_window_drawlist = ImGui::GetWindowDrawList( );

		//setup style
		ImGuiStyle& m_style = ImGui::GetStyle( );

		m_style.WindowBorderSize = 0.0f;
		m_style.ChildBorderSize = 0.0f;
		m_style.FrameBorderSize = 0.0f;
		m_style.PopupBorderSize = 0.0f;

		m_style.Colors[ImGuiCol_ScrollbarBg] = ImColor(0, 0, 0, 0);
		m_style.Colors[ImGuiCol_Border] = ImColor(0, 0, 0, 0);
		m_style.Colors[ImGuiCol_Button] = ImColor(14, 16, 25);
		m_style.Colors[ImGuiCol_PopupBg] = ImColor(14, 16, 25);

		m_style.ScrollbarRounding = 0;
		m_style.ScrollbarSize = 3.f;
		m_style.ItemSpacing = ImVec2(10, 10);
		style.ItemInnerSpacing.x = 10;
		style.ItemInnerSpacing.y = 10;

		UGui::Configure(m_background_drawlist, m_menu_pos, m_menu_size);

		ImGui::PushFont(middle_font);
		{
			static int tab = 1;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			{
				ImGui::SetCursorPos(ImVec2(0, 71));

				const float button_size_x = m_menu_size.x / 6;

				if (ImGui::TabButton("Aimbot",
					ImVec2(button_size_x, 25 * dpi_scale), 0, tab, 1))
					tab = 1;

				ImGui::SameLine( );

				if (ImGui::TabButton("Anti-aim",
					ImVec2(button_size_x, 25 * dpi_scale), 0, tab, 2))
					tab = 2;

				ImGui::SameLine( );

				if (ImGui::TabButton("Visuals",
					ImVec2(button_size_x, 25 * dpi_scale), 0, tab, 3))
					tab = 3;

				ImGui::SameLine( );

				if (ImGui::TabButton("ESP",
					ImVec2(button_size_x, 25 * dpi_scale), 0, tab, 4))
					tab = 4;

				ImGui::SameLine( );

				if (ImGui::TabButton("Misc",
					ImVec2(button_size_x, 25 * dpi_scale), 0, tab, 5))
					tab = 5;

				ImGui::SameLine( );

				if (ImGui::TabButton("Configs",
					ImVec2(button_size_x, 25 * dpi_scale), 0, tab, 6))
					tab = 6;

				int real_selected_tab_pos_x = button_size_x * (tab - 1);
				int animated_selected_tab_pos_x = ImGui::Animate("menu", "animated_selected_tab_pos_x", true, real_selected_tab_pos_x, 0.06, ImGui::animation_types::INTERP);

				m_background_drawlist->AddRectFilled(m_menu_pos + ImVec2(animated_selected_tab_pos_x, 95 * dpi_scale), m_menu_pos + ImVec2(animated_selected_tab_pos_x + button_size_x, 96 * dpi_scale), ImColor(50, 74, 168), 0);
				m_background_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(animated_selected_tab_pos_x, 70 * dpi_scale), m_menu_pos + ImVec2(animated_selected_tab_pos_x + button_size_x, 96 * dpi_scale), ImColor(51, 53, 61, 50), ImColor(51, 53, 61, 50), ImColor(51, 53, 61, 0), ImColor(51, 53, 61, 0));
			}
			ImGui::PopStyleVar( );

			switch (tab)
			{
				case 1:
				{
					UGui::BeginChild("Aimbot settings", 1);
					{
						static bool enable_aimbot;
						UGui::Checkbox("Enable", &enable_aimbot);

						if (enable_aimbot)
						{
							static int aim_fov;
							UGui::SliderInt("Aimbot fov", &aim_fov, 0, 180);
						}

						static bool enable_ignore_team;
						UGui::Checkbox("Ignore team", &enable_ignore_team);

						static bool hitscan[10];
						const char* text2[ ]{ "Head", "Neck", "Body", "Hands", "Pelvis", "Legs", "Foots" };
						UGui::MultiBox("Hitscan", hitscan, text2, ARRAYSIZE(text2));

						static int prior_hitbox;
						const char* text[ ]{ "Head", "Neck", "Body", "Hands", "Pelvis", "Legs", "Foots" };
						UGui::ComboBox("Priority hitbox", &prior_hitbox, text, ARRAYSIZE(text));

						static bool enable_autofire;
						UGui::Checkbox("Autofire", &enable_autofire);

						static bool enable_ignore_walls;
						UGui::Checkbox("Ignore walls", &enable_ignore_walls);

						static bool enable_autostop;
						UGui::Checkbox("Autostop", &enable_autostop);

						static bool enable_autoscope;
						UGui::Checkbox("Autoscope", &enable_autoscope);

						static bool enable_hitchance;
						UGui::Checkbox("Hitchance", &enable_hitchance);

						if (enable_hitchance)
						{
							static int hitchance;
							UGui::SliderInt("Hitchance value", &hitchance, 0, 100);
						}

						static bool enable_minimal_damage;
						UGui::Checkbox("Minimal damage", &enable_minimal_damage);

						if (enable_minimal_damage)
						{
							static int mindamage;
							UGui::SliderInt("Min.damage value", &mindamage, 0, 150);
						}
					}
					UGui::EndChild( );

					UGui::BeginChild("Weapon configuration", 2);
					{
						static float col[4];
						UGui::ColorPicker("Test colorpicker", col);

						UGui::Button("Save", ImVec2(186, 30));

						UGui::Button("Load", ImVec2(186, 30));

						static bool plesp;
						UGui::Checkbox("Player ESP", &plesp);

						static float col3[4];
						static float col2[4];

						UGui::ColorPickerButton("Visible", col2);
						UGui::ColorPickerButton("Invisible", col3, true);

						static int key;
						static bool dt;
						UGui::Checkbox("Double tap", &dt);
						UGui::Hotkey("Double tap key", &key, &dt);

						static char config_name[32];
						UGui::InputText("Config name", config_name, sizeof(config_name));
						{
							std::stringstream a;
							a << ImGui::GetIO( ).Framerate;
							UGui::Button(a.str( ).c_str( ), ImVec2(100, 25));
						}

						UGui::SliderFloat("GUI scale", &dpi_scale_slider, 0.5, 1.5, "%.2f");
					}
					UGui::EndChild( );

					UGui::BeginChild("Extra settings", 3);
					{

					}
					UGui::EndChild( );
				}
				break;

				case 2:
				{
					UGui::BeginChild("Aimbot settings", 1);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Weapon configuration", 2);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Extra settings", 3);
					{
					}
					UGui::EndChild( );
				}
				break;

				case 3:
				{
					UGui::BeginChild("Aimbot settings", 1);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Weapon configuration", 2);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Extra settings", 3);
					{
					}
					UGui::EndChild( );
				}
				break;

				case 4:
				{
					UGui::BeginChild("Aimbot settings", 1);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Weapon configuration", 2);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Extra settings", 3);
					{
					}
					UGui::EndChild( );
				}
				break;

				case 5:
				{
					UGui::BeginChild("Aimbot settings", 1);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Weapon configuration", 2);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Extra settings", 3);
					{
					}
					UGui::EndChild( );
				}
				break;

				case 6:
				{
					UGui::BeginChild("Aimbot settings", 1);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Weapon configuration", 2);
					{
					}
					UGui::EndChild( );

					UGui::BeginChild("Extra settings", 3);
					{
					}
					UGui::EndChild( );
				}
				break;
			}
		}
		ImGui::PopFont( );
	}
	ImGui::End( );
	//close
}
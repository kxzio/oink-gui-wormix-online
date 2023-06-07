#include "../ui.h"

using namespace ImGui;

bool c_oink_ui::hotkey(const char* label, keybind_t* keybind, const ImVec2& size_arg)
{
	same_line( );

	set_cursor_pos_x(140.f);
	//set_cursor_pos_y(get_cursor_pos_y( ) - 1.f);

	ImGuiWindow* window = GetCurrentWindow( );
	if (!window || window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	//size
	const ImGuiID id = window->GetID(label);

	ImVec2 size = size_arg;

	if (size.x <= 0.f)
		size.x = 58.f;
	if (size.y <= 0.f)
		size.y = 20.f;

	size *= m_dpi_scaling;

	//bb
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
	const ImRect total_bb(window->DC.CursorPos, frame_bb.Max);

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	auto& mode = keybind->m_activation_mode;
	auto& key = keybind->m_keycode;

	constexpr auto rmb_popup_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	bool rmb_popup_open = IsPopupOpen(id, rmb_popup_flags);

	bool hovered = false;

	if (!rmb_popup_open)
	{
		bool pressed_right, pressed_left;
		pressed_left = ButtonBehavior(total_bb, id, &hovered, nullptr, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_PressedOnRelease);

		if (hovered) // process only if hovered (logic)
		{
			if (!pressed_left)
			{
				pressed_right = ButtonBehavior(total_bb, id, nullptr, nullptr, ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_PressedOnRelease);

				if (pressed_right) // right opened
				{
					OpenPopupEx(id, rmb_popup_flags);
					rmb_popup_open = true;
				}
			}
			else // left opened
			{
				if (IsKeyDown(ImGuiKey_ModCtrl))
					key = keybind_t::keybind_unbound;
				else
					key = keybind_t::keybind_key_wait;
			};
		};
	};

	if (key == keybind_t::keybind_key_wait)
	{
		uint16_t i;

		for (i = 0; i < ImGuiMouseButton_COUNT; ++i)
		{
			if (IsMouseDown(static_cast<ImGuiMouseButton>(i)))
			{
				key = i;
				break;
			};
		};

		for (i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; ++i)
		{
			if (IsKeyDown(static_cast<ImGuiKey>(i)))
			{
				key = i;
				break;
			};
		};
	}
	else if (rmb_popup_open)
	{
		ImVec2 sizes = ImVec2(85.f * m_dpi_scaling, 82.f * m_dpi_scaling);
		ImVec2 pos = ImVec2(total_bb.Min.x + ((total_bb.Max.x - total_bb.Min.x - sizes.x) * 0.5f), total_bb.Max.y);

		SetNextWindowSize(sizes, ImGuiCond_Appearing);
		SetNextWindowPos(pos, ImGuiCond_Appearing);

		if (!IsPopupOpen(id, rmb_popup_flags))
			g.NextWindowData.ClearFlags( );
		else
		{
			if (begin("##rmb settings", 0, rmb_popup_flags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_Popup))
			{
				float avail = GetContentRegionAvail( ).x;

				if (button_ex("On Press", ImVec2(avail, 0.f)))
				{
					CloseCurrentPopup( );
					mode = keybind_mode_onpress;
				};

				if (button_ex("On Toogle", ImVec2(avail, 0.f)))
				{
					CloseCurrentPopup( );
					mode = keybind_mode_toggle;
				};

				if (button_ex("Always", ImVec2(avail, 0.f)))
				{
					CloseCurrentPopup( );
					mode = keybind_mode_always_on;
				};

				if (!IsWindowFocused( ))
					CloseCurrentPopup( );

				EndPopup( );
			};
		};
	}

	ImColor bg_new_colour = m_theme_colour_primary;

	const char* sz_display = "None";

	if (mode == keybind_mode_always_on)
		sz_display = "Always";
	else if (key != keybind_t::keybind_unbound)
	{
		if (key < ImGuiMouseButton_COUNT)
		{
			switch (key)
			{
				case ImGuiMouseButton_Left:
					sz_display = "LMB";
					break;
				case ImGuiMouseButton_Right:
					sz_display = "RMB";
					break;
				case ImGuiMouseButton_Middle:
					sz_display = "MMB";
					break;
				case 3:
					sz_display = "X1";
					break;
				case 4:
					sz_display = "X2";
					break;
				default:
					IM_ASSERT(0);
			};
		}
		else if (key == keybind_t::keybind_key_wait)
		{
			/*char buf[5];
			ZeroMemory(buf, sizeof(buf));

			uint8_t x = 1u + static_cast<uint8_t>(ImGui::GetTime( ) * 2.f) % (sizeof(buf) - 1u);

			FillMemory(buf, x, '.');*/

			sz_display = "...";
		}
		else
			sz_display = GetKeyName(key);
	};

	ImVec2 text_size = CalcTextSize(sz_display);

	const float frame_width = total_bb.GetWidth( );

	const bool is_hotkey_active = (key == keybind_t::keybind_key_wait || rmb_popup_open);

	float animation_bind_select = process_animation(label, 1, is_hotkey_active, 1.f, 13.f, e_animation_type::animation_dynamic);
	float animation_hovered = process_animation(label, 2, hovered, 0.22f, 15.f, animation_dynamic);

	float center_offset = (frame_width - text_size.x) * 0.5f; // center the text
	float text_slide = center_offset + animation_bind_select;

	if (is_hotkey_active)
		text_slide += ImSin(GetTime( ) * 4.f) * 8.f; // amp

	text_slide = ImClamp(text_slide, 0.f, frame_width - (text_size.x + style.FramePadding.x * 2.f));

	bg_new_colour.Value.w = 0.78f + animation_hovered;
	window->DrawList->AddRect(total_bb.Min, total_bb.Max, bg_new_colour, style.FrameRounding);

	if (animation_bind_select > 0.f)
	{
		bg_new_colour.Value.w = animation_bind_select;
		window->DrawList->AddRectFilled(total_bb.Min + ImVec2(1, 1), total_bb.Max - ImVec2(1, 1), bg_new_colour, style.FrameRounding);
	};

	PushClipRect(total_bb.Min, total_bb.Max, false);
	{
		RenderTextClipped(total_bb.Min + ImVec2(text_slide, style.FramePadding.y), total_bb.Max - ImVec2(text_slide, 0.f), sz_display, NULL, &text_size, ImVec2(0, 0), &total_bb);
	};
	PopClipRect( );

	return false;
}

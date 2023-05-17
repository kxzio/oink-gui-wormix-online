#include "../ui.h"

using namespace ImGui;

ImGuiID active_id_hotkey = 0;

bool c_oink_ui::hotkey(const char* label, keybind_t* keybind, const ImVec2& size_arg)
{
	same_line( );

	set_cursor_pos_x(140.f);
	set_cursor_pos_y(get_cursor_pos_y( ) - 1.f);

	ImGuiWindow* window = GetCurrentWindow( );
	if (!window || window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	//size
	const ImVec2 stored_cursor_pos = window->DC.CursorPos;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	ImVec2 size = ImVec2(58.f * m_dpi_scaling, 20.f * m_dpi_scaling);

	//bb
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
	const ImRect total_bb(window->DC.CursorPos, frame_bb.Max);

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	if (active_id_hotkey == 0)
	{
		bool hovered, pressed_right, pressed_left;
		pressed_left = ButtonBehavior(frame_bb, id, &hovered, nullptr, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_PressedOnRelease);

		if (hovered) // process only if hovered (logic)
		{
			if (!pressed_left)
			{
				pressed_right = ButtonBehavior(frame_bb, id, nullptr, nullptr, ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_PressedOnRelease);

				if (pressed_right) // right opened
					active_id_hotkey = id ^ 2;
			}
			else // left opened
			{
				active_id_hotkey = id;
			};
		};
	};

	const ImGuiID id_rmb = id ^ 2;

	if (active_id_hotkey == id)
	{
		for (uint16_t i = 0u; i < ImGuiKey_COUNT; ++i)
		{
			if (i < ImGuiMouseButton_COUNT)
			{
				if (ImGui::IsMouseDown(static_cast<ImGuiMouseButton>(i)))
				{
					keybind->m_keycode = i;

					ImGui::ClearActiveID( );
					active_id_hotkey = 0;
					break;
				};
			}
			else
			{
				if (ImGui::IsKeyDown(static_cast<ImGuiKey>(i)))
				{
					if (i != ImGuiKey_Escape)
						keybind->m_keycode = i;
					else if (i == ImGuiKey_Backspace) // unbind
						keybind->m_keycode = keybind_t::keybind_unbound;

					ImGui::ClearActiveID( );
					active_id_hotkey = 0;
					break;
				};
			};
		};
	};

	if (active_id_hotkey == id_rmb)
	{
		constexpr auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

		ImGui::SetNextWindowPos(ImVec2(stored_cursor_pos.x + 10.f * m_dpi_scaling, stored_cursor_pos.y + 10.f * m_dpi_scaling), ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(ImVec2(85.f * m_dpi_scaling, 82.f * m_dpi_scaling), ImGuiCond_Appearing);

		if (ImGui::Begin("bind_settings", NULL, flags))
		{
			float avail = ImGui::GetContentRegionAvail( ).x;

			if (sub_button("Toogle", ImVec2(avail, 0), 0, keybind_mode_toggle, keybind->m_activation_mode))
				keybind->m_activation_mode = keybind_mode_toggle;
			if (sub_button("On Press", ImVec2(avail, 0), 0, keybind_mode_onpress, keybind->m_activation_mode))
				keybind->m_activation_mode = keybind_mode_onpress;
			if (sub_button("Always", ImVec2(avail, 0), 0, keybind_mode_always_on, keybind->m_activation_mode))
				keybind->m_activation_mode = keybind_mode_always_on;

			if (!ImGui::IsWindowFocused( ))
			{
				active_id_hotkey = 0;
				ImGui::ClearActiveID( );
			}
		}
		ImGui::End( );
	}

	float animation_bind_select = g_ui.process_animation(label, 1, (active_id_hotkey == id || active_id_hotkey == id_rmb), 1.f, 13.f, e_animation_type::animation_dynamic);

	ImColor bg_new_colour = m_theme_colour_primary; bg_new_colour.Value.w = animation_bind_select;

	window->DrawList->AddRectFilled(frame_bb.Min + ImVec2(1, 1), frame_bb.Max - ImVec2(1, 1), bg_new_colour, style.FrameRounding);
	//outline

	bg_new_colour.Value.w = 150.f / 255.f;
	window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, bg_new_colour, style.FrameRounding);

	const char* sz_display = "None";

	if (keybind->m_activation_mode == keybind_mode_always_on)
		sz_display = "Always";
	else if (keybind->m_keycode != keybind_t::keybind_unbound)
	{
		if (keybind->m_keycode < ImGuiMouseButton_COUNT)
		{
			sz_display = "Left Mouse";

			switch (keybind->m_keycode)
			{
				case 0:
					break;
				case 1:
					sz_display = "Right Mouse";
					break;
				case 2:
					sz_display = "Middle Mouse";
					break;
				case 3:
					sz_display = "X1 Button";
					break;
				case 4:
					sz_display = "X2 Button";
					break;
				default:
					IM_ASSERT(0);
			};
		}
		else
			sz_display = ImGui::GetKeyName(keybind->m_keycode);
	};

	const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size
	ImVec2 render_pos = frame_bb.Min + style.FramePadding;

	ImGui::PushClipRect(frame_bb.Min, frame_bb.Max, false);

	ImVec2 test_size = ImGui::CalcTextSize(sz_display);

	float text_slide = animation_bind_select * ((frame_bb.Max.x - frame_bb.Min.x) * 0.5f - test_size.x); // center??

	ImGui::RenderTextClipped(frame_bb.Min + style.FramePadding + ImVec2(text_slide, 0.f), frame_bb.Max - style.FramePadding + ImVec2(text_slide, 0.f), sz_display, NULL, NULL);
	ImGui::PopClipRect( );

	// processing here is a bad idea
	/*if (keybind->m_activation_mode == 1)
	{
		if (ImGui::IsKeyDown(keybind->m_keycode) && ImGui::IsKeyPressed(keybind->m_keycode))
		{
			keybind->m_active = !keybind->m_active;
			return true;
		}
	}
	else
		if (keybind->m_activation_mode == 2)
		{
			if (ImGui::IsKeyDown(keybind->m_keycode))
			{
				keybind->m_active = true;
				return true;
			}
		}
		else
			if (keybind->m_activation_mode == 3)
			{
				keybind->m_active = true;
				return true;
			}

	if (keybind->m_keycode != 0 && keybind->m_activation_mode != 1)
		keybind->m_active = false;*/

	return false;
}

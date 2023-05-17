#include "../ui.h"

using namespace ImGui;

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

	bool hovered, pressed_right, pressed_left;

	pressed_left = ButtonBehavior(frame_bb, id, &hovered, nullptr, ImGuiButtonFlags_MouseButtonLeft);

	if (hovered) // process only if hovered (logic)
	{
		if (!pressed_left)
		{
			pressed_right = ButtonBehavior(frame_bb, id, nullptr, nullptr, ImGuiButtonFlags_MouseButtonRight);

			if (pressed_right) // right opened
			{

			};
		}
		else // left opened
		{

		};
	};

	bool value_changed = false;

	if (IsItemActive( ))
	{
		for (uint16_t i = 0u; i < ImGuiKey_COUNT; ++i)
		{
			if (i < ImGuiMouseButton_COUNT)
			{
				if (ImGui::IsMouseDown(static_cast<ImGuiMouseButton>(i)))
				{
					keybind->m_keycode = i;
					value_changed = true;
					ImGui::ClearActiveID( );
				};
			}
			else
			{
				if (ImGui::IsKeyDown(static_cast<ImGuiKey>(i)))
				{
					if (i != ImGuiKey_Escape) // skip binding
					{
						keybind->m_keycode = i;
						value_changed = true;
					}
					else if (i == ImGuiKey_Backspace) // unbind
					{
						keybind->m_keycode = UINT16_MAX;
						value_changed = true;
					}

					ImGui::ClearActiveID( );
				};
			};
		};
	};

	if (keybind->m_popup_open)
	{
		constexpr int m_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

		ImGui::SetNextWindowPos(ImVec2(stored_cursor_pos.x + 10 * m_dpi_scaling, stored_cursor_pos.y + 10 * m_dpi_scaling));
		ImGui::SetNextWindowSize(ImVec2(75 * m_dpi_scaling, 82 * m_dpi_scaling));


		ImGui::Begin("bind_settings", NULL, m_flags);
		{
			if (sub_button("Toogle", ImVec2(0, 0), 0, 1, keybind->m_activation_mode))
				keybind->m_activation_mode = keybind_mode_toggle;
			if (sub_button("On Press", ImVec2(0, 0), 0, 2, keybind->m_activation_mode))
				keybind->m_activation_mode = keybind_mode_onpress;
			if (sub_button("Always", ImVec2(0, 0), 0, 3, keybind->m_activation_mode))
				keybind->m_activation_mode = keybind_mode_always_on;

			if (!ImGui::IsWindowFocused( ))
				keybind->m_popup_open = false;
		}
		ImGui::End( );
	}

	char buf_display[64] = "None";

	float animation_bind_select = g_ui.process_animation(label, 2, g.ActiveId == id, 21, 13.f, e_animation_type::animation_dynamic);

	ImColor bg_new_colour = m_theme_colour_primary; bg_new_colour.Value.w = (20.f + (animation_bind_select / 21.f) * 200) / 255.f;

	window->DrawList->AddRectFilled(frame_bb.Min + ImVec2(1, 1), frame_bb.Max - ImVec2(1, 1), bg_new_colour, style.FrameRounding);
	//outline

	bg_new_colour.Value.w = 150.f / 255.f;
	window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, bg_new_colour, style.FrameRounding);

	if (keybind->m_keycode != 0 && g.ActiveId != id)
		strcpy_s(buf_display, m_key_names[keybind->m_keycode]);
	else if (g.ActiveId == id)
		strcpy_s(buf_display, "None");

	const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size
	ImVec2 render_pos = frame_bb.Min + style.FramePadding;

	ImGui::PushClipRect(frame_bb.Min, frame_bb.Max, false);
	ImGui::RenderTextClipped(frame_bb.Min + style.FramePadding + ImVec2(animation_bind_select * m_dpi_scaling, 0), frame_bb.Max - style.FramePadding + ImVec2(animation_bind_select, 0), buf_display, NULL, NULL);
	ImGui::PopClipRect( );
	\
		if (keybind->m_activation_mode == 1)
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
		keybind->m_active = false;

	return false;
}

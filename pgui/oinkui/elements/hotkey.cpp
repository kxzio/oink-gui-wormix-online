#include "../ui.h"

using namespace ImGui;

bool c_oink_ui::hotkey(const char* label, s_keybind* keybind, const ImVec2& size_arg)
{
	ImGui::SameLine( );
	ImGui::SetCursorPosX(140 * m_dpi_scaling);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY( ) - 1 * m_dpi_scaling);

	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	if (!window || window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	//size
	const ImVec2 stored_cursor_pos = window->DC.CursorPos;
	const ImGuiID id			   = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	ImVec2 size = ImVec2(58 * m_dpi_scaling, 20 * m_dpi_scaling);

	//bb
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
	const ImRect total_bb(window->DC.CursorPos, frame_bb.Max);

	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id))
		return false;

	const bool hovered = ImGui::ItemHoverable(frame_bb, id);

	if (hovered)
	{
		ImGui::SetHoveredID(id);
	}

	const bool user_clicked = hovered && io.MouseClicked[0];

	if (user_clicked)
	{
		if (g.ActiveId != id)
		{
			memset(io.MouseDown, 0, sizeof(io.MouseDown));
			memset(io.KeysDown, 0, sizeof(io.KeysDown));
			keybind->m_keycode = 0;
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

	if (g.ActiveId == id)
	{
		for (auto i = 0; i < 5; i++)
		{
			if (io.MouseDown[i])
			{
				switch (i)
				{
					case 0:
						keybind->m_keycode = VK_LBUTTON;
						break;
					case 1:
						keybind->m_keycode = VK_RBUTTON;
						break;
					case 2:
						keybind->m_keycode = VK_MBUTTON;
						break;
					case 3:
						keybind->m_keycode = VK_XBUTTON1;
						break;
					case 4:
						keybind->m_keycode = VK_XBUTTON2;
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
					keybind->m_keycode = i;
					value_changed = true;
					ImGui::ClearActiveID( );
				}
			}
		}

		if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
		{
			keybind->m_keycode = 0;
			ImGui::ClearActiveID( );
		}
	}
	else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hovered)
	{
		keybind->m_popup_open = !keybind->m_popup_open;
	}
	else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && keybind->m_popup_open)
	{
		keybind->m_popup_open = false;
	}

	if (keybind->m_popup_open)
	{
		constexpr int m_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar ;

		ImGui::SetNextWindowPos(ImVec2(stored_cursor_pos.x + 10 * m_dpi_scaling, stored_cursor_pos.y + 10 * m_dpi_scaling));
		ImGui::SetNextWindowSize(ImVec2(75 * m_dpi_scaling, 82 * m_dpi_scaling));


		ImGui::Begin("bind_settings", NULL, m_flags);
		{
			if (sub_button("Toogle", ImVec2(0, 0), 0, 1, keybind->m_activation_mode))
				keybind->m_activation_mode = 1;
			if (sub_button("On Press", ImVec2(0, 0), 0, 2, keybind->m_activation_mode))
				keybind->m_activation_mode = 2;
			if (sub_button("Always", ImVec2(0, 0), 0, 3, keybind->m_activation_mode))
				keybind->m_activation_mode = 3;

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

#include "../ui.h"

bool c_oink_ui::hotkey(const char* label, int* k, bool* controlled_value)
{
	static std::map <ImGuiID, bool> changemode_open;
	static std::map <ImGuiID, int> mode;
	static std::map <ImGuiID, int> pValue;
	static std::map <ImGuiID, int> pValue2;

	ImGui::SameLine( );
	ImGui::SetCursorPosX(140 * m_dpi_scaling);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY( ) - 2 * m_dpi_scaling);

	ImGuiWindow* window = ImGui::GetCurrentWindow( );
	if (!window || window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	const ImVec2 stored_cursor_pos = window->DC.CursorPos;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	ImVec2 size = ImVec2(56 * g_ui.m_dpi_scaling, 20 * g_ui.m_dpi_scaling);
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

	auto change_mode_open_map = changemode_open.find(ID);

	if (change_mode_open_map == changemode_open.end( ))
		change_mode_open_map = changemode_open.insert({ ID, false }).first;

	auto mode_map = mode.find(ID);

	if (mode_map == mode.end( ))
		mode_map = mode.insert({ ID, 1 }).first;

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
		constexpr int m_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

		ImGui::SetNextWindowPos(ImVec2(stored_cursor_pos.x + 10 * m_dpi_scaling, stored_cursor_pos.y + 10 * m_dpi_scaling));
		ImGui::SetNextWindowSize(ImVec2(67 * g_ui.m_dpi_scaling, 82 * g_ui.m_dpi_scaling));

		ImGui::Begin("bind_settings", NULL, m_flags);
		{
			if (sub_button("On press", ImVec2(0, 0), 0, 1, mode_map->second))
				mode_map->second = 1;
			if (sub_button("Toggle", ImVec2(0, 0), 0, 2, mode_map->second))
				mode_map->second = 2;
			if (sub_button("Always", ImVec2(0, 0), 0, 3, mode_map->second))
				mode_map->second = 3;

			if (!ImGui::IsWindowFocused( ))
				change_mode_open_map->second = false;
		}
		ImGui::End( );
	}

	// Render
	// Select which buffer we are going to display. When ImGuiInputTextFlags_NoLiveEdit is Set 'buf' might still be the old value. We Set buf to NULL to prevent accidental usage from now on.

	char buf_display[64] = "None";

	auto ItPLibrary = pValue.find(ID);

	if (ItPLibrary == pValue.end( ))
		ItPLibrary = pValue.insert({ ID, 0 }).first;

	auto ItPLibrary2 = pValue2.find(ID);

	if (ItPLibrary2 == pValue2.end( ))
	{
		pValue2.insert({ ID, 0 });
		ItPLibrary2 = pValue2.find(ID);
	}

	int alpha_active = g_ui.process_animation(label, 2, true, ItPLibrary2->second, 10.f, e_animation_type::animation_interp);

	if (g.ActiveId == id || value_changed)
		ItPLibrary->second = true;
	if (ItPLibrary->second)
	{
		ItPLibrary2->second = 255;
		if (alpha_active > 200)
			ItPLibrary->second = false;
	}
	else
		ItPLibrary2->second = 0;

	//background
	ImColor bg_new_colour = m_theme_colour;
	bg_new_colour.Value.w = (20.f + alpha_active) / 255.f;

	window->DrawList->AddRectFilled(frame_bb.Min + ImVec2(1, 1), frame_bb.Max - ImVec2(1, 1), bg_new_colour, style.FrameRounding);
	//outline

	bg_new_colour.Value.w = 150.f / 255.f;
	window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, bg_new_colour, style.FrameRounding);

	if (*k != 0 && g.ActiveId != id)
		strcpy_s(buf_display, m_key_names[*k]);
	else if (g.ActiveId == id)
		strcpy_s(buf_display, "None");

	const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size
	ImVec2 render_pos = frame_bb.Min + style.FramePadding;

	float add_pos = g_ui.process_animation(label, 3, g.ActiveId == id, 18, 10.f, e_animation_type::animation_dynamic);
	ImGui::PushClipRect(frame_bb.Min, frame_bb.Max, false);
	ImGui::RenderTextClipped(frame_bb.Min + style.FramePadding + ImVec2(add_pos, 0), frame_bb.Max - style.FramePadding + ImVec2(add_pos, 0), buf_display, NULL, NULL);
	ImGui::PopClipRect( );
	//draw_window->DrawList->AddText(g.Font, g.FontSize, render_pos, GetColorU32(ImGuiCol_Text), buf_display, NULL, 0.0f, &clip_rect);

	if (mode_map->second == 1)
	{
		if (ImGui::IsKeyDown(*k))
		{
			*controlled_value = true;
			return true;
		}
	}
	else
		if (mode_map->second == 2)
		{
			if (ImGui::IsKeyPressed(*k) && ImGui::IsKeyDown(*k))
			{
				*controlled_value = !*controlled_value;
				return *controlled_value;
			}
		}
		else
			if (mode_map->second == 3)
			{
				*controlled_value = true;
				return true;
			}

	if (*k != 0 && mode_map->second != 2)
		*controlled_value = false;

	return false;
}

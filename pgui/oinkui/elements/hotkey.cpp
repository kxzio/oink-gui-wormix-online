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

	auto& mode = keybind->m_activation_mode;
	auto& key = keybind->m_keycode;

	constexpr auto rmb_popup_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	bool rmb_popup_open = ImGui::IsPopupOpen(id, rmb_popup_flags);

	if (!rmb_popup_open)
	{
		bool hovered, pressed_right, pressed_left;
		pressed_left = ButtonBehavior(frame_bb, id, &hovered, nullptr, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_PressedOnRelease);

		if (hovered) // process only if hovered (logic)
		{
			if (!pressed_left)
			{
				pressed_right = ButtonBehavior(frame_bb, id, nullptr, nullptr, ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_PressedOnRelease);

				if (pressed_right) // right opened
				{
					ImGui::OpenPopupEx(id, rmb_popup_flags);
					rmb_popup_open = true;
				}
			}
			else // left opened
			{
				if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))
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
			if (ImGui::IsMouseDown(static_cast<ImGuiMouseButton>(i)))
			{
				key = i;
				break;
			};
		};

		for (i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; ++i)
		{
			if (ImGui::IsKeyDown(static_cast<ImGuiKey>(i)))
			{
				if (i != ImGuiKey_Escape)
					key = i;

				break;
			};
		};
	}
	else if (rmb_popup_open)
	{
		ImVec2 sizes = ImVec2(85.f * m_dpi_scaling, 82.f * m_dpi_scaling);
		ImVec2 pos = ImVec2(frame_bb.Min.x + ((frame_bb.Max.x - frame_bb.Min.x - sizes.x) * 0.5f), frame_bb.Max.y);

		ImGui::SetNextWindowSize(sizes, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(pos, ImGuiCond_Appearing);

		if (ImGui::BeginPopupEx(id, rmb_popup_flags))
		{
			float avail = ImGui::GetContentRegionAvail( ).x;

			if (button_ex("On Press", ImVec2(avail, 0.f)))
			{
				ImGui::CloseCurrentPopup( );
				mode = keybind_mode_onpress;
			}

			if (button_ex("On Toogle", ImVec2(avail, 0.f)))
			{
				ImGui::CloseCurrentPopup( );
				mode = keybind_mode_toggle;
			}

			if (button_ex("Always", ImVec2(avail, 0.f)))
			{
				ImGui::CloseCurrentPopup( );
				mode = keybind_mode_always_on;
			}

			ImGui::EndPopup( );
		};

	}

	float animation_bind_select = g_ui.process_animation(label, 1, (key == keybind_t::keybind_key_wait || rmb_popup_open), 1.f, 13.f, e_animation_type::animation_dynamic);

	ImColor bg_new_colour = m_theme_colour_primary; bg_new_colour.Value.w = animation_bind_select;

	window->DrawList->AddRectFilled(frame_bb.Min + ImVec2(1, 1), frame_bb.Max - ImVec2(1, 1), bg_new_colour, style.FrameRounding);
	//outline

	bg_new_colour.Value.w = 0.58f;
	window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, bg_new_colour, style.FrameRounding);

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
					sz_display = "Left Mouse";
					break;
				case ImGuiMouseButton_Right:
					sz_display = "Right Mouse";
					break;
				case ImGuiMouseButton_Middle:
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
		else if (key == keybind_t::keybind_key_wait)
		{
			char buf[4];
			ZeroMemory(buf, sizeof(buf));

			for (uint8_t i = 0; i <= ((uint8_t) (ImGui::GetTime( ) * 2.f) & 3); ++i)
				buf[i] = '.';

			sz_display = buf;
		}
		else
			sz_display = ImGui::GetKeyName(key);
	};

	const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size
	ImVec2 render_pos = frame_bb.Min + style.FramePadding;

	ImGui::PushClipRect(frame_bb.Min, frame_bb.Max, false);

	ImVec2 test_size = ImGui::CalcTextSize(sz_display);

	float text_slide = animation_bind_select * (((frame_bb.Max.x - frame_bb.Min.x) * 0.5f) - test_size.x * 0.5f); // center??

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

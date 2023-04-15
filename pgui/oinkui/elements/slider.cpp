#include "../ui.h"

using namespace ImGui;

extern ImGuiInputTextFlags InputScalar_DefaultCharsFilter(ImGuiDataType data_type, const char* format);

bool c_oink_ui::temp_input_scalar(const ImRect& bb, ImGuiID id, const char* label, ImGuiDataType data_type, void* p_data, const char* format, const void* p_clamp_min, const void* p_clamp_max)
{
	char fmt_buf[32];
	char data_buf[32];
	format = ImParseFormatTrimDecorations(format, fmt_buf, IM_ARRAYSIZE(fmt_buf));
	DataTypeFormatString(data_buf, IM_ARRAYSIZE(data_buf), data_type, p_data, format);
	ImStrTrimBlanks(data_buf);

	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoMarkEdited;
	flags |= InputScalar_DefaultCharsFilter(data_type, format);

	bool value_changed = false;

	if (temp_input_text(bb, id, label, data_buf, IM_ARRAYSIZE(data_buf), flags))
	{
		// Backup old value
		size_t data_type_size = DataTypeGetInfo(data_type)->Size;
		ImGuiDataTypeTempStorage data_backup;
		memcpy(&data_backup, p_data, data_type_size);

		// Apply new value (or operations) then clamp
		DataTypeApplyFromText(data_buf, data_type, p_data, format);
		if (p_clamp_min || p_clamp_max)
		{
			if (p_clamp_min && p_clamp_max && DataTypeCompare(data_type, p_clamp_min, p_clamp_max) > 0)
				ImSwap(p_clamp_min, p_clamp_max);
			DataTypeClamp(data_type, p_data, p_clamp_min, p_clamp_max);
		}

		// Only mark as edited if new value is different
		value_changed = memcmp(&data_backup, p_data, data_type_size) != 0;
		if (value_changed)
			MarkItemEdited(id);
	}

	return value_changed;
}


bool c_oink_ui::slider_scalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, const ImColor& theme)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = 186 * m_dpi_scaling;

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, 9 * m_dpi_scaling));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 3.0f * m_dpi_scaling));

	const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
		return false;

	// Default format string when passing NULL
	if (format == NULL)
		format = DataTypeGetInfo(data_type)->PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
		format = PatchFormatStringFloatToInt(format);

	const ImRect input_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 10.0f * m_dpi_scaling));

	// Tabbing or CTRL-clicking on Slider turns it into an input box
	const bool hovered = ItemHoverable(frame_bb, id);
	bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
	if (!temp_input_is_active)
	{
		const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
		const bool clicked = (hovered && g.IO.MouseClicked[0]);
		if (input_requested_by_tabbing || clicked || g.NavActivateId == id || g.NavActivateInputId == id)
		{
			SetActiveID(id, window);
			SetFocusID(id, window);
			FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
			if (temp_input_allowed && (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || g.NavActivateInputId == id))
				temp_input_is_active = true;
		}
	}

	if (temp_input_is_active)
	{
		// Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
		const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
		return temp_input_scalar(input_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
	}

	// Draw frame
	const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	RenderNavHighlight(frame_bb, id);
	//RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

	ImColor color = theme;

	color.Value.w = 0.3f;

	// draw slider bg
	window->DrawList->AddRectFilledMultiColor(frame_bb.Min, frame_bb.Max, color, IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, color);

	color.Value.w = 1.f;

	// draw slider frame
	window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, color);

	// Slider behavior
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
	if (value_changed)
		MarkItemEdited(id);

	float slider_difference = grab_bb.Max.x - frame_bb.Min.x;

	float button_animation = g_ui.process_animation(label, 1, g.ActiveId == id, 0.28f, 10.0f, e_animation_type::animation_dynamic);
	float button_hovered_animation = g_ui.process_animation(label, 2, hovered, 1.f - 0.78f, 10.f, e_animation_type::animation_dynamic);

	float slider_animation = g_ui.process_animation(label, 3, true, slider_difference, 10.f, e_animation_type::animation_interp);

	color.Value.w = 0.78f + button_hovered_animation;

	// draw filled space
	window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(frame_bb.Min.x + slider_animation, grab_bb.Max.y) + ImVec2(2 * m_dpi_scaling, 2 * m_dpi_scaling), color);

	color.Value.w = button_animation;

	// draw other space
	window->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, color);

	//color.Value.w = 0.19f;

	//window->DrawList->AddRectFilledMultiColor(frame_bb.Min, frame_bb.Max, color, color_no_alpha, color_no_alpha, color);

	// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

	ImVec2 textSize = m_fonts[3]->CalcTextSizeA(12.f * m_dpi_scaling, FLT_MAX, -1, value_buf, value_buf_end, NULL);

	textSize.x = ImMax(12.f * m_dpi_scaling, textSize.x);

	float new_grab_bb_max = frame_bb.Min.x + slider_animation;
	float new_grab_bb_min = frame_bb.Min.x + slider_animation - 8 * m_dpi_scaling;

	ImVec2 p1 = ImVec2(new_grab_bb_max - textSize.x / 2 - 4 * m_dpi_scaling, grab_bb.Min.y - 8 * m_dpi_scaling) - ImVec2(8 * m_dpi_scaling, 0);
	ImVec2 p2 = ImVec2(new_grab_bb_max + textSize.x / 2 + 4 * m_dpi_scaling, grab_bb.Max.y + 8 * m_dpi_scaling) - ImVec2(8 * m_dpi_scaling, 0);

	PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
	{
		window->DrawList->AddRectFilled(p1, p2, IM_COL32_BLACK);

		color.Value.w = 1.f;
		window->DrawList->AddRect(p1, p2, color);

		RenderTextClipped(ImVec2(p1.x, grab_bb.Min.y - 5 * m_dpi_scaling),
				  ImVec2(p2.x, grab_bb.Max.y + 5 * m_dpi_scaling),
				  value_buf,
				  value_buf_end,
				  NULL,
				  ImVec2(0.5f, 0.5f));


	}
	PopStyleColor( );

	//if (label_size.x > 0.0f)
		//RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return value_changed;
}

bool c_oink_ui::slider_int(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	ImGui::SetCursorPosX(m_dpi_scaling * m_gap);
	return slider_scalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags, m_theme_colour);
}

bool c_oink_ui::slider_float(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
	ImGui::SetCursorPosX(m_dpi_scaling * m_gap);
	return slider_scalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags, m_theme_colour);
}
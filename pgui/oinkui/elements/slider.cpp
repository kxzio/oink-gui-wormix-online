#include "../../menu.h"

using namespace ImGui;

static const char* PatchFormatStringFloatToInt(const char* fmt)
{
	if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
		return "%d";
	const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
	const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
	if (fmt_end > fmt_start && fmt_end[-1] == 'f')
	{
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
		if (fmt_start == fmt && fmt_end[0] == 0)
			return "%d";
		ImGuiContext& g = *GImGui;
		ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int) (fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
		return g.TempBuffer;
#else
		IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please replace with e.g. "%d"
#endif
	}
	return fmt;
}

bool slider_scalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = 186;

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, 9));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 3.0f));

	const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
		return false;

	// Default format string when passing NULL
	if (format == NULL)
		format = DataTypeGetInfo(data_type)->PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0) // (FIXME-LEGACY: Patch old "%.0f" format string to use "%d", read function more details.)
		format = PatchFormatStringFloatToInt(format);

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
		return TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
	}

	// Draw frame
	const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	RenderNavHighlight(frame_bb, id);
	//RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

	window->DrawList->AddRectFilledMultiColor(frame_bb.Min, frame_bb.Max,
											  ImColor(47, 70, 154, 0), ImColor(47, 70, 154, 0), ImColor(47, 70, 154, 20), ImColor(47, 70, 154, 20));

	window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, ImColor(47, 70, 154, 255));

	// Slider behavior
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
	if (value_changed)
		MarkItemEdited(id);

	float hovered1 = Animate("button_hover", label, hovered || g.ActiveId == id, 200, 0.07, DYNAMIC);
	float alpha = Animate("button", label, true, 200, 0.05, DYNAMIC);
	float hovered_alpha = Animate("button_hovered", label, true, frame_bb.GetSize( ).y, 0.05, DYNAMIC);

	float diff = grab_bb.Max.x - frame_bb.Min.x;
	float anim_diff = Animate("slider_grab", label, true, diff, 0.07, INTERP);
	window->DrawList->AddRectFilledMultiColor(frame_bb.Min - ImVec2(1, 0), ImVec2(frame_bb.Min.x + anim_diff, grab_bb.Max.y) + ImVec2(2, 2),
											   ImColor(47, 70, 154, 25 + int(alpha) + int((hovered_alpha / frame_bb.GetSize( ).y) * 200)),
											   ImColor(47, 70, 154, 25 + int(alpha) + int((hovered_alpha / frame_bb.GetSize( ).y) * 200)),
											   ImColor(47, 70, 154, 25 + int(alpha) + int((hovered_alpha / frame_bb.GetSize( ).y) * 200)),
											   ImColor(47, 70, 154, 25 + int(alpha) + int((hovered_alpha / frame_bb.GetSize( ).y) * 200)));

	int auto_red = 47;
	int auto_green = 70;
	int auto_blue = 154;

	window->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max,
									ImColor(auto_red, auto_green, auto_blue, int(hovered1 / 5)));

	window->DrawList->AddRectFilledMultiColor(frame_bb.Min, frame_bb.Max,
											  ImColor(auto_red, auto_green, auto_blue, 50), ImColor(auto_red, auto_green, auto_blue, 0), ImColor(auto_red, auto_green, auto_blue, 0), ImColor(auto_red, auto_green, auto_blue, 50));

	//window->DrawList->AddRectFilled(frame_bb.Min, grab_bb.Max + ImVec2(0, 1), GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

	// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
	if (g.LogEnabled)
		LogSetNextTextDecoration("{", "}");

	float new_grab_bb_max = frame_bb.Min.x + anim_diff;
	PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(255, 255, 255, 255)));
	{
		window->DrawList->AddRectFilled(ImVec2(new_grab_bb_max - CalcTextSize(value_buf, value_buf_end).x / 2 - 4, grab_bb.Min.y - 8) - ImVec2(8, 0), ImVec2(new_grab_bb_max + CalcTextSize(value_buf, value_buf_end).x / 2 + 4, grab_bb.Max.y + 8) - ImVec2(8, 0), ImColor(0, 0, 0, 255));
		window->DrawList->AddRectFilledMultiColor(ImVec2(new_grab_bb_max - CalcTextSize(value_buf, value_buf_end).x / 2 - 4, grab_bb.Min.y - 8) - ImVec2(8, 0), ImVec2(new_grab_bb_max + CalcTextSize(value_buf, value_buf_end).x / 2 + 4, grab_bb.Max.y + 8) - ImVec2(8, 0),
												  ImColor(47, 70, 154, 0),
												  ImColor(47, 70, 154, 0),
												  ImColor(47, 70, 154, 20),
												  ImColor(47, 70, 154, 20));

		window->DrawList->AddRect(ImVec2(new_grab_bb_max - CalcTextSize(value_buf, value_buf_end).x / 2 - 4, grab_bb.Min.y - 8) - ImVec2(8, 0), ImVec2(new_grab_bb_max + CalcTextSize(value_buf, value_buf_end).x / 2 + 4, grab_bb.Max.y + 8) - ImVec2(8, 0), ImColor(47, 70, 154, 255));

		RenderTextClipped(ImVec2(new_grab_bb_max - CalcTextSize(value_buf, value_buf_end).x / 2, grab_bb.Min.y - 8) - ImVec2(7, 0), ImVec2(new_grab_bb_max + CalcTextSize(value_buf, value_buf_end).x / 2, grab_bb.Max.y + 8) - ImVec2(7, 0), value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));
	}
	PopStyleColor( );

	//if (label_size.x > 0.0f)
		//RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return value_changed;
}

bool c_oink_ui::slider_int(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	ImGui::SetCursorPosX(gap * m_dpi_scaling);
	return slider_scalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

bool c_oink_ui::slider_float(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
	ImGui::SetCursorPosX(gap * m_dpi_scaling);
	return slider_scalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}
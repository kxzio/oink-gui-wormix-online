#include "../ui.h"

#include <sstream>

using namespace ImGui;

bool button_ex(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, const ImColor& theme_colour, const float& dpi_scale)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;

	ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	const ImRect check_bb = bb;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	// Render
	const ImU32 col = GetColorU32((pressed) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);

	float alpha = g_ui.process_animation(label, 1, g.LastActiveId == id && g.LastActiveIdTimer < 0.15f, 0.5f, 15.f, e_animation_type::animation_dynamic);
	float hovered_alpha = g_ui.process_animation(label, 2, hovered, 0.5f, 15.f, e_animation_type::animation_dynamic);

	//background

	ImColor color = theme_colour;

	color.Value.w = alpha;

	window->DrawList->AddRectFilled(bb.Min, bb.Max, color, style.FrameRounding);
	//outline

	color.Value.w = 0.78f + hovered_alpha;
	window->DrawList->AddRect(bb.Min, bb.Max, color, style.FrameRounding);

	auto backup_size = window->FontWindowScale;
	//SetWindowFontScale(window->FontWindowScale + (text_size / 1000.f));
	window->DrawList->AddText(bb.Min + bb.GetSize( ) / 2 - label_size / 2, ImColor(1.f, 1.f, 1.f, 0.7f + hovered_alpha), label);

	//setup font size
	SetWindowFontScale(backup_size);

	// Automatically close popups
	if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
		CloseCurrentPopup( );

	return pressed;
}

bool c_oink_ui::sub_button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, int this_tab, int opened_tab)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	RenderNavHighlight(bb, id);

	float alpha = g_ui.process_animation(label, 4, this_tab == opened_tab, 0.5, 15.f, e_animation_type::animation_dynamic);
	float hovered_alpha = g_ui.process_animation(label, 5, hovered, 0.5, 15.f, e_animation_type::animation_dynamic);

	ImColor color = m_theme_colour;


	// Render
	const ImU32 col = GetColorU32((pressed) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);

	//background
	color.Value.w = 0.078f + alpha;
	window->DrawList->AddRectFilled(bb.Min, bb.Max, color, style.FrameRounding);
	//outline

	color.Value.w = 0.39f + alpha;
	window->DrawList->AddRect(bb.Min, bb.Max, color, style.FrameRounding);

	window->DrawList->AddText(bb.Min + bb.GetSize( ) / 2.f - CalcTextSize(label) / 2.f, ImColor(1.f, 1.f, 1.f, 0.3f + alpha + hovered_alpha), label);

	// Automatically close popups
	if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
		CloseCurrentPopup( );

	return pressed;
}

bool c_oink_ui::tab_button(const char* label, ImVec2 size_arg, ImGuiButtonFlags flags, bool is_tab_active)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	size_arg.y *= m_dpi_scaling;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;

	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);

	float animation_hovered = g_ui.process_animation(label, 6, hovered, 0.39f, 15.f, e_animation_type::animation_dynamic);

	float selected_alpha = g_ui.process_animation(label, 7, is_tab_active, 0.78f, 15.f, e_animation_type::animation_dynamic);

	ImColor text_color = IM_COL32_WHITE;

	text_color.Value.w = 0.15f + animation_hovered + selected_alpha;

	RenderNavHighlight(bb, id);
	PushStyleColor(ImGuiCol_Text, text_color.Value);
	{
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
	}
	PopStyleColor( );

	//GetBackgroundDrawList()->AddRectFilledMultiColor(ImVec2(bb.Min.x, bb.Max.y - hovered_alpha), ImVec2(bb.Max.x, bb.Max.y),

	// Automatically close popups
	if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
		CloseCurrentPopup( );

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}

bool c_oink_ui::button(const char* label, const ImVec2& size_arg)
{
	set_cursor_pos_x(m_gap);
	return ::button_ex(label, size_arg * m_dpi_scaling, ImGuiButtonFlags_None, m_theme_colour, m_dpi_scaling);
}

bool c_oink_ui::button_ex(const char* label, const ImVec2& size_arg, const ImGuiButtonFlags& flags)
{
	return ::button_ex(label, size_arg * m_dpi_scaling, flags, m_theme_colour, m_dpi_scaling);
}
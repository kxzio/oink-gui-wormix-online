#include "../ui.h"

#include <sstream>

using namespace ImGui;

bool button_ex(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, const ImColor& theme)
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

	// кароче время коглда последний прессед был
	auto& last_click_time = g_ui.m_animations.try_emplace(g_ui.generate_unique_id(label , 1), 0.0f).first->second;
	if (pressed)
		last_click_time = GetTime( ); //обновляем при прессе
	double time_since_last_click = ImGui::GetTime( ) - last_click_time; //считаем время с последнего клика
	//(это на всякий случай все), это нужно для длительной анимации после нажатия клика смотри в process animation снизу

	float alpha = g_ui.process_animation(label, 5, time_since_last_click < 0.15, 0.5, 15.f, e_animation_type::animation_dynamic);
	float hovered_alpha = g_ui.process_animation(label, 6, hovered, 0.5, 15.f, e_animation_type::animation_dynamic);


	//background

	ImColor color = theme;
	color.Value.w = 20.f / 255.f + alpha;

	window->DrawList->AddRectFilled(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), color, style.FrameRounding);
	//outline

	color.Value.w = 150.f / 255.f + hovered_alpha;
	window->DrawList->AddRect(bb.Min, bb.Max, color, style.FrameRounding);

	auto backup_size = window->FontWindowScale;
	//SetWindowFontScale(window->FontWindowScale + (text_size / 1000.f));
	window->DrawList->AddText(bb.Min + bb.GetSize( ) / 2 - CalcTextSize(label) / 2, ImColor(1.f, 1.f, 1.f, 0.5 + hovered_alpha), label);

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

	float alpha = g_ui.process_animation(label, 5, this_tab == opened_tab, 0.5, 15.f, e_animation_type::animation_dynamic);
	float hovered_alpha = g_ui.process_animation(label, 6, hovered, 0.5, 15.f, e_animation_type::animation_dynamic);

	ImColor color = m_theme_colour;

	
	// Render
	const ImU32 col = GetColorU32((pressed) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);

	//background
	color.Value.w = 20.f / 255.f + alpha;
	window->DrawList->AddRectFilled(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), color, style.FrameRounding);
	//outline

	color.Value.w = 100 / 255.f + alpha;
	window->DrawList->AddRect(bb.Min, bb.Max, color, style.FrameRounding);

	window->DrawList->AddText(bb.Min + bb.GetSize( ) / 2 - CalcTextSize(label) / 2, ImColor(1.f, 1.f, 1.f, 0.3f + alpha + hovered_alpha), label);

	// Automatically close popups
	if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
		CloseCurrentPopup( );

	return pressed;
}

bool c_oink_ui::tab_button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, int this_tab, int opened_tab)
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

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	int hovered1 = g_ui.process_animation(label, 8, hovered, 100, 15.f, e_animation_type::animation_dynamic);
	float alpha = g_ui.process_animation(label, 9, this_tab == opened_tab, 200, 15.f, e_animation_type::animation_dynamic);
	int selected_alpha = g_ui.process_animation(label, 10, this_tab == opened_tab, 200, 15.f, e_animation_type::animation_dynamic);

	ImColor txt_clr = ImColor(255, 255, 255, 40 + hovered1 + selected_alpha);

	RenderNavHighlight(bb, id);
	PushStyleColor(ImGuiCol_Text, txt_clr.Value);
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
	ImGui::SetCursorPosX(m_gap * m_dpi_scaling);
	return button_ex(label, size_arg, ImGuiButtonFlags_None, m_theme_colour);
}
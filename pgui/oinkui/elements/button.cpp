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

	int text_size = g_ui.process_animation(label, 1, hovered, 120, 15.f, e_animation_type::animation_static);

	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	auto ID = GetID((std::stringstream{} << label << "adcty").str( ).c_str( ));

	static std::unordered_map <ImGuiID, int> pValue;
	auto ItPLibrary = pValue.find(ID);

	if (ItPLibrary == pValue.end( ))
	{
		pValue.insert({ ID, 0 });
		ItPLibrary = pValue.find(ID);
	}

	static std::unordered_map <ImGuiID, int> pValue2;
	auto ItPLibrary2 = pValue2.find(ID);

	if (ItPLibrary2 == pValue2.end( ))
	{
		pValue2.insert({ ID, 0 });
		ItPLibrary2 = pValue2.find(ID);
	}

	int alpha_active = g_ui.process_animation(label, 2, true, ItPLibrary2->second, 15.f, e_animation_type::animation_interp);
	if (pressed)
		ItPLibrary->second = true;

	if (ItPLibrary->second)
	{
		ItPLibrary2->second = 255;
		if (alpha_active > 200)
			ItPLibrary->second = false;
	}
	else
		ItPLibrary2->second = 0;

	// Render
	const ImU32 col = GetColorU32((pressed) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);

	//background

	ImColor color = theme;
	color.Value.w = 20.f / 255.f + alpha_active;

	window->DrawList->AddRectFilled(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), color, style.FrameRounding);
	//outline

	color.Value.w = 150.f / 255.f + text_size;
	window->DrawList->AddRect(bb.Min, bb.Max, color, style.FrameRounding);

	auto backup_size = window->FontWindowScale;
	//SetWindowFontScale(window->FontWindowScale + (text_size / 1000.f));
	window->DrawList->AddText(bb.Min + bb.GetSize( ) / 2 - CalcTextSize(label) / 2, ImColor(255, 255, 255, 150 + text_size), label);

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

	int anim_active = g_ui.process_animation(label, 3, this_tab == opened_tab, 200, 15.f, e_animation_type::animation_static);

	float hovered1 = g_ui.process_animation(label, 4, 200, 15.f, e_animation_type::animation_dynamic);
	float alpha = g_ui.process_animation(label, 5, this_tab == opened_tab, 200, 15.f, e_animation_type::animation_dynamic);
	float hovered_alpha = g_ui.process_animation(label, 6, this_tab == opened_tab, bb.GetSize( ).y, 15.f, e_animation_type::animation_dynamic);

	ImColor color = m_theme_colour;

	auto ID = ImGui::GetID((std::stringstream{} << label << "adcty").str( ).c_str( ));

	static std::map <ImGuiID, int> pValue;
	auto ItPLibrary = pValue.find(ID);

	if (ItPLibrary == pValue.end( ))
	{
		pValue.insert({ ID, 0 });
		ItPLibrary = pValue.find(ID);
	}

	static std::map <ImGuiID, int> pValue2;
	auto ItPLibrary2 = pValue2.find(ID);

	if (ItPLibrary2 == pValue2.end( ))
	{
		pValue2.insert({ ID, 0 });
		ItPLibrary2 = pValue2.find(ID);
	}

	int alpha_active = g_ui.process_animation(label, 7, true, ItPLibrary2->second, 15.f, e_animation_type::animation_interp);

	if (this_tab == opened_tab)
	{
		ItPLibrary->second = true;
	}
	if (ItPLibrary->second)
	{
		ItPLibrary2->second = 255;
		if (alpha_active > 200)
		{
			ItPLibrary->second = false;
		}
	}
	else
	{
		ItPLibrary2->second = 0;
	}

	// Render
	const ImU32 col = GetColorU32((pressed) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);

	//background
	color.Value.w = 20.f / 255.f + alpha_active;
	window->DrawList->AddRectFilled(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), color, style.FrameRounding);
	//outline

	color.Value.w = 150 / 255.f + alpha_active;
	window->DrawList->AddRect(bb.Min, bb.Max, color, style.FrameRounding);

	window->DrawList->AddText(bb.Min + bb.GetSize( ) / 2 - CalcTextSize(label) / 2, ImColor(255, 255, 255, 150 + alpha_active), label);

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
#include "../ui.h"

using namespace ImGui;

bool begin_combo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow( );

	auto backuppos = GetCursorPosX( );
	auto backup = GetStyle( ).Colors[ImGuiCol_Text];
	GetStyle( ).Colors[ImGuiCol_Text] = ImColor(47, 70, 154, 255);
	Text(label);
	GetStyle( ).Colors[ImGuiCol_Text] = backup;

	SetCursorPosX(backuppos);

	ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
	g.NextWindowData.ClearFlags( ); // We behave like Begin() and need to consume those values
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight( );
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const float w = 186;
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	const ImRect total_bb(bb.Min, bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &bb))
		return false;

	// Open on click
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);
	const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
	bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
	if (pressed && !popup_open)
	{
		OpenPopupEx(popup_id, ImGuiPopupFlags_None);
		popup_open = true;
	}

	// Render shape
	const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
	RenderNavHighlight(bb, id);

	if (!(flags & ImGuiComboFlags_NoArrowButton))
	{
		ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		ImU32 text_col = GetColorU32(ImGuiCol_Text);
	}

	window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max,
											  ImColor(47, 70, 154, 0), ImColor(47, 70, 154, 0), ImColor(47, 70, 154, 20), ImColor(47, 70, 154, 20));

	float hovered1 = g_ui.process_animation("button_hover", label, hovered || g.ActiveId == id, 200, 0.07, e_animation_type::animation_dynamic);

	int auto_red = 47;
	int auto_green = 70;
	int auto_blue = 154;

	window->DrawList->AddRectFilled(bb.Min, bb.Max,
									ImColor(auto_red, auto_green, auto_blue, int(hovered1 / 5)));

	window->DrawList->AddRect(bb.Min, bb.Max, ImColor(47, 70, 154, 255));

	// Custom preview
	if (flags & ImGuiComboFlags_CustomPreview)
	{
		g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
		IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
		preview_value = NULL;
	}

	// Render preview and label
	if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
	{
		if (g.LogEnabled)
			LogSetNextTextDecoration("{", "}");
		RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
	}

	if (!popup_open)
		return false;

	g.NextWindowData.Flags = backup_next_window_data_flags;
	return BeginComboPopup(popup_id, bb, flags);
}

bool c_oink_ui::combo(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items)
{
	ImGuiContext& g = *GImGui;

	// Call the getter to obtain the preview string which is a parameter to BeginCombo()
	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(data, *current_item, &preview_value);

	// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
	if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	if (!begin_combo(label, preview_value, ImGuiComboFlags_None))
		return false;

	// Display items
	// FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
	bool value_changed = false;
	for (int i = 0; i < items_count; i++)
	{
		PushID(i);
		bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(data, i, &item_text))
			item_text = "*Unknown item*";
		if (selectable(item_text, &item_selected))
		{
			value_changed = true;
			*current_item = i;
		}
		if (item_selected)
			SetItemDefaultFocus( );
		PopID( );
	}

	EndCombo( );

	if (value_changed)
		MarkItemEdited(g.LastItemData.ID);

	return value_changed;
}

void c_oink_ui::multi_box(const char* title, bool selection[ ], const char* text[ ], int size)
{
	ImGui::SetCursorPosX(gap * m_dpi_scaling);

	ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeight(-1)));

	std::string combo = "";

	ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeight(-1)));

	std::vector <std::string> vec;

	for (size_t i = 0; i < size; i++)
	{
		if (selection[i])
		{
			combo += text[i];
			combo += ", ";
		}
	}

	if (combo.length( ) > 2)
		combo.erase(combo.length( ) - 2, combo.length( ));

	if (begin_combo(title, combo.c_str( ), ImGuiComboFlags_NoArrowButton))
	{
		std::vector<std::string> vec;
		for (size_t i = 0; i < size; i++)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));

			selectable(text[i], &selection[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);

			if (selection[i])
				vec.push_back(text[i]);

			ImGui::PopStyleVar( );
		}

		ImGui::EndCombo( );
	}
};

bool c_oink_ui::combo_box(const char* label, int* current_item, const char* const items[ ], int items_count, int height_in_items)
{
	ImGui::SetCursorPosX(gap * m_dpi_scaling);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));
	const bool value_changed = combo(label, current_item, Items_ArrayGetter, (void*) items, items_count, height_in_items);
	ImGui::PopStyleVar( );
	return value_changed;
}
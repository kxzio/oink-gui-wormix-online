#include "../ui.h"

#include <sstream>

using namespace ImGui;

bool begin_combo(const char* label, const char* preview_value, ImGuiComboFlags flags, const ImColor& theme_colour, const float& dpi_scale)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow( );

	ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
	g.NextWindowData.ClearFlags( ); // We behave like Begin() and need to consume those values
	if (window->SkipItems)
		return false;

	float backup_x = ImGui::GetCursorPosX( );

	ImColor color = theme_colour;

	ImGui::PushStyleColor(ImGuiCol_Text, color.Value);
	Text(label);
	ImGui::PopStyleColor( );

	ImGui::SetCursorPosX(backup_x);

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const float w = 186 * dpi_scale;

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight( );
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f * dpi_scale));
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

	color.Value.w = 0.07f;
	window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max, IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, color, color);

	float animation_active_hovered = g_ui.process_animation(label, 1, hovered || pressed, 0.78f, 0.07f, e_animation_type::animation_dynamic);

	color.Value.w = animation_active_hovered / 5;
	window->DrawList->AddRectFilled(bb.Min, bb.Max, color);

	color.Value.w = 1.f;
	window->DrawList->AddRect(bb.Min, bb.Max, color);

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

bool c_oink_ui::combo_box(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items)
{
	ImGuiContext& g = *GImGui;

	// Call the getter to obtain the preview string which is a parameter to BeginCombo()
	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(data, *current_item, &preview_value);

	// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
	if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	if (!begin_combo(label, preview_value, ImGuiComboFlags_None, m_theme_colour, m_dpi_scaling))
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
	ImGui::SetCursorPosX(m_gap * m_dpi_scaling);

	auto max_popup_height = calc_max_popup_height(-1);

	ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, max_popup_height));

	std::string render_str;
	size_t active_items = 0u;

	for (size_t i = 0u; i < size; ++i)
	{
		if (selection[i])
		{
			active_items++;
			if (active_items > 1u)
				render_str.append(", ");
			render_str.append(text[i]);
		}
	}

	if (active_items < 1)
		render_str = "None";

	if (begin_combo(title, render_str.c_str( ), ImGuiComboFlags_NoArrowButton, m_theme_colour, m_dpi_scaling))
	{
		for (size_t i = 0u; i < size; ++i)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));
			selectable(text[i], &selection[i], ImGuiSelectableFlags_DontClosePopups);
			ImGui::PopStyleVar( );
		};

		ImGui::EndCombo( );
	};
};


bool c_oink_ui::combo_box(const char* label, int* current_item, void* const items, int items_count, int height_in_items)
{
	ImGui::SetCursorPosX(m_gap * m_dpi_scaling);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10 * m_dpi_scaling));
	const bool value_changed = combo_box(label, current_item, Items_ArrayGetter, items, items_count, height_in_items);
	ImGui::PopStyleVar( );
	return value_changed;
}

bool c_oink_ui::combo_box(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
	ImGui::SetCursorPosX(m_gap * m_dpi_scaling);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10 * m_dpi_scaling));
	int items_count = 0;
	const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
	while (*p)
	{
		p += strlen(p) + 1;
		items_count++;
	}
	bool value_changed = combo_box(label, current_item, Items_SingleStringGetter, (void*) items_separated_by_zeros, items_count, height_in_items);
	ImGui::PopStyleVar( );
	return value_changed;
}
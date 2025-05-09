#include "../ui.h"

using namespace ImGui;

bool selectable_ex(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg, const ImColor& theme_colour, const float& dpi_scale)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	ImColor color = theme_colour;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	// Submit label or explicit size to ItemSize(), whereas ItemAdd() will submit a larger/spanning rectangle.
	ImGuiID id = window->GetID(label);
	ImVec2 label_size = CalcTextSize(label, NULL, true);
	ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
	ImVec2 pos = window->DC.CursorPos;
	pos.y += window->DC.CurrLineTextBaseOffset;
	ItemSize(size, 0.0f);

	// Fill horizontal space
	// We don't support (size < 0.0f) in Selectable() because the ItemSpacing extension would make explicitly right-aligned sizes not visibly match other widgets.
	const bool span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
	const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
	const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
	if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth))
		size.x = ImMax(label_size.x, max_x - min_x);

	// Text stays at the submission position, but bounding box may be extended on both sides
	const ImVec2 text_min = pos + ImVec2(3.f * dpi_scale, 0.f);
	const ImVec2 text_max(min_x + size.x, pos.y + size.y);

	// Selectables are meant to be tightly packed together with no click-gap, so we extend their box to cover spacing between selectable.
	ImRect bb(min_x, pos.y, text_max.x, text_max.y);
	if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
	{
		const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
		const float spacing_y = style.ItemSpacing.y;
		const float spacing_L = IM_FLOOR(spacing_x * 0.50f);
		const float spacing_U = IM_FLOOR(spacing_y * 0.50f);
		bb.Min.x -= spacing_L;
		bb.Min.y -= spacing_U;
		bb.Max.x += (spacing_x - spacing_L);
		bb.Max.y += (spacing_y - spacing_U);
	}
	//if (g.IO.KeyCtrl) { GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(0, 255, 0, 255)); }

	// Modify ClipRect for the ItemAdd(), faster than doing a PushColumnsBackground/PushTableBackground for every Selectable..
	const float backup_clip_rect_min_x = window->ClipRect.Min.x;
	const float backup_clip_rect_max_x = window->ClipRect.Max.x;
	if (span_all_columns)
	{
		window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
		window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
	}

	const bool disabled_item = (flags & ImGuiSelectableFlags_Disabled) != 0;
	const bool item_add = ItemAdd(bb, id, NULL, disabled_item ? ImGuiItemFlags_Disabled : ImGuiItemFlags_None);
	if (span_all_columns)
	{
		window->ClipRect.Min.x = backup_clip_rect_min_x;
		window->ClipRect.Max.x = backup_clip_rect_max_x;
	}

	if (!item_add)
		return false;

	const bool disabled_global = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
	if (disabled_item && !disabled_global) // Only testing this as an optimization
		BeginDisabled( );

	// FIXME: We can standardize the behavior of those two, we could also keep the fast path of override ClipRect + full push on render only,
	// which would be advantageous since most selectable are not selected.
	if (span_all_columns && window->DC.CurrentColumns)
		PushColumnsBackground( );
	else if (span_all_columns && g.CurrentTable)
		TablePushBackgroundChannel( );

	// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
	ImGuiButtonFlags button_flags = 0;
	if (flags & ImGuiSelectableFlags_NoHoldingActiveID)
	{
		button_flags |= ImGuiButtonFlags_NoHoldingActiveId;
	}
	if (flags & ImGuiSelectableFlags_SelectOnClick)
	{
		button_flags |= ImGuiButtonFlags_PressedOnClick;
	}
	if (flags & ImGuiSelectableFlags_SelectOnRelease)
	{
		button_flags |= ImGuiButtonFlags_PressedOnRelease;
	}
	if (flags & ImGuiSelectableFlags_AllowDoubleClick)
	{
		button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
	}
	if (flags & ImGuiSelectableFlags_AllowItemOverlap)
	{
		button_flags |= ImGuiButtonFlags_AllowItemOverlap;
	}

	const bool was_selected = selected;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);

	// Auto-select when moved into
	// - This will be more fully fleshed in the range-select branch
	// - This is not exposed as it won't nicely work with some user side handling of shift/control
	// - We cannot do 'if (g.NavJustMovedToId != id) { selected = false; pressed = was_selected; }' for two reasons
	//   - (1) it would require focus scope to be set, need exposing PushFocusScope() or equivalent (e.g. BeginSelection() calling PushFocusScope())
	//   - (2) usage will fail with clipped items
	//   The multi-select API aim to fix those issues, e.g. may be replaced with a BeginSelection() API.
	if ((flags & ImGuiSelectableFlags_SelectOnNav) && g.NavJustMovedToId != 0 && g.NavJustMovedToFocusScopeId == g.CurrentFocusScopeId)
		if (g.NavJustMovedToId == id)
			selected = pressed = true;

	// Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with gamepad/keyboard
	if (pressed || (hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover)))
	{
		if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
		{
			SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, WindowRectAbsToRel(window, bb)); // (bb == NavRect)
			g.NavDisableHighlight = true;
		}
	}
	if (pressed)
		MarkItemEdited(id);

	if (flags & ImGuiSelectableFlags_AllowItemOverlap)
		SetItemAllowOverlap( );

	// In this branch, Selectable() cannot toggle the selection so this will never trigger.
	if (selected != was_selected) //-V547
		g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

	// Render
	//if (hovered || selected)
	//{
	//	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
	//	RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
	//}

	RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);

	if (span_all_columns && window->DC.CurrentColumns)
		PopColumnsBackground( );
	else if (span_all_columns && g.CurrentTable)
		TablePopBackgroundChannel( );

	float alpha = g_ui.process_animation(label, 1, hovered && !selected, 0.6f, 15.f, e_animation_type::animation_dynamic);
	float alpha_selected = g_ui.process_animation(label, 2, selected, 0.6f, 15.f, e_animation_type::animation_dynamic);

	RenderFrame(bb.Min, bb.Max, ImColor(0, 0, 0, 100), false, 0.0f);

	ImColor c1 = g_ui.m_theme_colour_primary;  c1.Value.w = alpha_selected;
	if (alpha_selected > 0.f)
	{
		auto bb_size = bb.GetSize( );

		window->DrawList->AddRectFilled(bb.Min, bb.Min + ImVec2(1.f, bb_size.y), c1, 0.f, 0);
		window->DrawList->AddRectFilled(bb.Max, bb.Max - ImVec2(1.f, bb_size.y), c1, 0.f, 0);

		color.Value.w = alpha_selected * 0.5f;

		if (color.Value.w > 0.0f)
		{
			window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Min + ImVec2(30.f * dpi_scale, bb_size.y), color, IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, color);
			window->DrawList->AddRectFilledMultiColor(bb.Max, bb.Max - ImVec2(30.f * dpi_scale, bb_size.y), color, IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, color);
		};
	};

	//default
	PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.2f + alpha));
	RenderTextClipped(text_min, text_max, label, NULL, &label_size, style.SelectableTextAlign, &bb);
	PopStyleColor( );

	//selected
	if (alpha_selected > 0.0f)
	{
		PushStyleColor(ImGuiCol_Text, c1.Value);
		RenderTextClipped(text_min, text_max, label, NULL, &label_size, style.SelectableTextAlign, &bb);
		PopStyleColor( );
	};

	// Automatically close popups
	if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(g.LastItemData.InFlags & ImGuiItemFlags_SelectableDontClosePopup))
		CloseCurrentPopup( );

	if (disabled_item && !disabled_global)
		EndDisabled( );

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed; //-V1020
}

bool c_oink_ui::selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
	if (selectable_ex(label, *p_selected, flags, size_arg, m_theme_colour_primary, m_dpi_scaling))
	{
		*p_selected = !*p_selected;
		return true;
	}
	return false;
}
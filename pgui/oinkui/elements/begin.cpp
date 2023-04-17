#include "../ui.h"

using namespace ImGui;
static const float WINDOWS_HOVER_PADDING = 4.0f;     // Extend outside window for hovering/resizing (maxxed with TouchPadding) and inside windows for borders. Affect FindHoveredWindow().
static const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f;    // Reduce visual noise by only highlighting the border after a certain time.
static const float WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER = 2.00f;    // Lock scrolled window (so it doesn't pick child windows that are scrolling through) for a certain time, unless mouse moved.

// Data for resizing from corner
struct ImGuiResizeGripDef
{
	ImVec2  CornerPosN;
	ImVec2  InnerDir;
	int     AngleMin12, AngleMax12;
};
static const ImGuiResizeGripDef resize_grip_def[4] =
{
	{ ImVec2(1, 1), ImVec2(-1, -1), 0, 3 },  // Lower-right
	{ ImVec2(0, 1), ImVec2(+1, -1), 3, 6 },  // Lower-left
	{ ImVec2(0, 0), ImVec2(+1, +1), 6, 9 },  // Upper-left (Unused)
	{ ImVec2(1, 0), ImVec2(-1, +1), 9, 12 }  // Upper-right (Unused)
};

// Data for resizing from borders
struct ImGuiResizeBorderDef
{
	ImVec2 InnerDir;
	ImVec2 SegmentN1, SegmentN2;
	float  OuterAngle;
};
static const ImGuiResizeBorderDef resize_border_def[4] =
{
	{ ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), IM_PI * 1.00f }, // Left
	{ ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), IM_PI * 0.00f }, // Right
	{ ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), IM_PI * 1.50f }, // Up
	{ ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), IM_PI * 0.50f }  // Down
};

void SetCurrentWindow(ImGuiWindow* window)
{
	ImGuiContext& g = *GImGui;
	g.CurrentWindow = window;
	g.CurrentTable = window && window->DC.CurrentTableIdx != -1 ? g.Tables.GetByIndex(window->DC.CurrentTableIdx) : NULL;
	if (window)
		g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize( );
}

bool scrollbar_ex(const ImRect& bb_frame, ImGuiID id, ImGuiAxis axis, ImS64* p_scroll_v, ImS64 size_avail_v, ImS64 size_contents_v, ImDrawFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (window->SkipItems)
		return false;

	const float bb_frame_width = bb_frame.GetWidth( );
	const float bb_frame_height = bb_frame.GetHeight( );
	if (bb_frame_width <= 0.0f || bb_frame_height <= 0.0f)
		return false;

	// When we are too small, start hiding and disabling the grab (this reduce visual noise on very small window and facilitate using the window resize grab)
	float alpha = 1.0f;
	if ((axis == ImGuiAxis_Y) && bb_frame_height < g.FontSize + g.Style.FramePadding.y * 2.0f)
		alpha = ImSaturate((bb_frame_height - g.FontSize) / (g.Style.FramePadding.y * 2.0f));
	if (alpha <= 0.0f)
		return false;

	const ImGuiStyle& style = g.Style;
	const bool allow_interaction = (alpha >= 1.0f);

	ImRect bb = bb_frame;
	bb.Min -= ImVec2(4, -2);
	bb.Max -= ImVec2(4, 5);

	bb.Expand(ImVec2(-ImClamp(IM_FLOOR((bb_frame_width - 2.0f) * 0.5f), 0.0f, 3.0f), -ImClamp(IM_FLOOR((bb_frame_height - 2.0f) * 0.5f), 0.0f, 3.0f)));

	// V denote the main, longer axis of the scrollbar (= height for a vertical scrollbar)
	const float scrollbar_size_v = (axis == ImGuiAxis_X) ? bb.GetWidth( ) : bb.GetHeight( );

	// Calculate the height of our grabbable box. It generally represent the amount visible (vs the total scrollable amount)
	// But we maintain a minimum size in pixel to allow for the user to still aim inside.
	IM_ASSERT(ImMax(size_contents_v, size_avail_v) > 0.0f); // Adding this assert to check if the ImMax(XXX,1.0f) is still needed. PLEASE CONTACT ME if this triggers.
	const ImS64 win_size_v = ImMax(ImMax(size_contents_v, size_avail_v), (ImS64) 1);
	const float grab_h_pixels = ImClamp(scrollbar_size_v * ((float) size_avail_v / (float) win_size_v), style.GrabMinSize, scrollbar_size_v);
	const float grab_h_norm = grab_h_pixels / scrollbar_size_v;

	// Handle input right away. None of the code of Begin() is relying on scrolling position before calling Scrollbar().
	bool held = false;
	bool hovered = false;
	ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);

	const ImS64 scroll_max = ImMax((ImS64) 1, size_contents_v - size_avail_v);
	float scroll_ratio = ImSaturate((float) *p_scroll_v / (float) scroll_max);
	float grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v; // Grab position in normalized space
	if (held && allow_interaction && grab_h_norm < 1.0f)
	{
		const float scrollbar_pos_v = bb.Min[axis];
		const float mouse_pos_v = g.IO.MousePos[axis];

		// Click position in scrollbar normalized space (0.0f->1.0f)
		const float clicked_v_norm = ImSaturate((mouse_pos_v - scrollbar_pos_v) / scrollbar_size_v);
		SetHoveredID(id);

		bool seek_absolute = false;
		if (g.ActiveIdIsJustActivated)
		{
			// On initial click calculate the distance between mouse and the center of the grab
			seek_absolute = (clicked_v_norm < grab_v_norm || clicked_v_norm > grab_v_norm + grab_h_norm);
			if (seek_absolute)
				g.ScrollbarClickDeltaToGrabCenter = 0.0f;
			else
				g.ScrollbarClickDeltaToGrabCenter = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
		}

		// Apply scroll (p_scroll_v will generally point on one member of window->Scroll)
		// It is ok to modify Scroll here because we are being called in Begin() after the calculation of ContentSize and before setting up our starting position
		const float scroll_v_norm = ImSaturate((clicked_v_norm - g.ScrollbarClickDeltaToGrabCenter - grab_h_norm * 0.5f) / (1.0f - grab_h_norm));
		*p_scroll_v = (ImS64) (scroll_v_norm * scroll_max);

		// Update values for rendering
		scroll_ratio = ImSaturate((float) *p_scroll_v / (float) scroll_max);
		grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;

		// Update distance to grab now that we have seeked and saturated
		if (seek_absolute)
			g.ScrollbarClickDeltaToGrabCenter = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
	}

	// Render
	const ImU32 bg_col = GetColorU32(ImGuiCol_ScrollbarBg);
	const ImColor grab_col = g_ui.m_theme_colour_primary;
	window->DrawList->AddRectFilled(bb_frame.Min, bb_frame.Max, bg_col, window->WindowRounding, flags);
	ImRect grab_rect;
	if (axis == ImGuiAxis_X)
		grab_rect = ImRect(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm), bb.Min.y, ImLerp(bb.Min.x, bb.Max.x, grab_v_norm) + grab_h_pixels, bb.Max.y);
	else
		grab_rect = ImRect(bb.Min.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm), bb.Max.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm) + grab_h_pixels);

	window->DrawList->AddRectFilled(ImVec2(grab_rect.Min.x, grab_rect.Min.y), grab_rect.Max, grab_col, style.ScrollbarRounding);

	return held;
}

void scrollbar(ImGuiAxis axis)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	const ImGuiID id = GetWindowScrollbarID(window, axis);
	KeepAliveID(id);

	// Calculate scrollbar bounding box
	ImRect bb = GetWindowScrollbarRect(window, axis);
	ImDrawFlags rounding_corners = ImDrawFlags_RoundCornersNone;
	if (axis == ImGuiAxis_X)
	{
		rounding_corners |= ImDrawFlags_RoundCornersBottomLeft;
		if (!window->ScrollbarY)
			rounding_corners |= ImDrawFlags_RoundCornersBottomRight;
	}
	else
	{
		if ((window->Flags & ImGuiWindowFlags_NoTitleBar) && !(window->Flags & ImGuiWindowFlags_MenuBar))
			rounding_corners |= ImDrawFlags_RoundCornersTopRight;
		if (!window->ScrollbarX)
			rounding_corners |= ImDrawFlags_RoundCornersBottomRight;
	}
	float size_avail = window->InnerRect.Max[axis] - window->InnerRect.Min[axis];
	float size_contents = window->ContentSize[axis] + window->WindowPadding[axis] * 2.0f;
	ImS64 scroll = (ImS64) window->Scroll[axis];
	scrollbar_ex(bb, id, axis, &scroll, (ImS64) size_avail, (ImS64) size_contents, rounding_corners);
	window->Scroll[axis] = (float) scroll;
}

ImVec2 CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window)
{
	ImVec2 scroll = window->Scroll;
	if (window->ScrollTarget.x < FLT_MAX)
	{
		float decoration_total_width = window->ScrollbarSizes.x;
		float center_x_ratio = window->ScrollTargetCenterRatio.x;
		float scroll_target_x = window->ScrollTarget.x;
		if (window->ScrollTargetEdgeSnapDist.x > 0.0f)
		{
			float snap_x_min = 0.0f;
			float snap_x_max = window->ScrollMax.x + window->SizeFull.x - decoration_total_width;
			scroll_target_x = CalcScrollEdgeSnap(scroll_target_x, snap_x_min, snap_x_max, window->ScrollTargetEdgeSnapDist.x, center_x_ratio);
		}
		scroll.x = scroll_target_x - center_x_ratio * (window->SizeFull.x - decoration_total_width);
	}
	if (window->ScrollTarget.y < FLT_MAX)
	{
		float decoration_total_height = window->TitleBarHeight( ) + window->MenuBarHeight( ) + window->ScrollbarSizes.y;
		float center_y_ratio = window->ScrollTargetCenterRatio.y;
		float scroll_target_y = window->ScrollTarget.y;
		if (window->ScrollTargetEdgeSnapDist.y > 0.0f)
		{
			float snap_y_min = 0.0f;
			float snap_y_max = window->ScrollMax.y + window->SizeFull.y - decoration_total_height;
			scroll_target_y = CalcScrollEdgeSnap(scroll_target_y, snap_y_min, snap_y_max, window->ScrollTargetEdgeSnapDist.y, center_y_ratio);
		}
		scroll.y = scroll_target_y - center_y_ratio * (window->SizeFull.y - decoration_total_height);
	}
	scroll.x = IM_FLOOR(ImMax(scroll.x, 0.0f));
	scroll.y = IM_FLOOR(ImMax(scroll.y, 0.0f));
	if (!window->Collapsed && !window->SkipItems)
	{
		scroll.x = ImMin(scroll.x, window->ScrollMax.x);
		scroll.y = ImMin(scroll.y, window->ScrollMax.y);
	}
	return scroll;
}

static ImGuiWindow* CreateNewWindow(const char* name, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	//IMGUI_DEBUG_LOG("CreateNewWindow '%s', flags = 0x%08X\n", name, flags);

	// Create window the first time
	ImGuiWindow* window = IM_NEW(ImGuiWindow)(&g, name);
	window->Flags = flags;
	g.WindowsById.SetVoidPtr(window->ID, window);

	// Default/arbitrary window position. Use SetNextWindowPos() with the appropriate condition flag to change the initial position of a window.
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport( );
	window->Pos = main_viewport->Pos + ImVec2(60, 60);

	// User can disable loading and saving of settings. Tooltip and child windows also don't store settings.
	if (!(flags & ImGuiWindowFlags_NoSavedSettings))
		if (ImGuiWindowSettings* settings = ImGui::FindWindowSettings(window->ID))
		{
			// Retrieve settings from .ini file
			window->SettingsOffset = g.SettingsWindows.offset_from_ptr(settings);
			SetWindowConditionAllowFlags(window, ImGuiCond_FirstUseEver, false);
			ApplyWindowSettings(window, settings);
		}
	window->DC.CursorStartPos = window->DC.CursorMaxPos = window->DC.IdealMaxPos = window->Pos; // So first call to CalcWindowContentSizes() doesn't return crazy values

	if ((flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
	{
		window->AutoFitFramesX = window->AutoFitFramesY = 2;
		window->AutoFitOnlyGrows = false;
	}
	else
	{
		if (window->Size.x <= 0.0f)
			window->AutoFitFramesX = 2;
		if (window->Size.y <= 0.0f)
			window->AutoFitFramesY = 2;
		window->AutoFitOnlyGrows = (window->AutoFitFramesX > 0) || (window->AutoFitFramesY > 0);
	}

	if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
		g.Windows.push_front(window); // Quite slow but rare and only once
	else
		g.Windows.push_back(window);
	UpdateWindowInFocusOrderList(window, true, window->Flags);

	return window;
}

ImGuiWindow* FindBlockingModal(ImGuiWindow* window)
{
	ImGuiContext& g = *GImGui;
	if (g.OpenPopupStack.Size <= 0)
		return NULL;

	// Find a modal that has common parent with specified window. Specified window should be positioned behind that modal.
	for (int i = g.OpenPopupStack.Size - 1; i >= 0; i--)
	{
		ImGuiWindow* popup_window = g.OpenPopupStack.Data[i].Window;
		if (popup_window == NULL || !(popup_window->Flags & ImGuiWindowFlags_Modal))
			continue;
		if (!popup_window->Active && !popup_window->WasActive)      // Check WasActive, because this code may run before popup renders on current frame, also check Active to handle newly created windows.
			continue;
		if (ImGui::IsWindowWithinBeginStackOf(window, popup_window))       // Window is rendered over last modal, no render order change needed.
			break;
		for (ImGuiWindow* parent = popup_window->ParentWindowInBeginStack->RootWindow; parent != NULL; parent = parent->ParentWindowInBeginStack->RootWindow)
			if (ImGui::IsWindowWithinBeginStackOf(window, parent))
				return popup_window;                                // Place window above its begin stack parent.
	}
	return NULL;
}

void RenderWindowTitleBarContents(ImGuiWindow* window, const ImRect& title_bar_rect, const char* name, bool* p_open)
{
	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = g.Style;
	ImGuiWindowFlags flags = window->Flags;

	const bool has_close_button = (p_open != NULL);
	const bool has_collapse_button = !(flags & ImGuiWindowFlags_NoCollapse) && (style.WindowMenuButtonPosition != ImGuiDir_None);

	// Close & Collapse button are on the Menu NavLayer and don't default focus (unless there's nothing else on that layer)
	// FIXME-NAV: Might want (or not?) to set the equivalent of ImGuiButtonFlags_NoNavFocus so that mouse clicks on standard title bar items don't necessarily set nav/keyboard ref?
	const ImGuiItemFlags item_flags_backup = g.CurrentItemFlags;
	g.CurrentItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
	window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

	// Layout buttons
	// FIXME: Would be nice to generalize the subtleties expressed here into reusable code.
	float pad_l = style.FramePadding.x;
	float pad_r = style.FramePadding.x;
	float button_sz = g.FontSize;
	ImVec2 close_button_pos;
	ImVec2 collapse_button_pos;
	if (has_close_button)
	{
		pad_r += button_sz;
		close_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - style.FramePadding.x, title_bar_rect.Min.y);
	}
	if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Right)
	{
		pad_r += button_sz;
		collapse_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - style.FramePadding.x, title_bar_rect.Min.y);
	}
	if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Left)
	{
		collapse_button_pos = ImVec2(title_bar_rect.Min.x + pad_l - style.FramePadding.x, title_bar_rect.Min.y);
		pad_l += button_sz;
	}

	// Collapse button (submitting first so it gets priority when choosing a navigation init fallback)
	if (has_collapse_button)
		if (CollapseButton(window->GetID("#COLLAPSE"), collapse_button_pos))
			window->WantCollapseToggle = true; // Defer actual collapsing to next frame as we are too far in the Begin() function

	// Close button
	if (has_close_button)
		if (CloseButton(window->GetID("#CLOSE"), close_button_pos))
			*p_open = false;

	window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
	g.CurrentItemFlags = item_flags_backup;

	// Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
	// FIXME: Refactor text alignment facilities along with RenderText helpers, this is WAY too much messy code..
	const float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? button_sz * 0.80f : 0.0f;
	const ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);

	// As a nice touch we try to ensure that centered title text doesn't get affected by visibility of Close/Collapse button,
	// while uncentered title text will still reach edges correctly.
	if (pad_l > style.FramePadding.x)
		pad_l += g.Style.ItemInnerSpacing.x;
	if (pad_r > style.FramePadding.x)
		pad_r += g.Style.ItemInnerSpacing.x;
	if (style.WindowTitleAlign.x > 0.0f && style.WindowTitleAlign.x < 1.0f)
	{
		float centerness = ImSaturate(1.0f - ImFabs(style.WindowTitleAlign.x - 0.5f) * 2.0f); // 0.0f on either edges, 1.0f on center
		float pad_extend = ImMin(ImMax(pad_l, pad_r), title_bar_rect.GetWidth( ) - pad_l - pad_r - text_size.x);
		pad_l = ImMax(pad_l, pad_extend * centerness);
		pad_r = ImMax(pad_r, pad_extend * centerness);
	}

	ImRect layout_r(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y, title_bar_rect.Max.x - pad_r, title_bar_rect.Max.y);
	ImRect clip_r(layout_r.Min.x, layout_r.Min.y, ImMin(layout_r.Max.x + g.Style.ItemInnerSpacing.x, title_bar_rect.Max.x), layout_r.Max.y);
	if (flags & ImGuiWindowFlags_UnsavedDocument)
	{
		ImVec2 marker_pos;
		marker_pos.x = ImClamp(layout_r.Min.x + (layout_r.GetWidth( ) - text_size.x) * style.WindowTitleAlign.x + text_size.x, layout_r.Min.x, layout_r.Max.x);
		marker_pos.y = (layout_r.Min.y + layout_r.Max.y) * 0.5f;
		if (marker_pos.x > layout_r.Min.x)
		{
			RenderBullet(window->DrawList, marker_pos, GetColorU32(ImGuiCol_Text));
			clip_r.Max.x = ImMin(clip_r.Max.x, marker_pos.x - (int) (marker_size_x * 0.5f));
		}
	}
	//if (g.IO.KeyShift) window->DrawList->AddRect(layout_r.Min, layout_r.Max, IM_COL32(255, 128, 0, 255)); // [DEBUG]
	//if (g.IO.KeyCtrl) window->DrawList->AddRect(clip_r.Min, clip_r.Max, IM_COL32(255, 128, 0, 255)); // [DEBUG]
	RenderTextClipped(layout_r.Min, layout_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_r);
}

bool UpdateWindowManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4], const ImRect& visibility_rect)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindowFlags flags = window->Flags;

	if ((flags & ImGuiWindowFlags_NoResize) || (flags & ImGuiWindowFlags_AlwaysAutoResize) || window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
		return false;
	if (window->WasActive == false) // Early out to avoid running this code for e.g. an hidden implicit/fallback Debug window.
		return false;

	bool ret_auto_fit = false;
	const int resize_border_count = g.IO.ConfigWindowsResizeFromEdges ? 4 : 0;
	const float grip_draw_size = IM_FLOOR(ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
	const float grip_hover_inner_size = IM_FLOOR(grip_draw_size * 0.75f);
	const float grip_hover_outer_size = g.IO.ConfigWindowsResizeFromEdges ? WINDOWS_HOVER_PADDING : 0.0f;

	ImVec2 pos_target(FLT_MAX, FLT_MAX);
	ImVec2 size_target(FLT_MAX, FLT_MAX);

	// Resize grips and borders are on layer 1
	window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

	// Manual resize grips
	PushID("#RESIZE");
	for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
	{
		const ImGuiResizeGripDef& def = resize_grip_def[resize_grip_n];
		const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, def.CornerPosN);

		// Using the FlattenChilds button flag we make the resize button accessible even if we are hovering over a child window
		bool hovered, held;
		ImRect resize_rect(corner - def.InnerDir * grip_hover_outer_size, corner + def.InnerDir * grip_hover_inner_size);
		if (resize_rect.Min.x > resize_rect.Max.x) ImSwap(resize_rect.Min.x, resize_rect.Max.x);
		if (resize_rect.Min.y > resize_rect.Max.y) ImSwap(resize_rect.Min.y, resize_rect.Max.y);
		ImGuiID resize_grip_id = window->GetID(resize_grip_n); // == GetWindowResizeCornerID()
		KeepAliveID(resize_grip_id);
		ButtonBehavior(resize_rect, resize_grip_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
		//GetForegroundDrawList(window)->AddRect(resize_rect.Min, resize_rect.Max, IM_COL32(255, 255, 0, 255));
		if (hovered || held)
			g.MouseCursor = (resize_grip_n & 1) ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;

		if (held && g.IO.MouseClickedCount[0] == 2 && resize_grip_n == 0)
		{
			// Manual auto-fit when double-clicking
			size_target = CalcWindowSizeAfterConstraint(window, size_auto_fit);
			ret_auto_fit = true;
			ClearActiveID( );
		}
		else if (held)
		{
			// Resize from any of the four corners
			// We don't use an incremental MouseDelta but rather compute an absolute target size based on mouse position
			ImVec2 clamp_min = ImVec2(def.CornerPosN.x == 1.0f ? visibility_rect.Min.x : -FLT_MAX, def.CornerPosN.y == 1.0f ? visibility_rect.Min.y : -FLT_MAX);
			ImVec2 clamp_max = ImVec2(def.CornerPosN.x == 0.0f ? visibility_rect.Max.x : +FLT_MAX, def.CornerPosN.y == 0.0f ? visibility_rect.Max.y : +FLT_MAX);
			ImVec2 corner_target = g.IO.MousePos - g.ActiveIdClickOffset + ImLerp(def.InnerDir * grip_hover_outer_size, def.InnerDir * -grip_hover_inner_size, def.CornerPosN); // Corner of the window corresponding to our corner grip
			corner_target = ImClamp(corner_target, clamp_min, clamp_max);
			CalcResizePosSizeFromAnyCorner(window, corner_target, def.CornerPosN, &pos_target, &size_target);
		}

		// Only lower-left grip is visible before hovering/activating
		if (resize_grip_n == 0 || held || hovered)
			resize_grip_col[resize_grip_n] = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);
	}
	for (int border_n = 0; border_n < resize_border_count; border_n++)
	{
		const ImGuiResizeBorderDef& def = resize_border_def[border_n];
		const ImGuiAxis axis = (border_n == ImGuiDir_Left || border_n == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;

		bool hovered, held;
		ImRect border_rect = GetResizeBorderRect(window, border_n, grip_hover_inner_size, WINDOWS_HOVER_PADDING);
		ImGuiID border_id = window->GetID(border_n + 4); // == GetWindowResizeBorderID()
		KeepAliveID(border_id);
		ButtonBehavior(border_rect, border_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
		//GetForegroundDrawLists(window)->AddRect(border_rect.Min, border_rect.Max, IM_COL32(255, 255, 0, 255));
		if ((hovered && g.HoveredIdTimer > WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER) || held)
		{
			g.MouseCursor = (axis == ImGuiAxis_X) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS;
			if (held)
				*border_held = border_n;
		}
		if (held)
		{
			ImVec2 clamp_min(border_n == ImGuiDir_Right ? visibility_rect.Min.x : -FLT_MAX, border_n == ImGuiDir_Down ? visibility_rect.Min.y : -FLT_MAX);
			ImVec2 clamp_max(border_n == ImGuiDir_Left ? visibility_rect.Max.x : +FLT_MAX, border_n == ImGuiDir_Up ? visibility_rect.Max.y : +FLT_MAX);
			ImVec2 border_target = window->Pos;
			border_target[axis] = g.IO.MousePos[axis] - g.ActiveIdClickOffset[axis] + WINDOWS_HOVER_PADDING;
			border_target = ImClamp(border_target, clamp_min, clamp_max);
			CalcResizePosSizeFromAnyCorner(window, border_target, ImMin(def.SegmentN1, def.SegmentN2), &pos_target, &size_target);
		}
	}
	PopID( );

	// Restore nav layer
	window->DC.NavLayerCurrent = ImGuiNavLayer_Main;

	// Navigation resize (keyboard/gamepad)
	if (g.NavWindowingTarget && g.NavWindowingTarget->RootWindow == window)
	{
		ImVec2 nav_resize_delta;
		if (g.NavInputSource == ImGuiInputSource_Keyboard && g.IO.KeyShift)
			nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_RawKeyboard, ImGuiNavReadMode_Down);
		if (g.NavInputSource == ImGuiInputSource_Gamepad)
			nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadDPad, ImGuiNavReadMode_Down);
		if (nav_resize_delta.x != 0.0f || nav_resize_delta.y != 0.0f)
		{
			const float NAV_RESIZE_SPEED = 600.0f;
			nav_resize_delta *= ImFloor(NAV_RESIZE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y));
			nav_resize_delta = ImMax(nav_resize_delta, visibility_rect.Min - window->Pos - window->Size);
			g.NavWindowingToggleLayer = false;
			g.NavDisableMouseHover = true;
			resize_grip_col[0] = GetColorU32(ImGuiCol_ResizeGripActive);
			// FIXME-NAV: Should store and accumulate into a separate size buffer to handle sizing constraints properly, right now a constraint will make us stuck.
			size_target = CalcWindowSizeAfterConstraint(window, window->SizeFull + nav_resize_delta);
		}
	}

	// Apply back modified position/size to window
	if (size_target.x != FLT_MAX)
	{
		window->SizeFull = size_target;
		MarkIniSettingsDirty(window);
	}
	if (pos_target.x != FLT_MAX)
	{
		window->Pos = ImFloor(pos_target);
		MarkIniSettingsDirty(window);
	}

	window->Size = window->SizeFull;
	return ret_auto_fit;
}

static void RenderWindowOuterBorders(ImGuiWindow* window)
{
	ImGuiContext& g = *GImGui;
	float rounding = window->WindowRounding;
	float border_size = window->WindowBorderSize;
	if (border_size > 0.0f && !(window->Flags & ImGuiWindowFlags_NoBackground))
		window->DrawList->AddRect(window->Pos, window->Pos + window->Size, GetColorU32(ImGuiCol_Border), rounding, 0, border_size);

	int border_held = window->ResizeBorderHeld;
	if (border_held != -1)
	{
		const ImGuiResizeBorderDef& def = resize_border_def[border_held];
		ImRect border_r = GetResizeBorderRect(window, border_held, rounding, 0.0f);
		window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.SegmentN1) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle - IM_PI * 0.25f, def.OuterAngle);
		window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.SegmentN2) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle, def.OuterAngle + IM_PI * 0.25f);
		window->DrawList->PathStroke(GetColorU32(ImGuiCol_SeparatorActive), 0, ImMax(2.0f, border_size)); // Thicker than usual
	}
	if (g.Style.FrameBorderSize > 0 && !(window->Flags & ImGuiWindowFlags_NoTitleBar))
	{
		float y = window->Pos.y + window->TitleBarHeight( ) - 1;
		window->DrawList->AddLine(ImVec2(window->Pos.x + border_size, y), ImVec2(window->Pos.x + window->Size.x - border_size, y), GetColorU32(ImGuiCol_Border), g.Style.FrameBorderSize);
	}
}

void RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size)
{
	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = g.Style;
	ImGuiWindowFlags flags = window->Flags;

	// Ensure that ScrollBar doesn't read last frame's SkipItems
	IM_ASSERT(window->BeginCount == 0);
	window->SkipItems = false;

	// Draw window + handle manual resize
	// As we highlight the title bar when want_focus is set, multiple reappearing windows will have have their title bar highlighted on their reappearing frame.
	const float window_rounding = window->WindowRounding;
	const float window_border_size = window->WindowBorderSize;
	if (window->Collapsed)
	{
		// Title bar only
		float backup_border_size = style.FrameBorderSize;
		g.Style.FrameBorderSize = window->WindowBorderSize;
		ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
		RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
		g.Style.FrameBorderSize = backup_border_size;
	}
	else
	{
		// Window background
		if (!(flags & ImGuiWindowFlags_NoBackground))
		{
			ImU32 bg_col = GetColorU32(GetWindowBgColorIdx(window));
			bool override_alpha = false;
			float alpha = 1.0f;
			if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasBgAlpha)
			{
				alpha = g.NextWindowData.BgAlphaVal;
				override_alpha = true;
			}
			if (override_alpha)
				bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
			window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight( )), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? 0 : ImDrawFlags_RoundCornersBottom);
		}

		// Title bar
		if (!(flags & ImGuiWindowFlags_NoTitleBar))
		{
			ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
			window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawFlags_RoundCornersTop);
		}

		// Menu bar
		if (flags & ImGuiWindowFlags_MenuBar)
		{
			ImRect menu_bar_rect = window->MenuBarRect( );
			menu_bar_rect.ClipWith(window->Rect( ));  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
			window->DrawList->AddRectFilled(menu_bar_rect.Min + ImVec2(window_border_size, 0), menu_bar_rect.Max - ImVec2(window_border_size, 0), GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawFlags_RoundCornersTop);
			if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
				window->DrawList->AddLine(menu_bar_rect.GetBL( ), menu_bar_rect.GetBR( ), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
		}

		// Scrollbars
		if (window->ScrollbarX)
			scrollbar(ImGuiAxis_X);
		if (window->ScrollbarY)
			scrollbar(ImGuiAxis_Y);

		// Render resize grips (after their input handling so we don't have a frame of latency)
		if (!(flags & ImGuiWindowFlags_NoResize))
		{
			for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
			{
				const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
				const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPosN);
				window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(window_border_size, resize_grip_draw_size) : ImVec2(resize_grip_draw_size, window_border_size)));
				window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(resize_grip_draw_size, window_border_size) : ImVec2(window_border_size, resize_grip_draw_size)));
				window->DrawList->PathArcToFast(ImVec2(corner.x + grip.InnerDir.x * (window_rounding + window_border_size), corner.y + grip.InnerDir.y * (window_rounding + window_border_size)), window_rounding, grip.AngleMin12, grip.AngleMax12);
				window->DrawList->PathFillConvex(resize_grip_col[resize_grip_n]);
			}
		}

		// Borders
		RenderWindowOuterBorders(window);
	}
}

bool c_oink_ui::begin_window(const char* name, bool* p_open, ImGuiWindowFlags flags, bool p)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	IM_ASSERT(name != NULL && name[0] != '\0');     // Window name required
	IM_ASSERT(g.WithinFrameScope);                  // Forgot to call ImGui::NewFrame()
	IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

	// Find or create
	ImGuiWindow* window = FindWindowByName(name);
	const bool window_just_created = (window == NULL);
	if (window_just_created)
		window = CreateNewWindow(name, flags);
	else
		UpdateWindowInFocusOrderList(window, window_just_created, flags);

	// Automatically disable manual moving/resizing when NoInputs is set
	if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
		flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	if (flags & ImGuiWindowFlags_NavFlattened)
		IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

	const int current_frame = g.FrameCount;
	const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);
	window->IsFallbackWindow = (g.CurrentWindowStack.Size == 0 && g.WithinFrameScopeWithImplicitWindow);

	// Update the Appearing flag
	bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
		window_just_activated_by_user |= (window != popup_ref.Window);
	}
	window->Appearing = window_just_activated_by_user;
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

	// Update Flags, LastFrameActive, BeginOrderXXX fields
	if (first_begin_of_the_frame)
	{
		window->Flags = (ImGuiWindowFlags) flags;
		window->LastFrameActive = current_frame;
		window->LastTimeActive = (float) g.Time;
		window->BeginOrderWithinParent = 0;
		window->BeginOrderWithinContext = (short) (g.WindowsActiveCount++);
	}
	else
	{
		flags = window->Flags;
	}

	// Parent window is latched only on the first call to Begin() of the frame, so further append-calls can be done from a different window stack
	ImGuiWindow* parent_window_in_stack = g.CurrentWindowStack.empty( ) ? NULL : g.CurrentWindowStack.back( ).Window;
	ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
	IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));

	// We allow window memory to be compacted so recreate the base stack when needed.
	if (window->IDStack.Size == 0)
		window->IDStack.push_back(window->ID);

	// Add to stack
	// We intentionally set g.CurrentWindow to NULL to prevent usage until when the viewport is set, then will call SetCurrentWindow()
	g.CurrentWindow = window;
	ImGuiWindowStackData window_stack_data;
	window_stack_data.Window = window;
	window_stack_data.ParentLastItemDataBackup = g.LastItemData;
	window_stack_data.StackSizesOnBegin.SetToCurrentState( );
	g.CurrentWindowStack.push_back(window_stack_data);
	g.CurrentWindow = NULL;
	if (flags & ImGuiWindowFlags_ChildMenu)
		g.BeginMenuCount++;

	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		popup_ref.Window = window;
		g.BeginPopupStack.push_back(popup_ref);
		window->PopupId = popup_ref.PopupId;
	}

	// Update ->RootWindow and others pointers (before any possible call to FocusWindow)
	if (first_begin_of_the_frame)
	{
		UpdateWindowParentAndRootLinks(window, flags, parent_window);
		window->ParentWindowInBeginStack = parent_window_in_stack;
	}

	// Process SetNextWindow***() calls
	// (FIXME: Consider splitting the HasXXX flags into X/Y components
	bool window_pos_set_by_api = false;
	bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
	if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos)
	{
		window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
		if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f)
		{
			// May be processed on the next frame if this is our first frame and we are measuring size
			// FIXME: Look into removing the branch so everything can go through this same code path for consistency.
			window->SetWindowPosVal = g.NextWindowData.PosVal;
			window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
			window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
		}
		else
		{
			SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
		}
	}
	if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize)
	{
		window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
		window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
		SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
	}
	if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasScroll)
	{
		if (g.NextWindowData.ScrollVal.x >= 0.0f)
		{
			window->ScrollTarget.x = g.NextWindowData.ScrollVal.x;
			window->ScrollTargetCenterRatio.x = 0.0f;
		}
		if (g.NextWindowData.ScrollVal.y >= 0.0f)
		{
			window->ScrollTarget.y = g.NextWindowData.ScrollVal.y;
			window->ScrollTargetCenterRatio.y = 0.0f;
		}
	}
	if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasContentSize)
		window->ContentSizeExplicit = g.NextWindowData.ContentSizeVal;
	else if (first_begin_of_the_frame)
		window->ContentSizeExplicit = ImVec2(0.0f, 0.0f);
	if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasCollapsed)
		SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
	if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasFocus)
		FocusWindow(window);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

	// When reusing window again multiple times a frame, just append content (don't need to setup again)
	if (first_begin_of_the_frame)
	{
		// Initialize
		const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip); // FIXME-WIP: Undocumented behavior of Child+Tooltip for pinned tooltip (#1345)
		const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesCannotSkipItems > 0);
		window->Active = true;
		window->HasCloseButton = (p_open != NULL);
		window->ClipRect = ImVec4(-FLT_MAX, -FLT_MAX, +FLT_MAX, +FLT_MAX);
		window->IDStack.resize(1);
		window->DrawList->_ResetForNewFrame( );
		window->DC.CurrentTableIdx = -1;

		// Restore buffer capacity when woken from a compacted state, to avoid
		if (window->MemoryCompacted)
			GcAwakeTransientWindowBuffers(window);

		// Update stored window name when it changes (which can _only_ happen with the "###" operator, so the ID would stay unchanged).
		// The title bar always display the 'name' parameter, so we only update the string storage if it needs to be visible to the end-user elsewhere.
		bool window_title_visible_elsewhere = false;
		if (g.NavWindowingListWindow != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)   // Window titles visible when using CTRL+TAB
			window_title_visible_elsewhere = true;
		if (window_title_visible_elsewhere && !window_just_created && strcmp(name, window->Name) != 0)
		{
			size_t buf_len = (size_t) window->NameBufLen;
			window->Name = ImStrdupcpy(window->Name, &buf_len, name);
			window->NameBufLen = (int) buf_len;
		}

		// UPDATE CONTENTS SIZE, UPDATE HIDDEN STATUS

		// Update contents size from last frame for auto-fitting (or use explicit size)
		CalcWindowContentSizes(window, &window->ContentSize, &window->ContentSizeIdeal);
		if (window->HiddenFramesCanSkipItems > 0)
			window->HiddenFramesCanSkipItems--;
		if (window->HiddenFramesCannotSkipItems > 0)
			window->HiddenFramesCannotSkipItems--;
		if (window->HiddenFramesForRenderOnly > 0)
			window->HiddenFramesForRenderOnly--;

		// Hide new windows for one frame until they calculate their size
		if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
			window->HiddenFramesCannotSkipItems = 1;

		// Hide popup/tooltip window when re-opening while we measure size (because we recycle the windows)
		// We reset Size/ContentSize for reappearing popups/tooltips early in this function, so further code won't be tempted to use the old size.
		if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
		{
			window->HiddenFramesCannotSkipItems = 1;
			if (flags & ImGuiWindowFlags_AlwaysAutoResize)
			{
				if (!window_size_x_set_by_api)
					window->Size.x = window->SizeFull.x = 0.f;
				if (!window_size_y_set_by_api)
					window->Size.y = window->SizeFull.y = 0.f;
				window->ContentSize = window->ContentSizeIdeal = ImVec2(0.f, 0.f);
			}
		}

		// SELECT VIEWPORT
		// FIXME-VIEWPORT: In the docking/viewport branch, this is the point where we select the current viewport (which may affect the style)

		ImGuiViewportP* viewport = (ImGuiViewportP*) (void*) GetMainViewport( );
		SetWindowViewport(window, viewport);
		SetCurrentWindow(window);

		// LOCK BORDER SIZE AND PADDING FOR THE FRAME (so that altering them doesn't cause inconsistencies)

		if (flags & ImGuiWindowFlags_ChildWindow)
			window->WindowBorderSize = style.ChildBorderSize;
		else
			window->WindowBorderSize = ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
		window->WindowPadding = style.WindowPadding;
		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Popup)) && window->WindowBorderSize == 0.0f)
			window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);

		// Lock menu offset so size calculation can use it as menu-bar windows need a minimum size.
		window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
		window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

		// Collapse window by double-clicking on title bar
		// At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
		if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse))
		{
			// We don't use a regular button+id to test for double-click on title bar (mostly due to legacy reason, could be fixed), so verify that we don't have items over the title bar.
			ImRect title_bar_rect = window->TitleBarRect( );
			if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseClickedCount[0] == 2)
				window->WantCollapseToggle = true;
			if (window->WantCollapseToggle)
			{
				window->Collapsed = !window->Collapsed;
				MarkIniSettingsDirty(window);
			}
		}
		else
		{
			window->Collapsed = false;
		}
		window->WantCollapseToggle = false;

		// SIZE

		// Calculate auto-fit size, handle automatic resize
		const ImVec2 size_auto_fit = CalcWindowAutoFitSize(window, window->ContentSizeIdeal);
		bool use_current_size_for_scrollbar_x = window_just_created;
		bool use_current_size_for_scrollbar_y = window_just_created;
		if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
		{
			// Using SetNextWindowSize() overrides ImGuiWindowFlags_AlwaysAutoResize, so it can be used on tooltips/popups, etc.
			if (!window_size_x_set_by_api)
			{
				window->SizeFull.x = size_auto_fit.x;
				use_current_size_for_scrollbar_x = true;
			}
			if (!window_size_y_set_by_api)
			{
				window->SizeFull.y = size_auto_fit.y;
				use_current_size_for_scrollbar_y = true;
			}
		}
		else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
		{
			// Auto-fit may only grow window during the first few frames
			// We still process initial auto-fit on collapsed windows to get a window width, but otherwise don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
			if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
			{
				window->SizeFull.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
				use_current_size_for_scrollbar_x = true;
			}
			if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
			{
				window->SizeFull.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
				use_current_size_for_scrollbar_y = true;
			}
			if (!window->Collapsed)
				MarkIniSettingsDirty(window);
		}

		// Apply minimum/maximum window size constraints and final size
		window->SizeFull = CalcWindowSizeAfterConstraint(window, window->SizeFull);
		window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect( ).GetSize( ) : window->SizeFull;

		// Decoration size
		const float decoration_up_height = window->TitleBarHeight( ) + window->MenuBarHeight( );

		// POSITION

		// Popup latch its initial position, will position itself when it appears next frame
		if (window_just_activated_by_user)
		{
			window->AutoPosLastDirection = ImGuiDir_None;
			if ((flags & ImGuiWindowFlags_Popup) != 0 && !(flags & ImGuiWindowFlags_Modal) && !window_pos_set_by_api) // FIXME: BeginPopup() could use SetNextWindowPos()
				window->Pos = g.BeginPopupStack.back( ).OpenPopupPos;
		}

		// Position child window
		if (flags & ImGuiWindowFlags_ChildWindow)
		{
			IM_ASSERT(parent_window && parent_window->Active);
			window->BeginOrderWithinParent = (short) parent_window->DC.ChildWindows.Size;
			parent_window->DC.ChildWindows.push_back(window);
			if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
				window->Pos = parent_window->DC.CursorPos;
		}

		const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesCannotSkipItems == 0);
		if (window_pos_with_pivot)
			SetWindowPos(window, window->SetWindowPosVal - window->Size * window->SetWindowPosPivot, 0); // Position given a pivot (e.g. for centering)
		else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
			window->Pos = FindBestWindowPosForPopup(window);

		// Calculate the range of allowed position for that window (to be movable and visible past safe area padding)
		// When clamping to stay visible, we will enforce that window->Pos stays inside of visibility_rect.
		ImRect viewport_rect(viewport->GetMainRect( ));
		ImRect viewport_work_rect(viewport->GetWorkRect( ));
		ImVec2 visibility_padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
		ImRect visibility_rect(viewport_work_rect.Min + visibility_padding, viewport_work_rect.Max - visibility_padding);

		// Clamp position/size so window stays visible within its viewport or monitor
		// Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
		if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
			if (viewport_rect.GetWidth( ) > 0.0f && viewport_rect.GetHeight( ) > 0.0f)
				ClampWindowRect(window, visibility_rect);
		window->Pos = ImFloor(window->Pos);

		// Lock window rounding for the frame (so that altering them doesn't cause inconsistencies)
		// Large values tend to lead to variety of artifacts and are not recommended.
		window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

		// For windows with title bar or menu bar, we clamp to FrameHeight(FontSize + FramePadding.y * 2.0f) to completely hide artifacts.
		//if ((window->Flags & ImGuiWindowFlags_MenuBar) || !(window->Flags & ImGuiWindowFlags_NoTitleBar))
		//    window->WindowRounding = ImMin(window->WindowRounding, g.FontSize + style.FramePadding.y * 2.0f);

		// Apply window focus (new and reactivated windows are moved to front)
		bool want_focus = false;
		if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (flags & ImGuiWindowFlags_Popup)
				want_focus = true;
			else if ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) == 0)
				want_focus = true;

			ImGuiWindow* modal = GetTopMostPopupModal( );
			if (modal != NULL && !IsWindowWithinBeginStackOf(window, modal))
			{
				// Avoid focusing a window that is created outside of active modal. This will prevent active modal from being closed.
				// Since window is not focused it would reappear at the same display position like the last time it was visible.
				// In case of completely new windows it would go to the top (over current modal), but input to such window would still be blocked by modal.
				// Position window behind a modal that is not a begin-parent of this window.
				want_focus = false;
				if (window == window->RootWindow)
				{
					ImGuiWindow* blocking_modal = FindBlockingModal(window);
					IM_ASSERT(blocking_modal != NULL);
					BringWindowToDisplayBehind(window, blocking_modal);
				}
			}
		}

		// [Test Engine] Register whole window in the item system
#ifdef IMGUI_ENABLE_TEST_ENGINE
		if (g.TestEngineHookItems)
		{
			IM_ASSERT(window->IDStack.Size == 1);
			window->IDStack.Size = 0;
			IMGUI_TEST_ENGINE_ITEM_ADD(window->Rect( ), window->ID);
			IMGUI_TEST_ENGINE_ITEM_INFO(window->ID, window->Name, (g.HoveredWindow == window) ? ImGuiItemStatusFlags_HoveredRect : 0);
			window->IDStack.Size = 1;
		}
#endif

		// Handle manual resize: Resize Grips, Borders, Gamepad
		int border_held = -1;
		ImU32 resize_grip_col[4] = {};
		const int resize_grip_count = g.IO.ConfigWindowsResizeFromEdges ? 2 : 1; // Allow resize from lower-left if we have the mouse cursor feedback for it.
		const float resize_grip_draw_size = IM_FLOOR(ImMax(g.FontSize * 1.10f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
		if (!window->Collapsed)
			if (UpdateWindowManualResize(window, size_auto_fit, &border_held, resize_grip_count, &resize_grip_col[0], visibility_rect))
				use_current_size_for_scrollbar_x = use_current_size_for_scrollbar_y = true;
		window->ResizeBorderHeld = (signed char) border_held;

		// SCROLLBAR VISIBILITY

		// Update scrollbar visibility (based on the Size that was effective during last frame or the auto-resized Size).
		if (!window->Collapsed)
		{
			// When reading the current size we need to read it after size constraints have been applied.
			// When we use InnerRect here we are intentionally reading last frame size, same for ScrollbarSizes values before we set them again.
			ImVec2 avail_size_from_current_frame = ImVec2(window->SizeFull.x, window->SizeFull.y - decoration_up_height);
			ImVec2 avail_size_from_last_frame = window->InnerRect.GetSize( ) + window->ScrollbarSizes;
			ImVec2 needed_size_from_last_frame = window_just_created ? ImVec2(0, 0) : window->ContentSize + window->WindowPadding * 2.0f;
			float size_x_for_scrollbars = use_current_size_for_scrollbar_x ? avail_size_from_current_frame.x : avail_size_from_last_frame.x;
			float size_y_for_scrollbars = use_current_size_for_scrollbar_y ? avail_size_from_current_frame.y : avail_size_from_last_frame.y;
			//bool scrollbar_y_from_last_frame = window->ScrollbarY; // FIXME: May want to use that in the ScrollbarX expression? How many pros vs cons?
			window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
			window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((needed_size_from_last_frame.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
			if (window->ScrollbarX && !window->ScrollbarY)
				window->ScrollbarY = (needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar);
			window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
		}

		// UPDATE RECTANGLES (1- THOSE NOT AFFECTED BY SCROLLING)
		// Update various regions. Variables they depends on should be set above in this function.
		// We set this up after processing the resize grip so that our rectangles doesn't lag by a frame.

		// Outer rectangle
		// Not affected by window border size. Used by:
		// - FindHoveredWindow() (w/ extra padding when border resize is enabled)
		// - Begin() initial clipping rect for drawing window background and borders.
		// - Begin() clipping whole child
		const ImRect host_rect = ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip) ? parent_window->ClipRect : viewport_rect;
		const ImRect outer_rect = window->Rect( );
		const ImRect title_bar_rect = window->TitleBarRect( );
		window->OuterRectClipped = outer_rect;
		window->OuterRectClipped.ClipWith(host_rect);

		// Inner rectangle
		// Not affected by window border size. Used by:
		// - InnerClipRect
		// - ScrollToRectEx()
		// - NavUpdatePageUpPageDown()
		// - Scrollbar()
		window->InnerRect.Min.x = window->Pos.x;
		window->InnerRect.Min.y = window->Pos.y + decoration_up_height;
		window->InnerRect.Max.x = window->Pos.x + window->Size.x - window->ScrollbarSizes.x;
		window->InnerRect.Max.y = window->Pos.y + window->Size.y - window->ScrollbarSizes.y;

		// Inner clipping rectangle.
		// Will extend a little bit outside the normal work region.
		// This is to allow e.g. Selectable or CollapsingHeader or some separators to cover that space.
		// Force round operator last to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
		// Note that if our window is collapsed we will end up with an inverted (~null) clipping rectangle which is the correct behavior.
		// Affected by window/frame border size. Used by:
		// - Begin() initial clip rect
		float top_border_size = (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
		window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerRect.Min.x + ImMax(ImFloor(window->WindowPadding.x * 0.5f), window->WindowBorderSize));
		window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerRect.Min.y + top_border_size);
		window->InnerClipRect.Max.x = ImFloor(0.5f + window->InnerRect.Max.x - ImMax(ImFloor(window->WindowPadding.x * 0.5f), window->WindowBorderSize));
		window->InnerClipRect.Max.y = ImFloor(0.5f + window->InnerRect.Max.y - window->WindowBorderSize);
		window->InnerClipRect.ClipWithFull(host_rect);

		// Default item width. Make it proportional to window size if window manually resizes
		if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
			window->ItemWidthDefault = ImFloor(window->Size.x * 0.65f);
		else
			window->ItemWidthDefault = ImFloor(g.FontSize * 16.0f);

		// SCROLLING

		// Lock down maximum scrolling
		// The value of ScrollMax are ahead from ScrollbarX/ScrollbarY which is intentionally using InnerRect from previous rect in order to accommodate
		// for right/bottom aligned items without creating a scrollbar.
		window->ScrollMax.x = ImMax(0.0f, window->ContentSize.x + window->WindowPadding.x * 2.0f - window->InnerRect.GetWidth( ));
		window->ScrollMax.y = ImMax(0.0f, window->ContentSize.y + window->WindowPadding.y * 2.0f - window->InnerRect.GetHeight( ));

		// Apply scrolling
		float needed_scroll = CalcNextScrollFromScrollTargetAndClamp(window).y;
		window->ScrollTarget = ImVec2(window->ScrollTarget.x, window->ScrollTarget.y + g.NextWindowData.ScrollVal.y);

		const ImGuiID id = window->GetID(name);

		static std::map<ImGuiID, float> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end( ))
		{
			anim.insert({ id, 0.f });
			it_anim = anim.find(id);
		}

		it_anim->second = ImLerp(it_anim->second, needed_scroll, g.IO.DeltaTime * 13.f);

		if (!ImGui::IsMouseDown(0) || !ImGui::IsMouseHoveringRect(window->Pos + ImVec2(window->Size.x - ImGui::GetStyle( ).ScrollbarSize, 0), window->Pos + window->Size))
		{
			if (window->Scroll.y != needed_scroll)
			{
				window->Scroll.y = it_anim->second;
			}
		}
		else
		{
			window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window);
			window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
		}

		// DRAWING

		// Setup draw list and outer clipping rectangle
		IM_ASSERT(window->DrawList->CmdBuffer.Size == 1 && window->DrawList->CmdBuffer[0].ElemCount == 0);
		window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
		PushClipRect(host_rect.Min, host_rect.Max, false);

		// Child windows can render their decoration (bg color, border, scrollbars, etc.) within their parent to save a draw call (since 1.71)
		// When using overlapping child windows, this will break the assumption that child z-order is mapped to submission order.
		// FIXME: User code may rely on explicit sorting of overlapping child window and would need to disable this somehow. Please get in contact if you are affected (github #4493)
		{
			bool render_decorations_in_parent = false;
			if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
			{
				// - We test overlap with the previous child window only (testing all would end up being O(log N) not a good investment here)
				// - We disable this when the parent window has zero vertices, which is a common pattern leading to laying out multiple overlapping childs
				ImGuiWindow* previous_child = parent_window->DC.ChildWindows.Size >= 2 ? parent_window->DC.ChildWindows[parent_window->DC.ChildWindows.Size - 2] : NULL;
				bool previous_child_overlapping = previous_child ? previous_child->Rect( ).Overlaps(window->Rect( )) : false;
				bool parent_is_empty = parent_window->DrawList->VtxBuffer.Size > 0;
				if (window->DrawList->CmdBuffer.back( ).ElemCount == 0 && parent_is_empty && !previous_child_overlapping)
					render_decorations_in_parent = true;

				window->DrawList->AddRectFilled(window->Pos + ImVec2(0, 0), window->Pos + ImVec2(window->Size.x, window->Size.y),
													ImColor(0, 0, 0));

				window->DrawList->AddRect(window->Pos + ImVec2(0, 0), window->Pos + ImVec2(window->Size.x, window->Size.y),
											  ImColor(m_theme_colour_primary));

			}
			if (render_decorations_in_parent)
				window->DrawList = parent_window->DrawList;


			// Handle title bar, scrollbar, resize grips and resize borders
			const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
			const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
			RenderWindowDecorations(window, title_bar_rect, title_bar_is_highlight, resize_grip_count, resize_grip_col, resize_grip_draw_size);

			if (render_decorations_in_parent)
				window->DrawList = &window->DrawListInst;
		}

		// UPDATE RECTANGLES (2- THOSE AFFECTED BY SCROLLING)

		// Work rectangle.
		// Affected by window padding and border size. Used by:
		// - Columns() for right-most edge
		// - TreeNode(), CollapsingHeader() for right-most edge
		// - BeginTabBar() for right-most edge
		const bool allow_scrollbar_x = !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar);
		const bool allow_scrollbar_y = !(flags & ImGuiWindowFlags_NoScrollbar);
		const float work_rect_size_x = (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : ImMax(allow_scrollbar_x ? window->ContentSize.x : 0.0f, window->Size.x - window->WindowPadding.x * 2.0f - window->ScrollbarSizes.x));
		const float work_rect_size_y = (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : ImMax(allow_scrollbar_y ? window->ContentSize.y : 0.0f, window->Size.y - window->WindowPadding.y * 2.0f - decoration_up_height - window->ScrollbarSizes.y));
		window->WorkRect.Min.x = ImFloor(window->InnerRect.Min.x - window->Scroll.x + ImMax(window->WindowPadding.x, window->WindowBorderSize));
		window->WorkRect.Min.y = ImFloor(window->InnerRect.Min.y - window->Scroll.y + ImMax(window->WindowPadding.y, window->WindowBorderSize));
		window->WorkRect.Max.x = window->WorkRect.Min.x + work_rect_size_x;
		window->WorkRect.Max.y = window->WorkRect.Min.y + work_rect_size_y;
		window->ParentWorkRect = window->WorkRect;

		// [LEGACY] Content Region
		// FIXME-OBSOLETE: window->ContentRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
		// Used by:
		// - Mouse wheel scrolling + many other things
		window->ContentRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x;
		window->ContentRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + decoration_up_height;
		window->ContentRegionRect.Max.x = window->ContentRegionRect.Min.x + (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : (window->Size.x - window->WindowPadding.x * 2.0f - window->ScrollbarSizes.x));
		window->ContentRegionRect.Max.y = window->ContentRegionRect.Min.y + (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : (window->Size.y - window->WindowPadding.y * 2.0f - decoration_up_height - window->ScrollbarSizes.y));

		// Setup drawing context
		// (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
		window->DC.Indent.x = 0.0f + window->WindowPadding.x - window->Scroll.x;
		window->DC.GroupOffset.x = 0.0f;
		window->DC.ColumnsOffset.x = 0.0f;

		// Record the loss of precision of CursorStartPos which can happen due to really large scrolling amount.
		// This is used by clipper to compensate and fix the most common use case of large scroll area. Easy and cheap, next best thing compared to switching everything to double or ImU64.
		double start_pos_highp_x = (double) window->Pos.x + window->WindowPadding.x - (double) window->Scroll.x + window->DC.ColumnsOffset.x;
		double start_pos_highp_y = (double) window->Pos.y + window->WindowPadding.y - (double) window->Scroll.y + decoration_up_height;
		window->DC.CursorStartPos = ImVec2((float) start_pos_highp_x, (float) start_pos_highp_y);
		window->DC.CursorStartPosLossyness = ImVec2((float) (start_pos_highp_x - window->DC.CursorStartPos.x), (float) (start_pos_highp_y - window->DC.CursorStartPos.y));
		window->DC.CursorPos = window->DC.CursorStartPos;
		window->DC.CursorPosPrevLine = window->DC.CursorPos;
		window->DC.CursorMaxPos = window->DC.CursorStartPos;
		window->DC.IdealMaxPos = window->DC.CursorStartPos;
		window->DC.CurrLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
		window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
		window->DC.IsSameLine = false;

		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.NavLayersActiveMask = window->DC.NavLayersActiveMaskNext;
		window->DC.NavLayersActiveMaskNext = 0x00;
		window->DC.NavHideHighlightOneFrame = false;
		window->DC.NavHasScroll = (window->ScrollMax.y > 0.0f);

		window->DC.MenuBarAppending = false;
		window->DC.MenuColumns.Update(style.ItemSpacing.x, window_just_activated_by_user);
		window->DC.TreeDepth = 0;
		window->DC.TreeJumpToParentOnPopMask = 0x00;
		window->DC.ChildWindows.resize(0);
		window->DC.StateStorage = &window->StateStorage;
		window->DC.CurrentColumns = NULL;
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;

		window->DC.ItemWidth = window->ItemWidthDefault;
		window->DC.TextWrapPos = -1.0f; // disabled
		window->DC.ItemWidthStack.resize(0);
		window->DC.TextWrapPosStack.resize(0);

		if (window->AutoFitFramesX > 0)
			window->AutoFitFramesX--;
		if (window->AutoFitFramesY > 0)
			window->AutoFitFramesY--;

		// Apply focus (we need to call FocusWindow() AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
		if (want_focus)
		{
			FocusWindow(window);
			NavInitWindow(window, false); // <-- this is in the way for us to be able to defer and sort reappearing FocusWindow() calls
		}

		// Title bar
		if (!(flags & ImGuiWindowFlags_NoTitleBar))
			RenderWindowTitleBarContents(window, ImRect(title_bar_rect.Min.x + window->WindowBorderSize, title_bar_rect.Min.y, title_bar_rect.Max.x - window->WindowBorderSize, title_bar_rect.Max.y), name, p_open);

		// Clear hit test shape every frame
		window->HitTestHoleSize.x = window->HitTestHoleSize.y = 0;

		// Pressing CTRL+C while holding on a window copy its content to the clipboard
		// This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
		// Maybe we can support CTRL+C on every element?
		/*
		//if (g.NavWindow == window && g.ActiveId == 0)
		if (g.ActiveId == window->MoveId)
			if (g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_C))
				LogToClipboard();
		*/

		// We fill last item data based on Title Bar/Tab, in order for IsItemHovered() and IsItemActive() to be usable after Begin().
		// This is useful to allow creating context menus on title bar only, etc.
		SetLastItemData(window->MoveId, g.CurrentItemFlags, IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0, title_bar_rect);

		// [Test Engine] Register title bar / tab
		if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
			IMGUI_TEST_ENGINE_ITEM_ADD(g.LastItemData.Rect, g.LastItemData.ID);
	}
	else
	{
		// Append
		SetCurrentWindow(window);
	}

	// Pull/inherit current state
	window->DC.NavFocusScopeIdCurrent = (flags & ImGuiWindowFlags_ChildWindow) ? parent_window->DC.NavFocusScopeIdCurrent : window->GetID("#FOCUSSCOPE"); // Inherit from parent only // -V595

	PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

	// Clear 'accessed' flag last thing (After PushClipRect which will set the flag. We want the flag to stay false when the default "Debug" window is unused)
	window->WriteAccessed = false;
	window->BeginCount++;
	g.NextWindowData.ClearFlags( );

	// Update visibility
	if (first_begin_of_the_frame)
	{
		if (flags & ImGuiWindowFlags_ChildWindow)
		{
			// Child window can be out of sight and have "negative" clip windows.
			// Mark them as collapsed so commands are skipped earlier (we can't manually collapse them because they have no title bar).
			IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
			if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0) // FIXME: Doesn't make sense for ChildWindow??
			{
				const bool nav_request = (flags & ImGuiWindowFlags_NavFlattened) && (g.NavAnyRequest && g.NavWindow && g.NavWindow->RootWindowForNav == window->RootWindowForNav);
				if (!g.LogEnabled && !nav_request)
					if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
						window->HiddenFramesCanSkipItems = 1;
			}

			// Hide along with parent or if parent is collapsed
			if (parent_window && (parent_window->Collapsed || parent_window->HiddenFramesCanSkipItems > 0))
				window->HiddenFramesCanSkipItems = 1;
			if (parent_window && (parent_window->Collapsed || parent_window->HiddenFramesCannotSkipItems > 0))
				window->HiddenFramesCannotSkipItems = 1;
		}

		// Don't render if style alpha is 0.0 at the time of Begin(). This is arbitrary and inconsistent but has been there for a long while (may remove at some point)
		if (style.Alpha <= 0.0f)
			window->HiddenFramesCanSkipItems = 1;

		// Update the Hidden flag
		bool hidden_regular = (window->HiddenFramesCanSkipItems > 0) || (window->HiddenFramesCannotSkipItems > 0);
		window->Hidden = hidden_regular || (window->HiddenFramesForRenderOnly > 0);

		// Disable inputs for requested number of frames
		if (window->DisableInputsFrames > 0)
		{
			window->DisableInputsFrames--;
			window->Flags |= ImGuiWindowFlags_NoInputs;
		}

		// Update the SkipItems flag, used to early out of all items functions (no layout required)
		bool skip_items = false;
		if (window->Collapsed || !window->Active || hidden_regular)
			if (window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesCannotSkipItems <= 0)
				skip_items = true;
		window->SkipItems = skip_items;
	}

	return !window->SkipItems;
}

bool c_oink_ui::begin_child_ex(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* parent_window = g.CurrentWindow;

	flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
	flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag

	// Size
	const ImVec2 content_avail = GetContentRegionAvail( );
	ImVec2 size = ImFloor(size_arg);
	const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
	if (size.x <= 0.0f)
		size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
	if (size.y <= 0.0f)
		size.y = ImMax(content_avail.y + size.y, 4.0f);
	SetNextWindowSize(size);

	// Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
	if (name)
		ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%s/%s_%08X", parent_window->Name, name, id);
	else
		ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%s/%08X", parent_window->Name, id);

	const float backup_border_size = g.Style.ChildBorderSize;
	if (!border)
		g.Style.ChildBorderSize = 0.0f;
	bool ret = begin_window(name, NULL, flags);
	g.Style.ChildBorderSize = backup_border_size;

	ImGuiWindow* child_window = g.CurrentWindow;
	child_window->ChildId = id;
	child_window->AutoFitChildAxises = (ImS8) auto_fit_axises;

	// Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
	// While this is not really documented/defined, it seems that the expected thing to do.
	if (child_window->BeginCount == 1)
		parent_window->DC.CursorPos = child_window->Pos;

	// Process navigation-in immediately so NavInit can run on first frame
	if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavHasScroll))
	{
		FocusWindow(child_window);
		NavInitWindow(child_window, false);
		SetActiveID(id + 1, child_window); // Steal ActiveId with another arbitrary id so that key-press won't activate child item
		g.ActiveIdSource = ImGuiInputSource_Nav;
	}
	return ret;
}

bool c_oink_ui::create_child(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
	ImGuiWindow* window = GetCurrentWindow( );
	return begin_child_ex(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}

bool c_oink_ui::begin_child(const char* label, int number_of_child)
{
	constexpr int child_x_size_const = 206;

	ImGui::SetCursorPos(ImVec2(10.f * m_dpi_scaling + ((child_x_size_const * m_dpi_scaling + 10.f * m_dpi_scaling) * (number_of_child - 1)), 105.f * m_dpi_scaling));

	ImGuiStyle& style = ImGui::GetStyle( );

	if (create_child(label, ImVec2(child_x_size_const * m_dpi_scaling, 393.f * m_dpi_scaling), 0, 0))
	{
		style.ItemSpacing = ImVec2(10.f * m_dpi_scaling, 5.f * m_dpi_scaling);
		ImGui::SetCursorPos(ImVec2(10.f * m_dpi_scaling, 10.f * m_dpi_scaling));

		text_colored(label, m_theme_colour_secondary);

		ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(m_theme_colour_primary));
		ImGui::Separator( );
		ImGui::PopStyleColor( );

		ImGui::SetCursorPosY(ImGui::GetCursorPosY( ) + 2.f * m_dpi_scaling);
		return true;
	};

	return false;
}

void c_oink_ui::end_child( )
{
	ImGuiStyle& style = ImGui::GetStyle( );
	style.ItemSpacing = ImVec2(20.f * m_dpi_scaling, 0.f * m_dpi_scaling);
	ImGui::Dummy(ImVec2(10.f * m_dpi_scaling, 10.f * m_dpi_scaling));
	ImGui::EndChild( );
}

bool c_oink_ui::begin(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
	return begin_window(name, p_open, flags);
}

void c_oink_ui::end( )
{
	ImGui::End( );
}

void c_oink_ui::same_line(const float offset_x, const float spacing)
{
	ImGui::SameLine(offset_x, spacing);
}

void c_oink_ui::set_cursor_pos(const ImVec2& v)
{
	ImGui::SetCursorPos(v * m_dpi_scaling);
};

void c_oink_ui::set_cursor_pos_x(const float x)
{
	ImGui::SetCursorPosX(x * m_dpi_scaling);
};

void c_oink_ui::set_cursor_pos_y(const float y)
{
	ImGui::SetCursorPosY(y * m_dpi_scaling);
}

float c_oink_ui::get_cursor_pos_x( )
{
	return ImGui::GetCursorPosX( );
};

float c_oink_ui::get_cursor_pos_y( )
{
	return ImGui::GetCursorPosY( );
};


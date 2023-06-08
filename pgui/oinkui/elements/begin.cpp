#include "../ui.h"

using namespace ImGui;

constexpr float WINDOWS_HOVER_PADDING = 4.0f;     // Extend outside window for hovering/resizing (maxxed with TouchPadding) and inside windows for borders. Affect FindHoveredWindow().
constexpr float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f;    // Reduce visual noise by only highlighting the border after a certain time.
constexpr float WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER = 2.00f;    // Lock scrolled window (so it doesn't pick child windows that are scrolling through) for a certain time, unless mouse moved.

// Data for resizing from corner
struct ImGuiResizeGripDef
{
	ImVec2  CornerPosN;
	ImVec2  InnerDir;
	int     AngleMin12, AngleMax12;
};

constexpr ImGuiResizeGripDef resize_grip_def[4] =
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

constexpr ImGuiResizeBorderDef resize_border_def[4] =
{
	{ ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), IM_PI * 1.00f }, // Left
	{ ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), IM_PI * 0.00f }, // Right
	{ ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), IM_PI * 1.50f }, // Up
	{ ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), IM_PI * 0.50f }  // Down
};

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

void RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, bool handle_borders_and_resize_grips, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size)
{
	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = g.Style;
	ImGuiWindowFlags flags = window->Flags;

	// Ensure that ScrollBar doesn't read last frame's SkipItems
	IM_ASSERT(window->BeginCount == 0);
	window->SkipItems = false;

	// Draw window + handle manual resize
	// As we highlight the title bar when want_focus is set, multiple reappearing windows will have their title bar highlighted on their reappearing frame.
	const float window_rounding = window->WindowRounding;
	const float window_border_size = window->WindowBorderSize;
	if (window->Collapsed)
	{
		// Title bar only
		const float backup_border_size = style.FrameBorderSize;
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
		if (handle_borders_and_resize_grips && !(flags & ImGuiWindowFlags_NoResize))
		{
			for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
			{
				const ImU32 col = resize_grip_col[resize_grip_n];
				if ((col & IM_COL32_A_MASK) == 0)
					continue;
				const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
				const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPosN);
				window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(window_border_size, resize_grip_draw_size) : ImVec2(resize_grip_draw_size, window_border_size)));
				window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(resize_grip_draw_size, window_border_size) : ImVec2(window_border_size, resize_grip_draw_size)));
				window->DrawList->PathArcToFast(ImVec2(corner.x + grip.InnerDir.x * (window_rounding + window_border_size), corner.y + grip.InnerDir.y * (window_rounding + window_border_size)), window_rounding, grip.AngleMin12, grip.AngleMax12);
				window->DrawList->PathFillConvex(col);
			}
		}

		// Borders
		if (handle_borders_and_resize_grips)
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
		UpdateWindowInFocusOrderList(window, window_just_created, flags);
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
	window_stack_data.StackSizesOnBegin.SetToContextState(&g);
	g.CurrentWindowStack.push_back(window_stack_data);
	if (flags & ImGuiWindowFlags_ChildMenu)
		g.BeginMenuCount++;

	// Update ->RootWindow and others pointers (before any possible call to FocusWindow)
	if (first_begin_of_the_frame)
	{
		UpdateWindowParentAndRootLinks(window, flags, parent_window);
		window->ParentWindowInBeginStack = parent_window_in_stack;
	}

	// Add to focus scope stack
	PushFocusScope(window->ID);
	window->NavRootFocusScopeId = g.CurrentFocusScopeId;
	g.CurrentWindow = NULL;

	// Add to popup stack
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		popup_ref.Window = window;
		popup_ref.ParentNavLayer = parent_window_in_stack->DC.NavLayerCurrent;
		g.BeginPopupStack.push_back(popup_ref);
		window->PopupId = popup_ref.PopupId;
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

		bool use_current_size_for_scrollbar_x = window_just_created;
		bool use_current_size_for_scrollbar_y = window_just_created;

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
				if (!window->Collapsed)
					use_current_size_for_scrollbar_y = true;
				MarkIniSettingsDirty(window);
			}
		}
		else
		{
			window->Collapsed = false;
		}
		window->WantCollapseToggle = false;

		// SIZE

		// Outer Decoration Sizes
		// (we need to clear ScrollbarSize immediatly as CalcWindowAutoFitSize() needs it and can be called from other locations).
		const ImVec2 scrollbar_sizes_from_last_frame = window->ScrollbarSizes;
		window->DecoOuterSizeX1 = 0.0f;
		window->DecoOuterSizeX2 = 0.0f;
		window->DecoOuterSizeY1 = window->TitleBarHeight( ) + window->MenuBarHeight( );
		window->DecoOuterSizeY2 = 0.0f;
		window->ScrollbarSizes = ImVec2(0.0f, 0.0f);

		// Calculate auto-fit size, handle automatic resize
		const ImVec2 size_auto_fit = CalcWindowAutoFitSize(window, window->ContentSizeIdeal);
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
		if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow))
			if (viewport_rect.GetWidth( ) > 0.0f && viewport_rect.GetHeight( ) > 0.0f)
				ClampWindowPos(window, visibility_rect);
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
		}

		// [Test Engine] Register whole window in the item system (before submitting further decorations)
#ifdef IMGUI_ENABLE_TEST_ENGINE
		if (g.TestEngineHookItems)
		{
			IM_ASSERT(window->IDStack.Size == 1);
			window->IDStack.Size = 0; // As window->IDStack[0] == window->ID here, make sure TestEngine doesn't erroneously see window as parent of itself.
			IMGUI_TEST_ENGINE_ITEM_ADD(window->ID, window->Rect( ), NULL);
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
			// Intentionally use previous frame values for InnerRect and ScrollbarSizes.
			// And when we use window->DecorationUp here it doesn't have ScrollbarSizes.y applied yet.
			ImVec2 avail_size_from_current_frame = ImVec2(window->SizeFull.x, window->SizeFull.y - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2));
			ImVec2 avail_size_from_last_frame = window->InnerRect.GetSize( ) + scrollbar_sizes_from_last_frame;
			ImVec2 needed_size_from_last_frame = window_just_created ? ImVec2(0, 0) : window->ContentSize + window->WindowPadding * 2.0f;
			float size_x_for_scrollbars = use_current_size_for_scrollbar_x ? avail_size_from_current_frame.x : avail_size_from_last_frame.x;
			float size_y_for_scrollbars = use_current_size_for_scrollbar_y ? avail_size_from_current_frame.y : avail_size_from_last_frame.y;
			//bool scrollbar_y_from_last_frame = window->ScrollbarY; // FIXME: May want to use that in the ScrollbarX expression? How many pros vs cons?
			window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
			window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((needed_size_from_last_frame.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
			if (window->ScrollbarX && !window->ScrollbarY)
				window->ScrollbarY = (needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar);
			window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);

			// Amend the partially filled window->DecorationXXX values.
			window->DecoOuterSizeX2 += window->ScrollbarSizes.x;
			window->DecoOuterSizeY2 += window->ScrollbarSizes.y;
		}

		// UPDATE RECTANGLES (1- THOSE NOT AFFECTED BY SCROLLING)
		// Update various regions. Variables they depend on should be set above in this function.
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
		window->InnerRect.Min.x = window->Pos.x + window->DecoOuterSizeX1;
		window->InnerRect.Min.y = window->Pos.y + window->DecoOuterSizeY1;
		window->InnerRect.Max.x = window->Pos.x + window->Size.x - window->DecoOuterSizeX2;
		window->InnerRect.Max.y = window->Pos.y + window->Size.y - window->DecoOuterSizeY2;

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
		window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window);
		window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
		window->DecoInnerSizeX1 = window->DecoInnerSizeY1 = 0.0f;

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
				bool parent_is_empty = (parent_window->DrawList->VtxBuffer.Size == 0);
				if (window->DrawList->CmdBuffer.back( ).ElemCount == 0 && !parent_is_empty && !previous_child_overlapping)
					render_decorations_in_parent = true;

				ImColor back = m_theme_colour_primary;
				back.Value.w = 0.005;
				window->DrawList->AddRectFilled(window->Pos + ImVec2(0, 0), window->Pos + ImVec2(window->Size.x, window->Size.y),
													back);

				window->DrawList->AddRect(window->Pos + ImVec2(-1, -1), window->Pos + ImVec2(window->Size.x + 1, window->Size.y + 1),
											  ImColor(m_theme_colour_primary));

			}
			if (render_decorations_in_parent)
				window->DrawList = parent_window->DrawList;

			// Handle title bar, scrollbar, resize grips and resize borders
			const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
			const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
			const bool handle_borders_and_resize_grips = true; // This exists to facilitate merge with 'docking' branch.
			RenderWindowDecorations(window, title_bar_rect, title_bar_is_highlight, handle_borders_and_resize_grips, resize_grip_count, resize_grip_col, resize_grip_draw_size);

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
		const float work_rect_size_x = (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : ImMax(allow_scrollbar_x ? window->ContentSize.x : 0.0f, window->Size.x - window->WindowPadding.x * 2.0f - (window->DecoOuterSizeX1 + window->DecoOuterSizeX2)));
		const float work_rect_size_y = (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : ImMax(allow_scrollbar_y ? window->ContentSize.y : 0.0f, window->Size.y - window->WindowPadding.y * 2.0f - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2)));
		window->WorkRect.Min.x = ImFloor(window->InnerRect.Min.x - window->Scroll.x + ImMax(window->WindowPadding.x, window->WindowBorderSize));
		window->WorkRect.Min.y = ImFloor(window->InnerRect.Min.y - window->Scroll.y + ImMax(window->WindowPadding.y, window->WindowBorderSize));
		window->WorkRect.Max.x = window->WorkRect.Min.x + work_rect_size_x;
		window->WorkRect.Max.y = window->WorkRect.Min.y + work_rect_size_y;
		window->ParentWorkRect = window->WorkRect;

		// [LEGACY] Content Region
		// FIXME-OBSOLETE: window->ContentRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
		// Used by:
		// - Mouse wheel scrolling + many other things
		window->ContentRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x + window->DecoOuterSizeX1;
		window->ContentRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->DecoOuterSizeY1;
		window->ContentRegionRect.Max.x = window->ContentRegionRect.Min.x + (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : (window->Size.x - window->WindowPadding.x * 2.0f - (window->DecoOuterSizeX1 + window->DecoOuterSizeX2)));
		window->ContentRegionRect.Max.y = window->ContentRegionRect.Min.y + (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : (window->Size.y - window->WindowPadding.y * 2.0f - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2)));

		// Setup drawing context
		// (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
		window->DC.Indent.x = window->DecoOuterSizeX1 + window->WindowPadding.x - window->Scroll.x;
		window->DC.GroupOffset.x = 0.0f;
		window->DC.ColumnsOffset.x = 0.0f;

		// Record the loss of precision of CursorStartPos which can happen due to really large scrolling amount.
		// This is used by clipper to compensate and fix the most common use case of large scroll area. Easy and cheap, next best thing compared to switching everything to double or ImU64.
		double start_pos_highp_x = (double) window->Pos.x + window->WindowPadding.x - (double) window->Scroll.x + window->DecoOuterSizeX1 + window->DC.ColumnsOffset.x;
		double start_pos_highp_y = (double) window->Pos.y + window->WindowPadding.y - (double) window->Scroll.y + window->DecoOuterSizeY1;
		window->DC.CursorStartPos = ImVec2((float) start_pos_highp_x, (float) start_pos_highp_y);
		window->DC.CursorStartPosLossyness = ImVec2((float) (start_pos_highp_x - window->DC.CursorStartPos.x), (float) (start_pos_highp_y - window->DC.CursorStartPos.y));
		window->DC.CursorPos = window->DC.CursorStartPos;
		window->DC.CursorPosPrevLine = window->DC.CursorPos;
		window->DC.CursorMaxPos = window->DC.CursorStartPos;
		window->DC.IdealMaxPos = window->DC.CursorStartPos;
		window->DC.CurrLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
		window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
		window->DC.IsSameLine = window->DC.IsSetPos = false;

		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.NavLayersActiveMask = window->DC.NavLayersActiveMaskNext;
		window->DC.NavLayersActiveMaskNext = 0x00;
		window->DC.NavIsScrollPushableX = true;
		window->DC.NavHideHighlightOneFrame = false;
		window->DC.NavWindowHasScrollY = (window->ScrollMax.y > 0.0f);

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
		// We ImGuiFocusRequestFlags_UnlessBelowModal to:
		// - Avoid focusing a window that is created outside of a modal. This will prevent active modal from being closed.
		// - Position window behind the modal that is not a begin-parent of this window.
		if (want_focus)
			FocusWindow(window, ImGuiFocusRequestFlags_UnlessBelowModal);
		if (want_focus && window == g.NavWindow)
			NavInitWindow(window, false); // <-- this is in the way for us to be able to defer and sort reappearing FocusWindow() calls

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

		// We fill last item data based on Title Bar/Tab, in order for IsItemHovered() and 
		// () to be usable after Begin().
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
	const char* temp_window_name;
	if (name)
		ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s_%08X", parent_window->Name, name, id);
	else
		ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%08X", parent_window->Name, id);

	const float backup_border_size = g.Style.ChildBorderSize;
	if (!border)
		g.Style.ChildBorderSize = 0.0f;
	bool ret = begin_window(temp_window_name, NULL, flags);
	g.Style.ChildBorderSize = backup_border_size;

	ImGuiWindow* child_window = g.CurrentWindow;
	child_window->ChildId = id;
	child_window->AutoFitChildAxises = (ImS8) auto_fit_axises;

	// Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
	// While this is not really documented/defined, it seems that the expected thing to do.
	if (child_window->BeginCount == 1)
		parent_window->DC.CursorPos = child_window->Pos;

	// Process navigation-in immediately so NavInit can run on first frame
	const ImGuiID temp_id_for_activation = (id + 1);
	if (g.ActiveId == temp_id_for_activation)
		ClearActiveID( );
	if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY))
	{
		FocusWindow(child_window);
		NavInitWindow(child_window, false);
		SetActiveID(temp_id_for_activation, child_window); // Steal ActiveId with another arbitrary id so that key-press won't activate child item
		g.ActiveIdSource = g.NavInputSource;
	}
	return ret;
}

bool c_oink_ui::create_child(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
	ImGuiWindow* window = GetCurrentWindow( );
	return begin_child_ex(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}
void c_oink_ui::separator_ex(ImGuiSeparatorFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	IM_ASSERT(ImIsPowerOfTwo(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)));   // Check that only 1 option is selected

	float thickness_draw = 1.0f;
	float thickness_layout = 0.0f;
	if (flags & ImGuiSeparatorFlags_Vertical)
	{
		// Vertical separator, for menu bars (use current line height). Not exposed because it is misleading and it doesn't have an effect on regular layout.
		float y1 = window->DC.CursorPos.y;
		float y2 = window->DC.CursorPos.y + window->DC.CurrLineSize.y;
		const ImRect bb(ImVec2(window->DC.CursorPos.x, y1), ImVec2(window->DC.CursorPos.x + thickness_draw, y2));
		ItemSize(ImVec2(thickness_layout, 0.0f));
		if (!ItemAdd(bb, 0))
			return;

		// Draw
		window->DrawList->AddLine(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x, bb.Max.y), GetColorU32(ImGuiCol_Separator));
		if (g.LogEnabled)
			LogText(" |");
	}
	else if (flags & ImGuiSeparatorFlags_Horizontal)
	{
		// Horizontal Separator
		float x1 = window->Pos.x;
		float x2 = window->Pos.x + window->Size.x + 2;

		// FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator

		// FIXME-WORKRECT: In theory we should simply be using WorkRect.Min.x/Max.x everywhere but it isn't aesthetically what we want,
		// need to introduce a variant of WorkRect for that purpose. (#4787)
		if (ImGuiTable* table = g.CurrentTable)
		{
			x1 = table->Columns[table->CurrentColumn].MinX;
			x2 = table->Columns[table->CurrentColumn].MaxX;
		}

		ImGuiOldColumns* columns = (flags & ImGuiSeparatorFlags_SpanAllColumns) ? window->DC.CurrentColumns : NULL;
		if (columns)
			PushColumnsBackground( );

		// We don't provide our width to the layout so that it doesn't get feed back into AutoFit
		// FIXME: This prevents ->CursorMaxPos based bounding box evaluation from working (e.g. TableEndCell)
		const ImRect bb(ImVec2(x1 + 12 * m_dpi_scaling, window->DC.CursorPos.y), ImVec2(x2 - 12 * m_dpi_scaling, window->DC.CursorPos.y + thickness_draw));
		ItemSize(ImVec2(0.0f, thickness_layout));
		const bool item_visible = ItemAdd(bb, 0);
		if (item_visible)
		{
			// Draw
			window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(ImGuiCol_Separator));
			if (g.LogEnabled)
				LogRenderedText(&bb.Min, "--------------------------------\n");
		}
		if (columns)
		{
			PopColumnsBackground( );
			columns->LineMinY = window->DC.CursorPos.y;
		}
	}
}

void c_oink_ui::separator( )
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (window->SkipItems)
		return;

	// Those flags should eventually be overridable by the user
	ImGuiSeparatorFlags flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
	flags |= ImGuiSeparatorFlags_SpanAllColumns; // NB: this only applies to legacy Columns() api as they relied on Separator() a lot.
	separator_ex(flags);
}

bool c_oink_ui::begin_child(const char* label, int number_of_child)
{
	constexpr float child_x_size_const = 206.f;

	ImGui::SetCursorPos(ImVec2(10.f * m_dpi_scaling + ((child_x_size_const * m_dpi_scaling + 10.f * m_dpi_scaling) * (number_of_child - 1)), 105.f * m_dpi_scaling));

	ImGuiStyle& style = ImGui::GetStyle( );

	if (create_child(label, ImVec2(child_x_size_const * m_dpi_scaling, 393.f * m_dpi_scaling), 0, 0))
	{
		style.ItemSpacing = ImVec2(10.f * m_dpi_scaling, 5.f * m_dpi_scaling);
		ImGui::SetCursorPos(ImVec2(10.f * m_dpi_scaling, 10.f * m_dpi_scaling));

		text_colored(label, m_theme_colour_secondary);

		ImGui::PushStyleColor(ImGuiCol_Separator, m_theme_colour_primary.Value);
		separator( );
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


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

bool scroll_bar_ex(const ImRect& bb_frame, ImGuiID id, ImGuiAxis axis, ImS64* p_scroll_v, ImS64 size_avail_v, ImS64 size_contents_v, ImDrawFlags flags)
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
	bb.Min -= ImVec2(5, 0);
	bb.Max -= ImVec2(5, 0);
	bb.Min += ImVec2(0, 5);
	bb.Max -= ImVec2(0, 5);

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
	const ImU32 grab_col = ImColor(47, 70, 154);
	window->DrawList->AddRectFilled(bb_frame.Min, bb_frame.Max, bg_col, window->WindowRounding, flags);
	ImRect grab_rect;
	if (axis == ImGuiAxis_X)
		grab_rect = ImRect(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm), bb.Min.y, ImLerp(bb.Min.x, bb.Max.x, grab_v_norm) + grab_h_pixels, bb.Max.y);
	else
		grab_rect = ImRect(bb.Min.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm), bb.Max.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm) + grab_h_pixels);

	window->DrawList->AddRectFilled(ImVec2(grab_rect.Min.x, grab_rect.Min.y) + ImVec2(2, 0), grab_rect.Max - ImVec2(2, 0), grab_col, style.ScrollbarRounding);

	return held;
}

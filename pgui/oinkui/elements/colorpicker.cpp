#include "../ui.h"
#include <sstream>

using namespace ImGui;

extern void ColorEditRestoreHS(const float* col, float* H, float* S, float* V);

bool color_picker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	ImDrawList* draw_list = window->DrawList;
	ImGuiStyle& style = g.Style;
	ImGuiIO& io = g.IO;

	const float width = CalcItemWidth( );
	g.NextItemData.ClearFlags( );

	PushID(label);
	BeginGroup( );

	if (!(flags & ImGuiColorEditFlags_NoSidePreview))
		flags |= ImGuiColorEditFlags_NoSmallPreview;

	// Context menu: display and store options.
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		ColorPickerOptionsPopup(col, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags_PickerMask_))
		flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_PickerMask_;
	if (!(flags & ImGuiColorEditFlags_InputMask_))
		flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_InputMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_InputMask_;
	IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_PickerMask_)); // Check that only 1 is selected
	IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));  // Check that only 1 is selected
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar);

	// Setup
	int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
	bool alpha_bar = !(flags & ImGuiColorEditFlags_NoAlpha);
	ImVec2 picker_pos = window->DC.CursorPos;
	float square_sz = GetFrameHeight( );
	float bars_width = square_sz - 10; // Arbitrary smallish width of Hue/Alpha picking bars
	float sv_picker_size = ImMax(bars_width * 1, width - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
	float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x - 3;
	float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;
	float bars_triangles_half_sz = IM_FLOOR(bars_width * 0.20f);

	float backup_initial_col[4];
	memcpy(backup_initial_col, col, components * sizeof(float));

	float wheel_thickness = sv_picker_size * 0.08f;
	float wheel_r_outer = sv_picker_size * 0.50f;
	float wheel_r_inner = wheel_r_outer - wheel_thickness;
	ImVec2 wheel_center(picker_pos.x + (sv_picker_size + bars_width) * 0.5f, picker_pos.y + sv_picker_size * 0.5f);

	// Note: the triangle is displayed rotated with triangle_pa pointing to Hue, but most coordinates stays unrotated for logic.
	float triangle_r = wheel_r_inner - (int) (sv_picker_size * 0.027f);
	ImVec2 triangle_pa = ImVec2(triangle_r, 0.0f); // Hue point.
	ImVec2 triangle_pb = ImVec2(triangle_r * -0.5f, triangle_r * -0.866025f); // Black point.
	ImVec2 triangle_pc = ImVec2(triangle_r * -0.5f, triangle_r * +0.866025f); // White point.

	float H = col[0], S = col[1], V = col[2];
	float R = col[0], G = col[1], B = col[2];
	if (flags & ImGuiColorEditFlags_InputRGB)
	{
		// Hue is lost when converting from greyscale rgb (saturation=0). Restore it.
		ColorConvertRGBtoHSV(R, G, B, H, S, V);
		ColorEditRestoreHS(col, &H, &S, &V);
	}
	else if (flags & ImGuiColorEditFlags_InputHSV)
	{
		ColorConvertHSVtoRGB(H, S, V, R, G, B);
	}

	bool value_changed = false, value_changed_h = false, value_changed_sv = false;

	PushItemFlag(ImGuiItemFlags_NoNav, true);
	if (flags & ImGuiColorEditFlags_PickerHueWheel)
	{
		// Hue wheel + SV triangle logic
		InvisibleButton("hsv", ImVec2(sv_picker_size + style.ItemInnerSpacing.x + bars_width, sv_picker_size));
		if (IsItemActive( ))
		{
			ImVec2 initial_off = g.IO.MouseClickedPos[0] - wheel_center;
			ImVec2 current_off = g.IO.MousePos - wheel_center;
			float initial_dist2 = ImLengthSqr(initial_off);
			if (initial_dist2 >= (wheel_r_inner - 1) * (wheel_r_inner - 1) && initial_dist2 <= (wheel_r_outer + 1) * (wheel_r_outer + 1))
			{
				// Interactive with Hue wheel
				H = ImAtan2(current_off.y, current_off.x) / IM_PI * 0.5f;
				if (H < 0.0f)
					H += 1.0f;
				value_changed = value_changed_h = true;
			}
			float cos_hue_angle = ImCos(-H * 2.0f * IM_PI);
			float sin_hue_angle = ImSin(-H * 2.0f * IM_PI);
			if (ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, ImRotate(initial_off, cos_hue_angle, sin_hue_angle)))
			{
				// Interacting with SV triangle
				ImVec2 current_off_unrotated = ImRotate(current_off, cos_hue_angle, sin_hue_angle);
				if (!ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated))
					current_off_unrotated = ImTriangleClosestPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated);
				float uu, vv, ww;
				ImTriangleBarycentricCoords(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated, uu, vv, ww);
				V = ImClamp(1.0f - vv, 0.0001f, 1.0f);
				S = ImClamp(uu / V, 0.0001f, 1.0f);
				value_changed = value_changed_sv = true;
			}
		}
	}
	else if (flags & ImGuiColorEditFlags_PickerHueBar)
	{
		// SV rectangle logic
		InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
		if (IsItemActive( ))
		{
			S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
			V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));

			// Greatly reduces hue jitter and reset to 0 when hue == 255 and color is rapidly modified using SV square.
			if (g.ColorEditLastColor == ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0)))
				H = g.ColorEditLastHue;
			value_changed = value_changed_sv = true;
		}

		// Hue bar logic
		SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
		InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
		if (IsItemActive( ))
		{
			H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = value_changed_h = true;
		}
	}

	// Alpha bar logic
	if (alpha_bar)
	{
		ImRect bar1_bb(bar1_pos_x - 171, picker_pos.y + 148, bar1_pos_x - 171 + sv_picker_size, picker_pos.y + bars_width + 148);
		SetCursorScreenPos(ImVec2(bar1_pos_x - 171, picker_pos.y + 148));
		InvisibleButton("alpha", ImVec2(sv_picker_size, bars_width));
		if (IsItemActive( ))
		{
			col[3] = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
			value_changed = true;
		}
	}
	PopItemFlag( ); // ImGuiItemFlags_NoNav

	if (!(flags & ImGuiColorEditFlags_NoSidePreview))
	{
		SameLine(0, style.ItemInnerSpacing.x);
		BeginGroup( );
	}

	if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))
	{
		char buf[32];
		unsigned int bitcolor[4];

		auto& color_ref = *reinterpret_cast<ImVec4*>(col);

		if (ImGui::IsKeyPressed(ImGuiKey_C))
		{
			ImU32 _col = ImGui::ColorConvertFloat4ToU32(color_ref);

			bitcolor[0] = (_col >> IM_COL32_R_SHIFT) & 0xFF;
			bitcolor[1] = (_col >> IM_COL32_G_SHIFT) & 0xFF;
			bitcolor[2] = (_col >> IM_COL32_B_SHIFT) & 0xFF;
			bitcolor[3] = (_col >> IM_COL32_A_SHIFT) & 0xFF;

			ImFormatString(buf, sizeof(buf), "#%02X%02X%02X%02X", bitcolor[0], bitcolor[1], bitcolor[2], bitcolor[3]);
			ImGui::SetClipboardText(buf);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_V))
		{
			const char* clip = ImGui::GetClipboardText( );

			size_t clip_length = strnlen_s(clip, 9);

			if ((clip_length == 7 || clip_length == 9) && clip[0] == '#')
			{
				int count = sscanf_s(clip + 1, "%02X%02X%02X%02X", &bitcolor[0], &bitcolor[1], &bitcolor[2], &bitcolor[3]);

				if (count == 4)
				{
					color_ref = ImGui::ColorConvertU32ToFloat4(IM_COL32(bitcolor[0], bitcolor[1], bitcolor[2], bitcolor[3]));
					value_changed = true;
				}
				else if (count == 3) // no alpha
				{
					float copy = color_ref.w;
					color_ref = ImGui::ColorConvertU32ToFloat4(IM_COL32(bitcolor[0], bitcolor[1], bitcolor[2], 0));
					color_ref.w = copy;
					value_changed = true;
				};
			};
		};
	};

	// Convert back color to RGB
	if (value_changed_h || value_changed_sv)
	{
		if (flags & ImGuiColorEditFlags_InputRGB)
		{
			ColorConvertHSVtoRGB(H, S, V, col[0], col[1], col[2]);
			g.ColorEditLastHue = H;
			g.ColorEditLastSat = S;
			g.ColorEditLastColor = ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(col));
		}
		else if (flags & ImGuiColorEditFlags_InputHSV)
		{
			col[0] = H;
			col[1] = S;
			col[2] = V;
		}
	}

	// R,G,B and H,S,V slider color editor
	bool value_changed_fix_hue_wrap = false;

	// Try to cancel hue wrap (after ColorEdit4 call), if any
	if (value_changed_fix_hue_wrap && (flags & ImGuiColorEditFlags_InputRGB))
	{
		float new_H, new_S, new_V;
		ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
		if (new_H <= 0 && H > 0)
		{
			if (new_V <= 0 && V != new_V)
				ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
			else if (new_S <= 0)
				ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
		}
	}

	if (value_changed)
	{
		if (flags & ImGuiColorEditFlags_InputRGB)
		{
			R = col[0];
			G = col[1];
			B = col[2];
			ColorConvertRGBtoHSV(R, G, B, H, S, V);
			ColorEditRestoreHS(col, &H, &S, &V);   // Fix local Hue as display below will use it immediately.
		}
		else if (flags & ImGuiColorEditFlags_InputHSV)
		{
			H = col[0];
			S = col[1];
			V = col[2];
			ColorConvertHSVtoRGB(H, S, V, R, G, B);
		}
	}

	const int style_alpha8 = IM_F32_TO_INT8_SAT(style.Alpha);
	const ImU32 col_black = IM_COL32(0, 0, 0, style_alpha8);
	const ImU32 col_white = IM_COL32(255, 255, 255, style_alpha8);
	const ImU32 col_midgrey = IM_COL32(128, 128, 128, style_alpha8);
	const ImU32 col_hues[6 + 1] = { IM_COL32(255,0,0,style_alpha8), IM_COL32(255,255,0,style_alpha8), IM_COL32(0,255,0,style_alpha8), IM_COL32(0,255,255,style_alpha8), IM_COL32(0,0,255,style_alpha8), IM_COL32(255,0,255,style_alpha8), IM_COL32(255,0,0,style_alpha8) };

	ImVec4 hue_color_f(1, 1, 1, style.Alpha); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
	ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
	ImU32 user_col32_striped_of_alpha = ColorConvertFloat4ToU32(ImVec4(R, G, B, style.Alpha)); // Important: this is still including the main rendering/style alpha!!

	ImVec2 sv_cursor_pos;

	if (flags & ImGuiColorEditFlags_PickerHueWheel)
	{
		// Render Hue Wheel
		const float aeps = 0.5f / wheel_r_outer; // Half a pixel arc length in radians (2pi cancels out).
		const int segment_per_arc = ImMax(4, (int) wheel_r_outer / 12);
		for (int n = 0; n < 6; n++)
		{
			const float a0 = (n) / 6.0f * 2.0f * IM_PI - aeps;
			const float a1 = (n + 1.0f) / 6.0f * 2.0f * IM_PI + aeps;
			const int vert_start_idx = draw_list->VtxBuffer.Size;
			draw_list->PathArcTo(wheel_center, (wheel_r_inner + wheel_r_outer) * 0.5f, a0, a1, segment_per_arc);
			draw_list->PathStroke(col_white, 0, wheel_thickness);
			const int vert_end_idx = draw_list->VtxBuffer.Size;

			// Paint colors over existing vertices
			ImVec2 gradient_p0(wheel_center.x + ImCos(a0) * wheel_r_inner, wheel_center.y + ImSin(a0) * wheel_r_inner);
			ImVec2 gradient_p1(wheel_center.x + ImCos(a1) * wheel_r_inner, wheel_center.y + ImSin(a1) * wheel_r_inner);
			ShadeVertsLinearColorGradientKeepAlpha(draw_list, vert_start_idx, vert_end_idx, gradient_p0, gradient_p1, col_hues[n], col_hues[n + 1]);
		}

		// Render Cursor + preview on Hue Wheel
		float cos_hue_angle = ImCos(H * 2.0f * IM_PI);
		float sin_hue_angle = ImSin(H * 2.0f * IM_PI);
		ImVec2 hue_cursor_pos(wheel_center.x + cos_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f, wheel_center.y + sin_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f);
		float hue_cursor_rad = value_changed_h ? wheel_thickness * 0.65f : wheel_thickness * 0.55f;
		int hue_cursor_segments = ImClamp((int) (hue_cursor_rad / 1.4f), 9, 32);

		draw_list->AddRectFilled(hue_cursor_pos - ImVec2(hue_cursor_rad, hue_cursor_rad), hue_cursor_pos + ImVec2(hue_cursor_rad, hue_cursor_rad), hue_color32);
		//draw_list->AddCircleFilled(hue_cursor_pos, hue_cursor_rad, hue_color32, hue_cursor_segments);
	   // draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad + 1, col_midgrey, hue_cursor_segments);
		//draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad, col_white, hue_cursor_segments);

		// Render SV triangle (rotated according to hue)
		ImVec2 tra = wheel_center + ImRotate(triangle_pa, cos_hue_angle, sin_hue_angle);
		ImVec2 trb = wheel_center + ImRotate(triangle_pb, cos_hue_angle, sin_hue_angle);
		ImVec2 trc = wheel_center + ImRotate(triangle_pc, cos_hue_angle, sin_hue_angle);
		ImVec2 uv_white = GetFontTexUvWhitePixel( );
		draw_list->PrimReserve(6, 6);
		draw_list->PrimVtx(tra, uv_white, hue_color32);
		draw_list->PrimVtx(trb, uv_white, hue_color32);
		draw_list->PrimVtx(trc, uv_white, col_white);
		draw_list->PrimVtx(tra, uv_white, 0);
		draw_list->PrimVtx(trb, uv_white, col_black);
		draw_list->PrimVtx(trc, uv_white, 0);
		draw_list->AddTriangle(tra, trb, trc, col_midgrey, 1.5f);
		sv_cursor_pos = ImLerp(ImLerp(trc, tra, ImSaturate(S)), trb, ImSaturate(1 - V));
	}
	else if (flags & ImGuiColorEditFlags_PickerHueBar)
	{
		// Render SV Square
		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), col_white, hue_color32, hue_color32, col_white);
		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0, 0, col_black, col_black);
		RenderFrameBorder(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0.0f);
		sv_cursor_pos.x = ImClamp(IM_ROUND(picker_pos.x + ImSaturate(S) * sv_picker_size), picker_pos.x + 2, picker_pos.x + sv_picker_size - 2); // Sneakily prevent the circle to stick out too much
		sv_cursor_pos.y = ImClamp(IM_ROUND(picker_pos.y + ImSaturate(1 - V) * sv_picker_size), picker_pos.y + 2, picker_pos.y + sv_picker_size - 2);

		// Render Hue Bar
		for (int i = 0; i < 6; ++i)
			draw_list->AddRectFilledMultiColor(ImVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)), ImVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)), col_hues[i], col_hues[i], col_hues[i + 1], col_hues[i + 1]);
		float bar0_line_y = IM_ROUND(picker_pos.y + H * sv_picker_size);
		RenderFrameBorder(ImVec2(bar0_pos_x, picker_pos.y), ImVec2(bar0_pos_x + bars_width, picker_pos.y + sv_picker_size), 0.0f);

		draw_list->AddLine(ImVec2(bar0_pos_x, bar0_line_y), ImVec2(bar0_pos_x + bars_width, bar0_line_y), ImColor(0, 0, 0, 255));
		draw_list->AddLine(ImVec2(bar0_pos_x, bar0_line_y - 1), ImVec2(bar0_pos_x + bars_width, bar0_line_y - 1), ImColor(255, 255, 255, 255));
		draw_list->AddLine(ImVec2(bar0_pos_x, bar0_line_y + 1), ImVec2(bar0_pos_x + bars_width, bar0_line_y + 1), ImColor(255, 255, 255, 255));
		draw_list->AddLine(ImVec2(bar0_pos_x, bar0_line_y + 2), ImVec2(bar0_pos_x + bars_width, bar0_line_y + 2), ImColor(0, 0, 0, 150));
		draw_list->AddLine(ImVec2(bar0_pos_x, bar0_line_y - 2), ImVec2(bar0_pos_x + bars_width, bar0_line_y - 2), ImColor(0, 0, 0, 150));
	}

	// Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
	float sv_cursor_rad = value_changed_sv ? 6.0f : 3.0f;
	draw_list->AddCircleFilled(sv_cursor_pos, sv_cursor_rad, user_col32_striped_of_alpha, 12);
	draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad + 1, col_midgrey, 12);
	draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad, col_white, 12);

	draw_list->AddRectFilled(sv_cursor_pos - ImVec2(sv_cursor_rad, sv_cursor_rad), sv_cursor_pos + ImVec2(sv_cursor_rad, sv_cursor_rad), user_col32_striped_of_alpha);
	draw_list->AddRect(sv_cursor_pos - ImVec2(sv_cursor_rad, sv_cursor_rad), sv_cursor_pos + ImVec2(sv_cursor_rad, sv_cursor_rad), col_midgrey);
	draw_list->AddRect(sv_cursor_pos - ImVec2(sv_cursor_rad + 1, sv_cursor_rad + 1), sv_cursor_pos + ImVec2(sv_cursor_rad + 1, sv_cursor_rad + 1), col_white);

	// Render alpha bar
	if (alpha_bar)
	{
		float alpha = ImSaturate(col[3]);
		ImRect bar1_bb(bar1_pos_x - 158, picker_pos.y + 148, bar1_pos_x - 158 + sv_picker_size, picker_pos.y + bars_width + 148);
		RenderColorRectWithAlphaCheckerboard(draw_list, bar1_bb.Min, bar1_bb.Max, 0, bar1_bb.GetHeight( ) / 2.0f, ImVec2(0.0f, 0.0f));
		draw_list->AddRectFilledMultiColor(bar1_bb.Min, bar1_bb.Max, user_col32_striped_of_alpha & ~IM_COL32_A_MASK, user_col32_striped_of_alpha, user_col32_striped_of_alpha, user_col32_striped_of_alpha & ~IM_COL32_A_MASK);
		float bar1_line_x = IM_ROUND(picker_pos.x + alpha * sv_picker_size);
		RenderFrameBorder(bar1_bb.Min, bar1_bb.Max, 0.0f);
		draw_list->AddLine(ImVec2(bar1_line_x, picker_pos.y + 148), ImVec2(bar1_line_x, picker_pos.y + 148 + bars_width), ImColor(0, 0, 0, 255));
		draw_list->AddLine(ImVec2(bar1_line_x - 1, picker_pos.y + 148), ImVec2(bar1_line_x - 1, picker_pos.y + 148 + bars_width), ImColor(255, 255, 255, 255));
		draw_list->AddLine(ImVec2(bar1_line_x + 1, picker_pos.y + 148), ImVec2(bar1_line_x + 1, picker_pos.y + 148 + bars_width), ImColor(255, 255, 255, 255));
		draw_list->AddLine(ImVec2(bar1_line_x + 2, picker_pos.y + 148), ImVec2(bar1_line_x + 2, picker_pos.y + 148 + bars_width), ImColor(0, 0, 0, 150));
		draw_list->AddLine(ImVec2(bar1_line_x - 2, picker_pos.y + 148), ImVec2(bar1_line_x - 2, picker_pos.y + 148 + bars_width), ImColor(0, 0, 0, 150));
	}

	EndGroup( );
	EndGroup( );

	if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
		value_changed = false;
	if (value_changed)
		MarkItemEdited(g.LastItemData.ID);

	PopID( );

	return value_changed;
}

bool color_edit4(const char* label, float col[4], ImGuiColorEditFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float square_sz = GetFrameHeight( ) - 4;
	const float w_full = CalcItemWidth( );
	const float w_button = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
	const float w_inputs = w_full - w_button;
	const char* label_display_end = FindRenderedTextEnd(label);
	g.NextItemData.ClearFlags( );

	BeginGroup( );
	PushID(label);

	// If we're not showing any slider there's no point in doing any HSV conversions
	const ImGuiColorEditFlags flags_untouched = flags;
	if (flags & ImGuiColorEditFlags_NoInputs)
		flags = (flags & (~ImGuiColorEditFlags_DisplayMask_)) | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoOptions;

	// Context menu: display and modify options (before defaults are applied)
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		ColorEditOptionsPopup(col, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags_DisplayMask_))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags_DisplayMask_);
	if (!(flags & ImGuiColorEditFlags_DataTypeMask_))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags_DataTypeMask_);
	if (!(flags & ImGuiColorEditFlags_PickerMask_))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_);
	if (!(flags & ImGuiColorEditFlags_InputMask_))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags_InputMask_);
	flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_));
	IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_DisplayMask_)); // Check that only 1 is selected
	IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));   // Check that only 1 is selected

	const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
	const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
	const int components = alpha ? 4 : 3;

	// Convert to the formats we need
	float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
	if ((flags & ImGuiColorEditFlags_InputHSV) && (flags & ImGuiColorEditFlags_DisplayRGB))
		ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
	else if ((flags & ImGuiColorEditFlags_InputRGB) && (flags & ImGuiColorEditFlags_DisplayHSV))
	{
		// Hue is lost when converting from greyscale rgb (saturation=0). Restore it.
		ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
		ColorEditRestoreHS(col, &f[0], &f[1], &f[2]);
	}
	int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

	bool value_changed = false;
	bool value_changed_as_float = false;

	const ImVec2 pos = window->DC.CursorPos;
	const float inputs_offset_x = (style.ColorButtonPosition == ImGuiDir_Left) ? w_button : 0.0f;
	window->DC.CursorPos.x = pos.x + inputs_offset_x;

	if ((flags & (ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB/HSV 0..255 Sliders
		const float w_item_one = ImMax(1.0f, IM_FLOOR((w_inputs - (style.ItemInnerSpacing.x) * (components - 1)) / (float) components));
		const float w_item_last = ImMax(1.0f, IM_FLOOR(w_inputs - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));

		const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
		static const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
		static const char* fmt_table_int[3][4] =
		{
			{   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
			{ "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
			{ "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
		};
		static const char* fmt_table_float[3][4] =
		{
			{   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
			{ "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
			{ "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
		};
		const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_DisplayHSV) ? 2 : 1;

		for (int n = 0; n < components; n++)
		{
			if (n > 0)
				SameLine(0, style.ItemInnerSpacing.x);
			SetNextItemWidth((n + 1 < components) ? w_item_one : w_item_last);
			{
				value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
			}
			if (!(flags & ImGuiColorEditFlags_NoOptions))
				OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
		}
	}
	else if ((flags & ImGuiColorEditFlags_DisplayHex) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB Hexadecimal Input
		char buf[64];
		if (alpha)
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
		else
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
		SetNextItemWidth(w_inputs);
		if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
		{
			value_changed = true;
			char* p = buf;
			while (*p == '#' || ImCharIsBlankA(*p))
				p++;
			i[0] = i[1] = i[2] = 0;
			i[3] = 0xFF; // alpha default to 255 is not parsed by scanf (e.g. inputting #FFFFFF omitting alpha)
			int r;
			if (alpha)
				r = sscanf(p, "%02X%02X%02X%02X", (unsigned int*) &i[0], (unsigned int*) &i[1], (unsigned int*) &i[2], (unsigned int*) &i[3]); // Treat at unsigned (%X is unsigned)
			else
				r = sscanf(p, "%02X%02X%02X", (unsigned int*) &i[0], (unsigned int*) &i[1], (unsigned int*) &i[2]);
			IM_UNUSED(r); // Fixes C6031: Return value ignored: 'sscanf'.
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
	}

	ImGuiWindow* picker_active_window = NULL;
	if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
	{
		const float button_offset_x = ((flags & ImGuiColorEditFlags_NoInputs) || (style.ColorButtonPosition == ImGuiDir_Left)) ? 0.0f : w_inputs + style.ItemInnerSpacing.x;
		window->DC.CursorPos = ImVec2(pos.x + button_offset_x, pos.y);

		const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
		if (ColorButton("##ColorButton", col_v4, flags))
		{
			if (!(flags & ImGuiColorEditFlags_NoPicker))
			{
				// Store current color and open a picker
				g.ColorPickerRef = col_v4;
				OpenPopup("picker");
				SetNextWindowPos(g.LastItemData.Rect.GetBL( ) + ImVec2(0.0f, style.ItemSpacing.y));
			}
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);

		if (BeginPopup("picker"))
		{
			picker_active_window = g.CurrentWindow;
			ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
			ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
			SetNextItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
			PushStyleColor(ImGuiCol_PopupBg, ImVec4(ImColor(51, 53, 61, 150)));
			value_changed |= color_picker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
			PopStyleColor( );
			EndPopup( );
		}
	}

	if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
	{
		SameLine(0.0f, style.ItemInnerSpacing.x);
		TextEx(label, label_display_end);
	}

	// Convert back
	if (value_changed && picker_active_window == NULL)
	{
		if (!value_changed_as_float)
			for (int n = 0; n < 4; n++)
				f[n] = i[n] / 255.0f;
		if ((flags & ImGuiColorEditFlags_DisplayHSV) && (flags & ImGuiColorEditFlags_InputRGB))
		{
			g.ColorEditLastHue = f[0];
			g.ColorEditLastSat = f[1];
			ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
			g.ColorEditLastColor = ColorConvertFloat4ToU32(ImVec4(f[0], f[1], f[2], 0));
		}
		if ((flags & ImGuiColorEditFlags_DisplayRGB) && (flags & ImGuiColorEditFlags_InputHSV))
			ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);

		col[0] = f[0];
		col[1] = f[1];
		col[2] = f[2];
		if (alpha)
			col[3] = f[3];
	}

	PopID( );
	EndGroup( );

	// Drag and Drop Target
	// NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
	if ((g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget( ))
	{
		bool accepted_drag_drop = false;
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
		{
			memcpy((float*) col, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
			value_changed = accepted_drag_drop = true;
		}
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
		{
			memcpy((float*) col, payload->Data, sizeof(float) * components);
			value_changed = accepted_drag_drop = true;
		}

		// Drag-drop payloads are always RGB
		if (accepted_drag_drop && (flags & ImGuiColorEditFlags_InputHSV))
			ColorConvertRGBtoHSV(col[0], col[1], col[2], col[0], col[1], col[2]);
		EndDragDropTarget( );
	}

	// When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
	if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
		g.LastItemData.ID = g.ActiveId;

	if (value_changed)
		MarkItemEdited(g.LastItemData.ID);

	return value_changed;
}

bool c_oink_ui::color_picker(const char* sz, float* col, bool alpha_bar)
{
	auto flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder;

	text(sz);
	ImGui::SameLine( );
	ImGui::SetCursorPosX(180 * m_dpi_scaling);

	if (!alpha_bar)
		flags |= ImGuiColorEditFlags_NoAlpha;

	return color_edit4(sz, col, flags);
}

bool c_oink_ui::color_picker_button(const char* label, float* col, bool draw_on_same_line, bool alpha_bar)
{
	auto flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder;

	ImGui::SameLine( );
	ImGui::SetCursorPosX(draw_on_same_line ? 160 * m_dpi_scaling : 180 * m_dpi_scaling);

	if (!alpha_bar)
		flags |= ImGuiColorEditFlags_NoAlpha;

	return color_edit4(label, col, flags);
}
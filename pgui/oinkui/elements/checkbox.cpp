#include "../ui.h"

using namespace ImGui;

bool c_oink_ui::checkbox(const char* label, bool* v)
{
	ImGui::SetCursorPosX(m_gap * m_dpi_scaling);

	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const float square_sz = GetFrameHeight( ) - 4;
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
	{
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return false;
	}
	const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}

	ImColor color = m_theme_colour;

	float active_animation = g_ui.process_animation(label, 1, *v, 0.78f, 20.f, e_animation_type::animation_static);
	float hovered_animation = g_ui.process_animation(label, 2, hovered, 0.78f, 17.f, e_animation_type::animation_dynamic);
	float alpha = g_ui.process_animation(label, 3, *v, 0.78f, 15.f, e_animation_type::animation_dynamic);

	float active_alpha = g_ui.process_animation(label, 4, *v, 1.f, 20.f, e_animation_type::animation_dynamic);
	float active_pad_modifer = g_ui.process_animation(label, 5, *v, 1.f, 18.f, e_animation_type::animation_dynamic);

	float pad = ImMax(1.0f, square_sz / (3.6f + (1.f - active_pad_modifer) / 100.f));
	float checkmark_rad = g_ui.process_animation(label, 6, true, pad, 15.f, e_animation_type::animation_interp);

	window->DrawList->AddRectFilled(pos, pos + check_bb.GetSize( ), ImColor(0.f, 0.f, 0.f, 0.39f));

	color.Value.w = 0.19f + active_animation;
	window->DrawList->AddRect(pos, pos + check_bb.GetSize( ), color);

	color.Value.w = 0.09f + alpha + alpha;
	window->DrawList->AddRectFilled(ImVec2(check_bb.Min.x, check_bb.Min.y), ImVec2(check_bb.Max.x, check_bb.Max.y), color);

	for (int i = 0; i < 5; i++)
	{
		// wrong
		color.Value.w = (alpha / 3 + 0.02f + (i / 110.f)) / 5;
		window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + check_bb.GetSize( ).x / 2, check_bb.Min.y + check_bb.GetSize( ).y / 2), 7 + i, color);
	};

	//window->DrawList->AddRect(check_bb.Min, check_bb.Max, ImColor(0, 0, 0, active_alpha), style.FrameRounding);

	RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(checkmark_rad, pad), ImColor(0.f, 0.f, 0.f, active_alpha), square_sz - checkmark_rad * 2.0f);

	ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y - 2);

	color = IM_COL32_WHITE;
	color.Value.w = 0.19f + alpha + hovered_animation;
	PushStyleColor(ImGuiCol_Text, color.Value);
	{
		RenderText(label_pos, label);
	}
	PopStyleColor( );

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	return pressed || hovered;
}
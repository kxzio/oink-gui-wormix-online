#include "../../menu.h"

using namespace ImGui; 

bool c_oink_ui::checkbox(const char* label, bool* v)
{
	ImGui::SetCursorPosX(gap * m_dpi_scaling);

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

	int anim_active = Animate("active", label, *v, 200, 0.15, STATIC);

	window->DrawList->AddRectFilled(pos, pos + check_bb.GetSize( ),
									ImColor(0, 0, 0, 100), 0);

	//window->DrawList->AddRect(pos, pos + size,
	//    ImColor(52, 140, 235, int((50 + anim_active) * float(cc_menu::get().border_alpha / 200))));

	float hovered1 = Animate("button_hover", label, hovered, 200, 0.07, DYNAMIC);
	float alpha = Animate("button", label, *v, 200, 0.05, DYNAMIC);
	float hovered_alpha = Animate("button_hovered", label, *v, check_bb.GetSize( ).y, 0.05, DYNAMIC);

	int auto_red = 47;
	int auto_green = 70;
	int auto_blue = 154;

	window->DrawList->AddRect(pos, pos + check_bb.GetSize( ),
							  ImColor(auto_red, auto_green, auto_blue, int(50 + hovered1)), 0, 0, 1);

	window->DrawList->AddRectFilledMultiColor(ImVec2(check_bb.Min.x, check_bb.Min.y), ImVec2(check_bb.Max.x, check_bb.Max.y),
	ImColor(auto_red, auto_green, auto_blue, 25 + int(alpha) + int((hovered_alpha / check_bb.GetSize( ).y) * 200)), 
	ImColor(47, 70, 154, 25 + int(alpha) + int((hovered_alpha / check_bb.GetSize( ).y) * 200)), 
	ImColor(47, 70, 154, 25 + int(alpha) + int((hovered_alpha / check_bb.GetSize( ).y) * 200)), 
	ImColor(47, 70, 154, 25 + int(alpha) + int((hovered_alpha / check_bb.GetSize( ).y) * 200)));

	for (int i = 0; i < 5; i++)
	{
		window->DrawList->AddCircleFilled(ImVec2(check_bb.Min.x + check_bb.GetSize( ).x / 2, check_bb.Min.y + check_bb.GetSize( ).y / 2), 7 + i,
										  ImColor(auto_red, auto_green, auto_blue, int(alpha / (10 + i))));
	}

	int active_alpha = Animate(label, "active_alpha", *v, 255, 0.2, DYNAMIC);
	int active_pad_modifer = Animate(label, "active_pad_modifer", *v, 255, 0.05, DYNAMIC);
	int hovered_alpha2 = Animate(label, "hover_alpha", hovered, 50, 0.05, DYNAMIC);

	bool checkmark_animate = false;
	//window->DrawList->AddRect(check_bb.Min, check_bb.Max, ImColor(0, 0, 0, active_alpha), style.FrameRounding);
	const float pad = ImMax(1.0f, square_sz / (3.0f + (255.f - active_pad_modifer) / 100.f));
	const float checkmark_rad = Animate(label, "rad", true, pad, 0.15, INTERP);
	checkmark_animate = !(hovered && !(*v));
	RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(checkmark_rad, pad), ImColor(0, 0, 0, active_alpha), square_sz - checkmark_rad * 2.0f);

	ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y - 2);

	PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(255, 255, 255, 50 + int(alpha) + int(hovered1 / 3))));
	{
		RenderText(label_pos, label);
	}
	PopStyleColor( );

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	if (hovered)
		return 3;
	return pressed;
}
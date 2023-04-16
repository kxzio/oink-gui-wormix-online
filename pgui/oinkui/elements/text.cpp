#include "../ui.h"

void c_oink_ui::text(const char* str)
{
	set_cursor_pos_x(m_gap);
	ImGui::Text(str);
};

void c_oink_ui::text_colored(const char* str, const ImColor& color)
{
	set_cursor_pos_x(m_gap);
	ImGui::PushStyleColor(ImGuiCol_Text, color.Value);
	text(str);
	ImGui::PopStyleColor( );
}
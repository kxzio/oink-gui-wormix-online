#include "../ui.h"

void c_oink_ui::text(const char* text)
{
	ImGui::SetCursorPosX(m_gap * m_dpi_scaling);
	ImGui::Text(text);
};

void c_oink_ui::text_colored(const char* text)
{
	ImGui::SetCursorPosX(m_gap * m_dpi_scaling);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(47 / 255.f, 70 / 255.f, 154 / 255.f, 1.f));
	ImGui::Text(text);
	ImGui::PopStyleColor( );
}
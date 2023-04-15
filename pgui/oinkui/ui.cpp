#include "ui.h"

int get_random_number(int min, int max)
{
	int num = min + rand( ) % (max - min + 1);
	return num;
}

std::string current_help_tip = "";

void c_oink_ui::textures_create(IDirect3DDevice9* device)
{
	D3DXCreateTextureFromFileInMemoryEx(device, pig, sizeof(pig), 100, 100, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_textures[tex_pig]);
	D3DXCreateTextureFromFileInMemoryEx(device, syb, sizeof(syb), 2000, 2000, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_textures[tex_syb]);
};

float c_oink_ui::process_animation(const char* label, unsigned int seed, bool if_, float v_max, float percentage_speed, e_animation_type type)
{
	return process_animation(generate_unique_id(label, seed), if_, v_max, percentage_speed, type);
}

float c_oink_ui::process_animation(ImGuiID id, bool if_, float v_max, float percentage_speed /* 1.0f = 100%, 1.25f = 125% */, e_animation_type type)
{
#ifdef _DEBUG
	static ImGuiID old_id = 0;
	IM_ASSERT(id != old_id);
	old_id = id;
#endif
	auto& animation = m_animations.try_emplace(id, 0.0f).first->second;

	const auto& io = ImGui::GetIO( );
	float speed = io.DeltaTime * percentage_speed;

	switch (type)
	{
		case e_animation_type::animation_static:
			animation += if_ ? speed : -speed;
			break;
		case e_animation_type::animation_dynamic:
			animation += if_ ? ImAbs(v_max - animation) * speed : animation * (speed * -1.f);
			break;
		case e_animation_type::animation_interp:
			animation = ImLerp(animation, v_max, speed);
			break;
	}

	if (type != e_animation_type::animation_interp)
	{
		animation = ImMin(animation, v_max);

		if (animation < std::numeric_limits<float>::epsilon( ))
			animation = 0.f;
	};

	return animation;
}

ImGuiID c_oink_ui::generate_unique_id(const char* label, unsigned int seed)
{
	return ImGui::GetID(label) ^ ~seed;
}

void c_oink_ui::fonts_create(bool invalidate)
{
	ImGuiIO& io = ImGui::GetIO( );

	auto glyph_ranges = io.Fonts->GetGlyphRangesCyrillic( );

	m_fonts[0] = io.Fonts->AddFontFromMemoryTTF(museo1, sizeof(museo1), 100.0f * m_dpi_scaling, NULL, glyph_ranges);
	m_fonts[1] = io.Fonts->AddFontFromMemoryTTF(museo2, sizeof(museo2), 50.0f * m_dpi_scaling, NULL, glyph_ranges);
	m_fonts[2] = io.Fonts->AddFontFromMemoryTTF(museo3, sizeof(museo3), 13.0f * m_dpi_scaling, NULL, glyph_ranges);
	m_fonts[3] = io.Fonts->AddFontFromMemoryTTF(museo3, sizeof(museo3), 12.0f * m_dpi_scaling, NULL, glyph_ranges);
	m_fonts[4] = io.Fonts->AddFontFromMemoryTTF(museo1, sizeof(museo1), 100.f * m_dpi_scaling, NULL, glyph_ranges);

	if (invalidate)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects( );

		io.Fonts->ClearTexData( );
		io.Fonts->Build( );

		ImGui_ImplDX9_CreateDeviceObjects( );
	};
}

void c_oink_ui::terminate_menu( )
{
	for (auto& font : m_fonts)
		delete font;

	ZeroMemory(m_fonts, sizeof(m_fonts));
	ZeroMemory(m_textures, sizeof(m_textures));
};

void c_oink_ui::draw_menu( )
{
	if (ImGui::IsKeyPressed(ImGuiKey_Insert))
		m_menu_opened = !m_menu_opened;

	if (!m_menu_opened)
		return;

	m_theme_colour = m_theme_colour_backup;
	m_dpi_scaling = m_dpi_scaling_backup;

	//get general drawlist
	ImDrawList* fg_drawlist = ImGui::GetForegroundDrawList( );
	ImDrawList* bg_drawlist = ImGui::GetBackgroundDrawList( );
	ImDrawList* wnd_drawlist = nullptr;

	{ // draw cursor
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);

		const ImVec2& pointer = ImGui::GetIO( ).MousePos;

		fg_drawlist->AddTriangleFilled(
			ImVec2(pointer.x, pointer.y),
			ImVec2(pointer.x, pointer.y + 10 + 3),
			ImVec2(pointer.x + 7 + 3, pointer.y + 7 + 1),
			ImColor(0.f, 0.f, 0.f));

		fg_drawlist->AddTriangle(
			ImVec2(pointer.x, pointer.y),
			ImVec2(pointer.x, pointer.y + 10 + 3),
			ImVec2(pointer.x + 7 + 3, pointer.y + 7 + 1),
			ImColor(1.f, 1.f, 1.f));
	}

	//int backgrnd = g_ui.process_animation("menu", "bckrg", m_menu_opened, 13, 0.15, ImGui::animation_types::e_animation_type::animation_static);

	//window vars
	ImVec2 wnd_pos, wnd_size;

	//window flags
	constexpr int m_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

	ImGui::SetNextWindowSize(ImVec2(658.f * m_dpi_scaling, 510.f * m_dpi_scaling), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(100.f, 100.f), ImGuiCond_Once);

	//menu code
	if (ImGui::Begin("main window", &m_menu_opened, m_flags))
	{
		//load vars
		wnd_pos = ImGui::GetWindowPos( );
		wnd_size = ImGui::GetWindowSize( );
		wnd_drawlist = ImGui::GetWindowDrawList( );

		//setup style
		ImGuiStyle& style = ImGui::GetStyle( );

		style.WindowBorderSize = 0.0f;
		style.ChildBorderSize = 0.0f;
		style.FrameBorderSize = 0.0f;
		style.PopupBorderSize = 0.0f;

		style.Colors[ImGuiCol_ScrollbarBg] = ImColor(0, 0, 0, 0);
		style.Colors[ImGuiCol_Border] = ImColor(0, 0, 0, 0);
		style.Colors[ImGuiCol_Button] = ImColor(14, 16, 25);
		style.Colors[ImGuiCol_PopupBg] = ImColor(14, 16, 25);

		style.ScrollbarRounding = 0;
		style.ScrollbarSize = 3.f;
		style.ItemSpacing = ImVec2(10, 10);
		style.ItemInnerSpacing.x = 10;
		style.ItemInnerSpacing.y = 10;

		configure(bg_drawlist, wnd_pos, wnd_size);

		ImGui::PushFont(m_fonts[e_font_id::font_middle]);
		{
			static int tab = 1;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			{
				ImGui::SetCursorPos(ImVec2(0, 71));

				const float button_size_x = wnd_size.x / 6;

				if (tab_button("Aimbot", ImVec2(button_size_x, 25 * m_dpi_scaling), 0, tab, 1))
					tab = 1;

				ImGui::SameLine( );

				if (tab_button("Anti-aim", ImVec2(button_size_x, 25 * m_dpi_scaling), 0, tab, 2))
					tab = 2;

				ImGui::SameLine( );

				if (tab_button("Visuals", ImVec2(button_size_x, 25 * m_dpi_scaling), 0, tab, 3))
					tab = 3;

				ImGui::SameLine( );

				if (tab_button("ESP", ImVec2(button_size_x, 25 * m_dpi_scaling), 0, tab, 4))
					tab = 4;

				ImGui::SameLine( );

				if (tab_button("Misc", ImVec2(button_size_x, 25 * m_dpi_scaling), 0, tab, 5))
					tab = 5;

				ImGui::SameLine( );

				if (tab_button("Configs", ImVec2(button_size_x, 25 * m_dpi_scaling), 0, tab, 6))
					tab = 6;

				int real_selected_tab_pos_x = button_size_x * (tab - 1);
				int animate_selected_tab_pos_x = g_ui.process_animation("menu", 1, true, real_selected_tab_pos_x, 15.f, e_animation_type::animation_interp);

				bg_drawlist->AddRectFilled(wnd_pos + ImVec2(animate_selected_tab_pos_x, 95 * m_dpi_scaling), wnd_pos + ImVec2(animate_selected_tab_pos_x + button_size_x, 96 * m_dpi_scaling), ImColor(50, 74, 168), 0);
				bg_drawlist->AddRectFilledMultiColor(wnd_pos + ImVec2(animate_selected_tab_pos_x, 70 * m_dpi_scaling), wnd_pos + ImVec2(animate_selected_tab_pos_x + button_size_x, 96 * m_dpi_scaling), ImColor(51, 53, 61, 50), ImColor(51, 53, 61, 50), ImColor(51, 53, 61, 0), ImColor(51, 53, 61, 0));
			}
			ImGui::PopStyleVar( );

			switch (tab)
			{
				case 1:
				{
					begin_child("Aimbot settings", 1);
					{
						static bool enable_aimbot;
						checkbox("Enable", &enable_aimbot);

						if (enable_aimbot)
						{
							static int aim_fov;
							slider_int("Aimbot fov", &aim_fov, 0, 180);
						}

						static bool enable_ignore_team;
						checkbox("Ignore team", &enable_ignore_team);

						static bool hitscan[10];
						const char* text2[ ]{ "Head", "Neck", "Body", "Hands", "Pelvis", "Legs", "Foots" };
						multi_box("Hitscan", hitscan, text2, ARRAYSIZE(text2));

						static int prior_hitbox;
						const char* text[ ]{ "Head", "Neck", "Body", "Hands", "Pelvis", "Legs", "Foots" };
						combo_box("Priority hitbox", &prior_hitbox, text, ARRAYSIZE(text));

						static bool enable_autofire;
						checkbox("Autofire", &enable_autofire);

						static bool enable_ignore_walls;
						checkbox("Ignore walls", &enable_ignore_walls);

						static bool enable_autostop;
						checkbox("Autostop", &enable_autostop);

						static bool enable_autoscope;
						checkbox("Autoscope", &enable_autoscope);

						static bool enable_hitchance;
						checkbox("Hitchance", &enable_hitchance);

						if (enable_hitchance)
						{
							static int hitchance;
							slider_int("Hitchance value", &hitchance, 0, 100);
						}

						static bool enable_minimal_damage;
						checkbox("Minimal damage", &enable_minimal_damage);

						if (enable_minimal_damage)
						{
							static int mindamage;
							slider_int("Min.damage value", &mindamage, 0, 150);
						}
					}
					end_child( );

					begin_child("Weapon configuration", 2);
					{
						static float col[4];
						color_picker("Test colorpicker", col);

						button("Save", ImVec2(186, 30));

						button("Load", ImVec2(186, 30));

						static bool plesp;
						checkbox("Player ESP", &plesp);

						static float col3[4];
						static float col2[4];

						color_picker_button("Visible", col2);
						color_picker_button("Invisible", col3, true);

						static int key;
						static bool dt;
						checkbox("Double tap", &dt);
						hotkey("Double tap key", &key, &dt);

						static char config_name[32];
						input_text("Config name", config_name, sizeof(config_name));
						{
							char buf[64];
							sprintf_s(buf, "%.5f", ImGui::GetIO( ).DeltaTime);
							button(buf, ImVec2(100, 25));
						}

					}
					end_child( );

					begin_child("Extra settings", 3);
					{
					}
					end_child( );
				}
				break;

				case 2:
				{
					begin_child("Aimbot settings", 1);
					{
					}
					end_child( );

					begin_child("Weapon configuration", 2);
					{
					}
					end_child( );

					begin_child("Extra settings", 3);
					{
					}
					end_child( );
				}
				break;

				case 3:
				{
					begin_child("Aimbot settings", 1);
					{
					}
					end_child( );

					begin_child("Weapon configuration", 2);
					{
					}
					end_child( );

					begin_child("Extra settings", 3);
					{
					}
					end_child( );
				}
				break;

				case 4:
				{
					begin_child("Aimbot settings", 1);
					{
					}
					end_child( );

					begin_child("Weapon configuration", 2);
					{
					}
					end_child( );

					begin_child("Extra settings", 3);
					{
					}
					end_child( );
				}
				break;

				case 5:
				{
					begin_child("Aimbot settings", 1);
					{
					}
					end_child( );

					begin_child("Weapon configuration", 2);
					{
					}
					end_child( );

					begin_child("Extra settings", 3);
					{
					}
					end_child( );
				}
				break;

				case 6:
				{
					begin_child("Aimbot settings", 1);
					{
					}
					end_child( );

					begin_child("Weapon configuration", 2);
					{
					}
					end_child( );

					begin_child("Extra settings", 3);
					{
						slider_float("ui scale", &m_dpi_scaling_backup, 0.5f, 1.5f, "%.2f");
						color_picker("ui theme", reinterpret_cast<float*>(&m_theme_colour_backup.Value), false);
					}
					end_child( );
				}
				break;
			}
		}
		ImGui::PopFont( );
	}
	ImGui::End( );
	//close
}

void c_oink_ui::configure(ImDrawList* bg_drawlist, ImVec2& m_menu_pos, ImVec2& m_menu_size, bool main)
{
	bg_drawlist->AddRectFilled(m_menu_pos, m_menu_pos + m_menu_size, ImColor(5, 5, 5), 0);

	bg_drawlist->AddRectFilled(m_menu_pos, m_menu_pos + ImVec2(m_menu_size.x, 70), ImColor(0, 0, 0), 0);

	bg_drawlist->AddRectFilledMultiColor(m_menu_pos, m_menu_pos + ImVec2(m_menu_size.x, 100), ImColor(25, 25, 25, 150), ImColor(25, 25, 25, 150), ImColor(25, 25, 25, 0), ImColor(25, 25, 25, 0));

	if (main)
	{
		//flying pigs
		{
			for (int i = 0; i < 10; i++)
			{
				static std::unordered_map <int, ImVec2> pValue;
				static std::unordered_map <int, int> pValue2;
				static std::unordered_map <int, float> pValue3;
				static std::unordered_map <int, ImVec2> pValue4;

				auto ItPLibrary = pValue.find(i);
				if (ItPLibrary == pValue.end( ))
					ItPLibrary = pValue.insert({ i, ImVec2(get_random_number(0, m_menu_size.x), get_random_number(0, 80)) }).first;

				auto Rotation = pValue2.find(i);

				if (Rotation == pValue2.end( ))
					Rotation = pValue2.insert({ i, get_random_number(0, 360) }).first;

				auto Rotation_value = pValue3.find(i);
				if (Rotation_value == pValue3.end( ))
					Rotation_value = pValue3.insert({ i, static_cast<float>(get_random_number(-3, 3)) }).first;

				auto Move_speed = pValue4.find(i);
				if (Move_speed == pValue4.end( ))
					Move_speed = pValue4.insert({ i, ImVec2(get_random_number(-3, 3), get_random_number(-3, 3)) }).first;

				ItPLibrary->second.x += Move_speed->second.x / 200.f;
				ItPLibrary->second.y += Move_speed->second.y / 200.f;

				Rotation_value->second += ImGui::GetIO( ).DeltaTime;

				if (ItPLibrary->second.x + m_menu_pos.x > m_menu_pos.x + m_menu_size.y)
					Move_speed->second.x *= -1;

				if (ItPLibrary->second.y + m_menu_pos.y > m_menu_pos.y + 70)
					Move_speed->second.y *= -1;

				if (ItPLibrary->second.x + m_menu_pos.x < m_menu_pos.x)
					Move_speed->second.x *= -1;

				if (ItPLibrary->second.y + m_menu_pos.y < m_menu_pos.y)
					Move_speed->second.y *= -1;

				bg_drawlist->PushClipRect(ImVec2(m_menu_pos), ImVec2(m_menu_pos + ImVec2(m_menu_size.x, 70)));
				ImRotateStart(Rotation->second);

				bg_drawlist->AddImage(m_textures[tex_pig], m_menu_pos + ItPLibrary->second, m_menu_pos + ItPLibrary->second + ImVec2(35, 35), ImVec2(0, 0), ImVec2(1, 1), ImColor(125, 143, 212, 30));
				ImRotateEnd(Rotation_value->second, Rotation->second);
				bg_drawlist->PopClipRect( );
			}
		}

		bg_drawlist->AddText(m_fonts[font_giant], 100, m_menu_pos + ImVec2(100, 1), ImColor(50, 74, 168, 200), "Industries");
	}

	//bg_drawlist->AddText(c_oink_ui::get( ).small_font, 12, m_menu_pos + ImVec2(4, 3), ImColor(255, 255, 255, 100), main ? "Oink.industries | beta | v1.01 | User" : "Player list");

	bg_drawlist->AddRectFilled(m_menu_pos + ImVec2(0, 70), m_menu_pos + m_menu_size, ImColor(10, 10, 10), 0);

	bg_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 70), m_menu_pos + ImVec2(m_menu_size.x, 71), ImColor(50, 74, 168), ImColor(50, 74, 168, 0), ImColor(50, 74, 168, 0), ImColor(50, 74, 168));
	bg_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 0), m_menu_pos + ImVec2(m_menu_size.x, 71), ImColor(50, 74, 168, 20), ImColor(50, 74, 168, 20), ImColor(50, 74, 168, 0), ImColor(50, 74, 168, 0));
	bg_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 70), m_menu_pos + ImVec2(m_menu_size.x, m_menu_size.y), ImColor(50, 74, 168, 25), ImColor(50, 74, 168, 20), ImColor(50, 74, 168, 0), ImColor(50, 74, 168, 0));
	bg_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 96), m_menu_pos + ImVec2(m_menu_size.x, m_menu_size.y), ImColor(9, 10, 15, 100), ImColor(9, 10, 15, 100), ImColor(21, 25, 38, 0), ImColor(21, 25, 38, 0));
	bg_drawlist->AddRectFilledMultiColor(m_menu_pos + ImVec2(0, 96), m_menu_pos + ImVec2(m_menu_size.x, m_menu_size.y / 2), ImColor(0, 0, 0, 100), ImColor(0, 0, 0, 100), ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0));

	bg_drawlist->AddRectFilled(m_menu_pos + ImVec2(0, 95), m_menu_pos + ImVec2(m_menu_size.x, 96), ImColor(51, 53, 61, 150), 0);

	bg_drawlist->AddRect(m_menu_pos, m_menu_pos + m_menu_size, ImColor(50, 74, 168, 50), 0);
}

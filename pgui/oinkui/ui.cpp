#include "ui.h"

const char* GKeyNames[ ] =
{
	"Tab", "Left", "Right", "Up", "Down", "Pg Up", "Pg Dn",
	"Home", "End", "Insert", "Delete", "Back", "Space", "Enter", "Esc",
	"L Ctrl", "L Shift", "L Alt", "L Super", "R Ctrl", "R Shift", "R Alt", "R Super", "Menu",
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H",
	"I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
	"'", ",", "-", ".", "/", ";", "=", "[",
	"\\", "]", "~", "Caps", "ScrLck", "Num", "PrtScr",
	"Pause", "Num 0", "Num 1", "Num 2", "Num 3", "Num 4", "Num 5", "Num 6",
	"Num 7", "Num 8", "Num 9", "Num .", "Num /", "Num *",
	"Num -", "Num +", "Num Enter", "Num =",
	"Start", "Back",
	"FaceLeft", "FaceRight", "FaceUp", "FaceDown",
	"DPadLeft", "DPadRight", "DPadUp", "DPadDown",
	"L1", "R1", "L2", "R2", "L3", "R3",
	"LS Left", "LS Right", "LS Up", "LS Down",
	"RS Left", "RS Right", "RS Up", "RS Down",
	"LMB", "RMB", "MMB", "X1", "X2", "X Wheel", "Y Wheel",
	"Mod Ctrl", "Mod Shift", "Mod Alt", "Mod Super"
};
std::string current_help_tip = "";

void c_oink_ui::textures_create(IDirect3DDevice9* device)
{
	D3DXCreateTextureFromFileInMemoryEx(device, pig, sizeof(pig), 100, 100, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_textures[tex_pig]);
};

float c_oink_ui::process_animation(const char* label, unsigned int seed, bool condition, float max_value, float percentage_speed, e_animation_type type)
{
	return process_animation(generate_unique_id(label, seed), condition, max_value, percentage_speed, type);
}

float c_oink_ui::process_animation(ImGuiID id, bool condition, float max_value, float percentage_speed /* 1.0f = 100%, 1.25f = 125% */, e_animation_type type)
{
#ifdef _DEBUG
	static ImGuiID old_id = 0;
	IM_ASSERT(id != old_id);
	old_id = id;
#endif
	auto& animation = m_animations.try_emplace(id, 0.0f).first->second;

	const auto& io = ImGui::GetIO( );
	float speed = io.DeltaTime * percentage_speed;

	IM_ASSERT(speed > 0.0f);

	switch (type)
	{
		case e_animation_type::animation_static:
			animation += condition ? speed : -speed;
			break;
		case e_animation_type::animation_dynamic:
			animation += condition ? ImAbs(max_value - animation) * speed : animation * (speed * -1.f);
			break;
		case e_animation_type::animation_interp:
			animation = ImLerp(animation, max_value, speed);
			break;
	}

	if (type != e_animation_type::animation_interp)
	{
		animation = ImMin(animation, max_value);

		if (animation < std::numeric_limits<float>::epsilon( ))
			animation = 0.f;
	};

	return animation;
}

ImGuiID c_oink_ui::generate_unique_id(const char* label, unsigned int seed)
{
	return ImGui::GetID(label) ^ ~seed;
}

void c_oink_ui::render_cursor(ImDrawList* fg_drawlist)
{
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
		m_theme_colour_primary);
};

void c_oink_ui::fonts_create(bool invalidate)
{
	ImGuiIO& io = ImGui::GetIO( );

	auto glyph_ranges = io.Fonts->GetGlyphRangesCyrillic( );

	ZeroMemory(m_fonts, sizeof(m_fonts));
	io.Fonts->Clear( );

	ImFontConfig cfg = ImFontConfig( );
	cfg.FontDataOwnedByAtlas = false;

	cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_ForceAutoHint;

	m_fonts[e_font_id::font_big] = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Micross.ttf", 100.0f * m_dpi_scaling_backup, &cfg, glyph_ranges);
	m_fonts[e_font_id::font_small] = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Micross.ttf", 50.0f * m_dpi_scaling_backup, &cfg, glyph_ranges);
	m_fonts[e_font_id::font_default] = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Micross.ttf", 12.0f * m_dpi_scaling_backup, &cfg, glyph_ranges);
	m_fonts[e_font_id::font_middle] = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Micross.ttf", 12.0f * m_dpi_scaling_backup, &cfg, glyph_ranges);

	cfg.FontBuilderFlags &= ~ImGuiFreeTypeBuilderFlags_ForceAutoHint;
	cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Bold;

	m_fonts[e_font_id::font_giant] = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Segoeuib.ttf", 100.f * m_dpi_scaling_backup, &cfg, glyph_ranges);

	if (invalidate)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects( );

		io.Fonts->ClearTexData( );
		io.Fonts->Build( );

		ImGui_ImplDX9_CreateDeviceObjects( );
	};
}

void c_oink_ui::draw_menu( )
{
	if (ImGui::IsKeyPressed(ImGuiKey_Insert))
		m_menu_opened = !m_menu_opened;

	if (!m_menu_opened)
		return;

	// apply colors from previous frame
	ImGui::ColorConvertHSVtoRGB(m_theme_hsv_backup[0], m_theme_hsv_backup[1], m_theme_hsv_backup[2], m_theme_colour_primary.Value.x, m_theme_colour_primary.Value.y, m_theme_colour_primary.Value.z);
	ImGui::ColorConvertHSVtoRGB(m_theme_hsv_backup[0], m_theme_hsv_backup[1], m_theme_hsv_backup[2] * 0.6f, m_theme_colour_secondary.Value.x, m_theme_colour_secondary.Value.y, m_theme_colour_secondary.Value.z);

	// apply scaling from previous frame
	m_dpi_scaling = m_dpi_scaling_backup;

	//get general drawlist
	ImDrawList* fg_drawlist = ImGui::GetForegroundDrawList( );
	ImDrawList* bg_drawlist = ImGui::GetBackgroundDrawList( );
	ImDrawList* wnd_drawlist = nullptr;

	render_cursor(fg_drawlist);

	//window vars
	ImVec2 wnd_pos, wnd_size;

	//window flags
	constexpr int m_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

	ImGui::SetNextWindowSize(ImVec2(658.f * m_dpi_scaling, 510.f * m_dpi_scaling), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(100.f, 100.f), ImGuiCond_Once);

	//menu code
	if (begin("main window", &m_menu_opened, m_flags))
	{
		//style
		auto& style = ImGui::GetStyle( );

		style.Colors[ImGuiCol_ScrollbarBg] = m_theme_colour_primary;
		style.Colors[ImGuiCol_Border] = m_theme_colour_primary;
		style.Colors[ImGuiCol_ScrollbarGrab] = m_theme_colour_primary;
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = m_theme_colour_primary;
		style.Colors[ImGuiCol_ScrollbarGrabActive] = m_theme_colour_primary;
		style.ScrollbarSize = 1.f;
		style.PopupBorderSize = 0.f;

		//load vars
		wnd_pos = ImGui::GetWindowPos( );
		wnd_size = ImGui::GetWindowSize( );
		wnd_drawlist = ImGui::GetWindowDrawList( );

		//setup style
		configure(bg_drawlist, wnd_pos, wnd_size);

		ImGui::PushFont(m_fonts[e_font_id::font_middle]);
		{
			static int tab = 1;
			const float button_size_x = wnd_size.x / 6;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
			{
				set_cursor_pos(ImVec2(0.f, 71.f));

				if (tab_button("Aimbot", ImVec2(button_size_x, 25.f), ImGuiButtonFlags_None, tab == 1))
					tab = 1;

				same_line( );

				if (tab_button("Anti-aim", ImVec2(button_size_x, 25.f), ImGuiButtonFlags_None, tab == 2))
					tab = 2;

				same_line( );

				if (tab_button("Visuals", ImVec2(button_size_x, 25.f), ImGuiButtonFlags_None, tab == 3))
					tab = 3;

				same_line( );

				if (tab_button("ESP", ImVec2(button_size_x, 25.f), ImGuiButtonFlags_None, tab == 4))
					tab = 4;

				same_line( );

				if (tab_button("Misc", ImVec2(button_size_x, 25.f), ImGuiButtonFlags_None, tab == 5))
					tab = 5;

				same_line( );

				if (tab_button("Configs", ImVec2(button_size_x, 25.f), ImGuiButtonFlags_None, tab == 6))
					tab = 6;

				int real_selected_tab_pos_x = button_size_x * (tab - 1);
				int animate_selected_tab_pos_x = g_ui.process_animation("menu", 1, true, real_selected_tab_pos_x, 15.f, e_animation_type::animation_interp);

				bg_drawlist->AddRectFilled(wnd_pos + ImVec2(animate_selected_tab_pos_x, 95.f * m_dpi_scaling),
										   wnd_pos + ImVec2(animate_selected_tab_pos_x + button_size_x, 96.f * m_dpi_scaling),
										   m_theme_colour_primary);

				bg_drawlist->AddRectFilledMultiColor(wnd_pos + ImVec2(animate_selected_tab_pos_x, 70.f * m_dpi_scaling),
													 wnd_pos + ImVec2(animate_selected_tab_pos_x + button_size_x, 96 * m_dpi_scaling),
													 ImColor(51, 51, 51, 50),
													 ImColor(51, 51, 51, 50),
													 IM_COL32_BLACK_TRANS,
													 IM_COL32_BLACK_TRANS);
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

						for (int i = 0; i <= 50; ++i)
						{
							char buf[64];
							sprintf_s(buf, "button %d", i);
							button(buf, ImVec2(0, 0));
						}
					}
					end_child( );

					begin_child("Weapon configuration", 2);
					{
						static float col[4];
						color_picker("Test colorpicker", col);

						button("Save", ImVec2(184, 30));

						button("Load", ImVec2(184, 30));

						static bool plesp;
						checkbox("Player ESP", &plesp);

						static float col3[4];
						static float col2[4];

						color_picker_button("Visible", col2);
						color_picker_button("Invisible", col3, true);

						{
							set_cursor_pos_x(m_gap);
							button_ex("button1_test");

							set_cursor_pos_x(m_gap);
							button_ex("button2_test");
							same_line(0.f, 5.f);
							button_ex("button3_test");
							same_line(0.f, 5.f);
							button_ex("button4_test");
						};

						static keybind_t key;
						checkbox("Double tap", &key.m_active);
						hotkey("Double tap key", &key);

						static char config_name[32];
						input_text("Config name", config_name, sizeof(config_name));
						{
							char buf[64];
							sprintf_s(buf, "time: %.5f", ImGui::GetIO( ).DeltaTime);
							text(buf);
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
						static int slider_value_dpi_scale = 2;
						if (combo_box("DPI Scale", &slider_value_dpi_scale, "50%\00075%\000100%\000125%\000150%\000175%\000200%", 6))
						{
						};

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

						static int slider_value_dpi_scale = 1;
						if (combo_box("DPI Scale", &slider_value_dpi_scale, "75%\000100%\000125%\000150%\000175%\000200%", 6))
						{
							constexpr float scale_factors[ ] = { 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };
							m_dpi_scaling_backup = scale_factors[slider_value_dpi_scale];
							m_dpi_changed = true;
						};

						text("ui hue");
						slider_float("ui hue", &m_theme_hsv_backup[0], 0.f, 1.f);
						text("ui saturation");
						slider_float("ui saturation", &m_theme_hsv_backup[1], 0.f, 1.f);
						text("ui brightness");
						slider_float("ui brightness", &m_theme_hsv_backup[2], 0.f, 1.f);
					}
					end_child( );
				}
				break;
			}
		}
		ImGui::PopFont( );
	}
	end( );
	//close
}

void c_oink_ui::pre_draw_menu( )
{
	if (m_dpi_changed)
	{
		g_ui.fonts_create(true);
		m_dpi_changed = false;
	};
};

void c_oink_ui::configure(ImDrawList* bg_drawlist, ImVec2& menu_pos, ImVec2& menu_size, bool main)
{
	auto& io = ImGui::GetIO( );

	ImColor color_sec_transparent = m_theme_colour_secondary;
	color_sec_transparent.Value.w = 0.2f;

	constexpr float y_max = 70.f;

	bg_drawlist->AddRectFilled(menu_pos, menu_pos + menu_size, ImColor(5, 5, 5));

	bg_drawlist->AddRectFilledMultiColor(menu_pos, menu_pos + ImVec2(menu_size.x, 100.f * m_dpi_scaling), ImColor(25, 25, 25, 200), ImColor(25, 25, 25, 200), IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS);

	{
		const ImVec2 real_bb[2] = { menu_pos, menu_pos + ImVec2(menu_size.x, y_max * m_dpi_scaling) };
		const ImVec2 bb[2] = { ImVec2(0, 0), ImVec2(menu_size.x, y_max * m_dpi_scaling) };
		const ImVec2 picture_size = ImVec2(35.f * m_dpi_scaling, 35.f * m_dpi_scaling);

		bg_drawlist->PushClipRect(real_bb[0], real_bb[1]);

		for (auto& data : m_pigs_data)
		{
			auto& rotation = data.m_rotation;
			auto& rotation_index = data.m_rotation_index;
			auto& position = data.m_position;
			auto& speed = data.m_speed;

			rotation += io.DeltaTime;

			for (uint8_t axis = 0u; axis != 2; ++axis)
			{
				bool collide = false;

				position[axis] += io.DeltaTime * speed[axis];

				if (position[axis] >= bb[1][axis])
				{
					collide = true;
					position[axis] = bb[1][axis];
				}
				else if ((position[axis] + picture_size[axis]) >= bb[1][axis])
				{
					collide = true;
					position[axis] = bb[1][axis] - picture_size[axis];
				}
				else if (position[axis] <= bb[0][axis])
				{
					collide = true;
					position[axis] = bb[0][axis];
				}
				else if ((position[axis] + picture_size[axis]) <= bb[0][axis])
				{
					collide = true;
					position[axis] = bb[0][axis] + picture_size[axis];
				};

				if (collide)
					speed[axis] *= -1.f;
			};

			rotate_start(bg_drawlist, rotation_index);

			bg_drawlist->AddImage(m_textures[e_tex_id::tex_pig],
								  position + menu_pos,
								  position + picture_size + menu_pos,
								  ImVec2(0, 0),
								  ImVec2(1, 1),
								  color_sec_transparent);

			rotate_end(bg_drawlist, rotation, rotation_index);
		};

		color_sec_transparent.Value.w = 0.5f;

		bg_drawlist->AddText(m_fonts[font_giant], 100.f * m_dpi_scaling, menu_pos + (ImVec2(100.f, 1.f) * m_dpi_scaling), color_sec_transparent, "Industries");

		bg_drawlist->PopClipRect( );
	}

	//bg_drawlist->AddText(c_oink_ui::get( ).small_font, 12, m_menu_pos + ImVec2(4, 3), ImColor(255, 255, 255, 100), main ? "Oink.industries | beta | v1.01 | User" : "Player list");

	bg_drawlist->AddRectFilled(menu_pos + ImVec2(0, 70 * m_dpi_scaling),
							   menu_pos + menu_size,
							   ImColor(10, 10, 10));

	// shine?
	bg_drawlist->AddRectFilledMultiColor(menu_pos + ImVec2(0, 70 * m_dpi_scaling),
										 menu_pos + ImVec2(menu_size.x, 71 * m_dpi_scaling),
										 m_theme_colour_secondary,
										 IM_COL32_BLACK_TRANS,
										 IM_COL32_BLACK_TRANS,
										 m_theme_colour_secondary);

	color_sec_transparent.Value.w = 0.07f;

	bg_drawlist->AddRectFilledMultiColor(menu_pos,
										 menu_pos + ImVec2(menu_size.x, 71.f * m_dpi_scaling),
										 color_sec_transparent,
										 color_sec_transparent,
										 IM_COL32_BLACK_TRANS,
										 IM_COL32_BLACK_TRANS);

	bg_drawlist->AddRectFilledMultiColor(menu_pos + ImVec2(0, 70.f * m_dpi_scaling),
										 menu_pos + ImVec2(menu_size.x, menu_size.y),
										 color_sec_transparent,
										 color_sec_transparent,
										 IM_COL32_BLACK_TRANS,
										 IM_COL32_BLACK_TRANS);

	bg_drawlist->AddRectFilledMultiColor(menu_pos + ImVec2(0, 96 * m_dpi_scaling),
										 menu_pos + ImVec2(menu_size.x, menu_size.y),
										 ImColor(9, 10, 15, 100),
										 ImColor(9, 10, 15, 100),
										 IM_COL32_BLACK_TRANS,
										 IM_COL32_BLACK_TRANS);

	bg_drawlist->AddRectFilledMultiColor(menu_pos + ImVec2(0, 96 * m_dpi_scaling),
										 menu_pos + ImVec2(menu_size.x, menu_size.y / 2),
										 ImColor(0, 0, 0, 100),
										 ImColor(0, 0, 0, 100),
										 IM_COL32_BLACK_TRANS,
										 IM_COL32_BLACK_TRANS);

	color_sec_transparent.Value.w = 0.58f;

	bg_drawlist->AddRectFilled(menu_pos + (ImVec2(0.f, 95.f) * m_dpi_scaling), menu_pos + (ImVec2(menu_size.x, 96.f * m_dpi_scaling)), ImColor(51, 51, 51, 150));

	bg_drawlist->AddRect(menu_pos, menu_pos + menu_size, color_sec_transparent);
}

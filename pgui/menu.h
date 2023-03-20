#pragma once
#include "singleton.hpp"
#include "imgui/imgui.h"
#include <d3dx9.h>

class cc_menu : public singleton <cc_menu>
{
public:
	void init_fonts( );
	void reinit_fonts( );
	float dpi_scale = 1.f;
	bool dpi_changed;
	float theme_col[4] = { 47 / 255.f, 70 / 255.f, 154 / 255.f };
	int theme_r = int(theme_col[0] * 255);
	int theme_g = int(theme_col[1] * 255);
	int theme_b = int(theme_col[2] * 255);
	int theme_a = int(theme_col[3] * 255);
	void draw_menu( );
	bool g_menu_opened;
	ImFont* giant_font;
	ImFont* default_font;
	ImFont* middle_font;
	ImFont* small_font;
	IDirect3DTexture9* stars_data;
	IDirect3DTexture9* syb;
	IDirect3DTexture9* pig;
	int g_opened_tab = 1;
	float border_alpha;
};
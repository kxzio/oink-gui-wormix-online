#include "../ui.h"

using namespace ImGui;

bool input_text_ex(const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data, ImColor& theme_colour, const float& dpi_scale, bool show_title = true)
{
	ImGuiWindow* window = GetCurrentWindow( );
	if (window->SkipItems)
		return false;

	if (show_title)
	{
		float backup_x = GetCursorPosX( );
		ImGui::PushStyleColor(ImGuiCol_Text, theme_colour.Value);
		Text(label);
		ImGui::PopStyleColor( );
		SetCursorPosX(backup_x);
	}

	IM_ASSERT(buf != NULL && buf_size >= 0);
	IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackHistory) && (flags & ImGuiInputTextFlags_Multiline)));        // Can't use both together (they both use up/down keys)
	IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackCompletion) && (flags & ImGuiInputTextFlags_AllowTabInput))); // Can't use both together (they both use tab key)

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	const bool RENDER_SELECTION_WHEN_INACTIVE = false;
	const bool is_multiline = (flags & ImGuiInputTextFlags_Multiline) != 0;
	const bool is_readonly = (flags & ImGuiInputTextFlags_ReadOnly) != 0;
	const bool is_password = (flags & ImGuiInputTextFlags_Password) != 0;
	const bool is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;
	const bool is_resizable = (flags & ImGuiInputTextFlags_CallbackResize) != 0;
	if (is_resizable)
		IM_ASSERT(callback != NULL); // Must provide a callback if you set the ImGuiInputTextFlags_CallbackResize flag!

	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	if (is_multiline) // Open group before calling GetID() because groups tracks id created within their scope (including the scrollbar)
		BeginGroup( );

	const float w = 186 * dpi_scale;

	ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f * dpi_scale));
	ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	if (size_arg.x == 0 && size_arg.y == 0)
	{

	}
	else
	{
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const ImVec2 frame_size = CalcItemSize(size_arg, CalcItemWidth( ), (is_multiline ? g.FontSize * 8.0f : label_size.y) + style.FramePadding.y * 2.0f); // Arbitrary default of 8 lines high for multi-line
		const ImVec2 total_size = ImVec2(frame_size.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_size.y);

		ImRect frame_bb = ImRect(window->DC.CursorPos, window->DC.CursorPos + frame_size);
		ImRect total_bb = ImRect(frame_bb.Min, frame_bb.Min + total_size);
	}



	ImColor color = theme_colour;

	color.Value.w = 0.07f;
	window->DrawList->AddRectFilledMultiColor(frame_bb.Min, frame_bb.Max, IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, color, color);

	const bool hovered = ItemHoverable(frame_bb, id);

	float animation_active_hovered = g_ui.process_animation(label, 1, hovered || g.ActiveId == id, 0.78f, 0.07f, e_animation_type::animation_dynamic);

	color.Value.w = animation_active_hovered / 5.f;

	window->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, color);

	color.Value.w = 1.f;
	window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, color);

	const ImVec2 frame_size = frame_bb.GetSize( ); // Arbitrary default of 8 lines high for multi-line
	const ImVec2 total_size = total_bb.GetSize( );

	ImGuiWindow* draw_window = window;
	ImVec2 inner_size = frame_size;
	ImGuiItemStatusFlags item_status_flags = 0;
	ImGuiLastItemData item_data_backup;
	if (is_multiline)
	{
		ImVec2 backup_pos = window->DC.CursorPos;
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
		{
			EndGroup( );
			return false;
		}
		item_status_flags = g.LastItemData.StatusFlags;
		item_data_backup = g.LastItemData;
		window->DC.CursorPos = backup_pos;

		// We reproduce the contents of BeginChildFrame() in order to provide 'label' so our window internal data are easier to read/debug.
		// FIXME-NAV: Pressing NavActivate will trigger general child activation right before triggering our own below. Harmless but bizarre.
		PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
		PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
		PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Ensure no clip rect so mouse hover can reach FramePadding edges
		bool child_visible = BeginChildEx(label, id, frame_bb.GetSize( ), true, ImGuiWindowFlags_NoMove);
		PopStyleVar(3);
		PopStyleColor( );
		if (!child_visible)
		{
			EndChild( );
			EndGroup( );
			return false;
		}
		draw_window = g.CurrentWindow; // Child window
		draw_window->DC.NavLayersActiveMaskNext |= (1 << draw_window->DC.NavLayerCurrent); // This is to ensure that EndChild() will display a navigation highlight so we can "enter" into it.
		draw_window->DC.CursorPos += style.FramePadding;
		inner_size.x -= draw_window->ScrollbarSizes.x;
	}
	else
	{
		// Support for internal ImGuiInputTextFlags_MergedItem flag, which could be redesigned as an ItemFlags if needed (with test performed in ItemAdd)
		ItemSize(total_bb, style.FramePadding.y);
		if (!(flags & ImGuiInputTextFlags_MergedItem))
			if (!ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
				return false;
		item_status_flags = g.LastItemData.StatusFlags;
	}
	if (hovered)
		g.MouseCursor = ImGuiMouseCursor_TextInput;

	// We are only allowed to access the state if we are already the active widget.
	ImGuiInputTextState* state = GetInputTextState(id);

	const bool input_requested_by_tabbing = (item_status_flags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
	const bool input_requested_by_nav = (g.ActiveId != id) && ((g.NavActivateId == id) && ((g.NavActivateFlags & ImGuiActivateFlags_PreferInput) || (g.NavInputSource == ImGuiInputSource_Keyboard)));

	const bool user_clicked = hovered && io.MouseClicked[0];
	const bool user_scroll_finish = is_multiline && state != NULL && g.ActiveId == 0 && g.ActiveIdPreviousFrame == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
	const bool user_scroll_active = is_multiline && state != NULL && g.ActiveId == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
	bool clear_active_id = false;
	bool select_all = false;

	float scroll_y = is_multiline ? draw_window->Scroll.y : FLT_MAX;

	const bool init_changed_specs = (state != NULL && state->Stb.single_line != !is_multiline);
	const bool init_make_active = (user_clicked || user_scroll_finish || input_requested_by_nav || input_requested_by_tabbing);
	const bool init_state = (init_make_active || user_scroll_active);
	if ((init_state && g.ActiveId != id) || init_changed_specs)
	{
		// Access state even if we don't own it yet.
		state = &g.InputTextState;
		state->CursorAnimReset( );

		// Take a copy of the initial buffer value (both in original UTF-8 format and converted to wchar)
		// From the moment we focused we are ignoring the content of 'buf' (unless we are in read-only mode)
		const int buf_len = (int) strlen(buf);
		state->InitialTextA.resize(buf_len + 1);    // UTF-8. we use +1 to make sure that .Data is always pointing to at least an empty string.
		memcpy(state->InitialTextA.Data, buf, buf_len + 1);

		// Start edition
		const char* buf_end = NULL;
		state->TextW.resize(buf_size + 1);          // wchar count <= UTF-8 count. we use +1 to make sure that .Data is always pointing to at least an empty string.
		state->TextA.resize(0);
		state->TextAIsValid = false;                // TextA is not valid yet (we will display buf until then)
		state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, buf_size, buf, NULL, &buf_end);
		state->CurLenA = (int) (buf_end - buf);      // We can't get the result from ImStrncpy() above because it is not UTF-8 aware. Here we'll cut off malformed UTF-8.

		// Preserve cursor position and undo/redo stack if we come back to same widget
		// FIXME: For non-readonly widgets we might be able to require that TextAIsValid && TextA == buf ? (untested) and discard undo stack if user buffer has changed.
		const bool recycle_state = (state->ID == id && !init_changed_specs);
		if (recycle_state)
		{
			// Recycle existing cursor/selection/undo stack but clamp position
			// Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
			state->CursorClamp( );
		}
		else
		{
			state->ID = id;
			state->ScrollX = 0.0f;
			stb_textedit_initialize_state(&state->Stb, !is_multiline);
		}

		if (!is_multiline)
		{
			if (flags & ImGuiInputTextFlags_AutoSelectAll)
				select_all = true;
			if (input_requested_by_nav && (!recycle_state || !(g.NavActivateFlags & ImGuiActivateFlags_TryToPreserveState)))
				select_all = true;
			if (input_requested_by_tabbing || (user_clicked && io.KeyCtrl))
				select_all = true;
		}

		if (flags & ImGuiInputTextFlags_AlwaysOverwrite)
			state->Stb.insert_mode = 1; // stb field name is indeed incorrect (see #2863)
	}

	const bool is_osx = io.ConfigMacOSXBehaviors;
	if (g.ActiveId != id && init_make_active)
	{
		IM_ASSERT(state && state->ID == id);
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
	}
	if (g.ActiveId == id)
	{
		// Declare some inputs, the other are registered and polled via Shortcut() routing system.
		if (user_clicked)
			SetKeyOwner(ImGuiKey_MouseLeft, id);
		g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
		if (is_multiline || (flags & ImGuiInputTextFlags_CallbackHistory))
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
		SetKeyOwner(ImGuiKey_Home, id);
		SetKeyOwner(ImGuiKey_End, id);
		if (is_multiline)
		{
			SetKeyOwner(ImGuiKey_PageUp, id);
			SetKeyOwner(ImGuiKey_PageDown, id);
		}
		if (is_osx)
			SetKeyOwner(ImGuiMod_Alt, id);
		if (flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_AllowTabInput)) // Disable keyboard tabbing out as we will use the \t character.
			SetShortcutRouting(ImGuiKey_Tab, id);
	}


	// We have an edge case if ActiveId was set through another widget (e.g. widget being swapped), clear id immediately (don't wait until the end of the function)
	if (g.ActiveId == id && state == NULL)
		ClearActiveID( );

	// Release focus when we click outside
	if (g.ActiveId == id && io.MouseClicked[0] && !init_state && !init_make_active) //-V560
		clear_active_id = true;

	// Lock the decision of whether we are going to take the path displaying the cursor or selection
	bool render_cursor = (g.ActiveId == id) || (state && user_scroll_active);
	bool render_selection = state && (state->HasSelection( ) || select_all) && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
	bool value_changed = false;
	bool enter_pressed = false;
	bool validated = false;

	// When read-only we always use the live data passed to the function
	// FIXME-OPT: Because our selection/cursor code currently needs the wide text we need to convert it when active, which is not ideal :(
	if (is_readonly && state != NULL && (render_cursor || render_selection))
	{
		const char* buf_end = NULL;
		state->TextW.resize(buf_size + 1);
		state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, state->TextW.Size, buf, NULL, &buf_end);
		state->CurLenA = (int) (buf_end - buf);
		state->CursorClamp( );
		render_selection &= state->HasSelection( );
	}

	// Select the buffer to render.
	const bool buf_display_from_state = (render_cursor || render_selection || g.ActiveId == id) && !is_readonly && state && state->TextAIsValid;
	const bool is_displaying_hint = (hint != NULL && (buf_display_from_state ? state->TextA.Data : buf)[0] == 0);

	// Password pushes a temporary font with only a fallback glyph
	if (is_password && !is_displaying_hint)
	{
		const ImFontGlyph* glyph = g.Font->FindGlyph('*');
		ImFont* password_font = &g.InputTextPasswordFont;
		password_font->FontSize = g.Font->FontSize;
		password_font->Scale = g.Font->Scale;
		password_font->Ascent = g.Font->Ascent;
		password_font->Descent = g.Font->Descent;
		password_font->ContainerAtlas = g.Font->ContainerAtlas;
		password_font->FallbackGlyph = glyph;
		password_font->FallbackAdvanceX = glyph->AdvanceX;
		IM_ASSERT(password_font->Glyphs.empty( ) && password_font->IndexAdvanceX.empty( ) && password_font->IndexLookup.empty( ));
		PushFont(password_font);
	}

	// Process mouse inputs and character inputs
	int backup_current_text_length = 0;
	if (g.ActiveId == id)
	{
		IM_ASSERT(state != NULL);
		backup_current_text_length = state->CurLenA;
		state->Edited = false;
		state->BufCapacityA = buf_size;
		state->Flags = flags;

		// Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
		// Down the line we should have a cleaner library-wide concept of Selected vs Active.
		g.ActiveIdAllowOverlap = !io.MouseDown[0];
		g.WantTextInputNextFrame = 1;

		// Edit in progress
		const float mouse_x = (io.MousePos.x - frame_bb.Min.x - style.FramePadding.x) + state->ScrollX;
		const float mouse_y = (is_multiline ? (io.MousePos.y - draw_window->DC.CursorPos.y) : (g.FontSize * 0.5f));

		const bool is_osx = io.ConfigMacOSXBehaviors;
		if (select_all)
		{
			state->SelectAll( );
			state->SelectedAllMouseLock = true;
		}
		else if (hovered && io.MouseClickedCount[0] >= 2 && !io.KeyShift)
		{
			stb_textedit_click(state, &state->Stb, mouse_x, mouse_y);
			const int multiclick_count = (io.MouseClickedCount[0] - 2);
			if ((multiclick_count % 2) == 0)
			{
				// Double-click: Select word
				// We always use the "Mac" word advance for double-click select vs CTRL+Right which use the platform dependent variant:
				// FIXME: There are likely many ways to improve this behavior, but there's no "right" behavior (depends on use-case, software, OS)
				const bool is_bol = (state->Stb.cursor == 0) || ImStb::STB_TEXTEDIT_GETCHAR(state, state->Stb.cursor - 1) == '\n';
				if (STB_TEXT_HAS_SELECTION(&state->Stb) || !is_bol)
					state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);
				//state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
				if (!STB_TEXT_HAS_SELECTION(&state->Stb))
					ImStb::stb_textedit_prep_selection_at_cursor(&state->Stb);
				state->Stb.cursor = ImStb::STB_TEXTEDIT_MOVEWORDRIGHT_MAC(state, state->Stb.cursor);
				state->Stb.select_end = state->Stb.cursor;
				ImStb::stb_textedit_clamp(state, &state->Stb);
			}
			else
			{
				// Triple-click: Select line
				const bool is_eol = ImStb::STB_TEXTEDIT_GETCHAR(state, state->Stb.cursor) == '\n';
				state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART);
				state->OnKeyPressed(STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_SHIFT);
				state->OnKeyPressed(STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_SHIFT);
				if (!is_eol && is_multiline)
				{
					ImSwap(state->Stb.select_start, state->Stb.select_end);
					state->Stb.cursor = state->Stb.select_end;
				}
				state->CursorFollow = false;
			}
			state->CursorAnimReset( );
		}
		else if (io.MouseClicked[0] && !state->SelectedAllMouseLock)
		{
			// FIXME: unselect on late click could be done release?
			if (hovered)
			{
				stb_textedit_click(state, &state->Stb, mouse_x, mouse_y);
				state->CursorAnimReset( );
			}
		}
		else if (io.MouseDown[0] && !state->SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
		{
			stb_textedit_drag(state, &state->Stb, mouse_x, mouse_y);
			state->CursorAnimReset( );
			state->CursorFollow = true;
		}
		if (state->SelectedAllMouseLock && !io.MouseDown[0])
			state->SelectedAllMouseLock = false;

		// We except backends to emit a Tab key but some also emit a Tab character which we ignore (#2467, #1336)
		// (For Tab and Enter: Win32/SFML/Allegro are sending both keys and chars, GLFW and SDL are only sending keys. For Space they all send all threes)
		const bool ignore_char_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeySuper);
		if ((flags & ImGuiInputTextFlags_AllowTabInput) && IsKeyPressed(ImGuiKey_Tab) && !ignore_char_inputs && !io.KeyShift && !is_readonly)
		{
			unsigned int c = '\t'; // Insert TAB
			if (InputTextFilterCharacter(&c, flags, callback, callback_user_data, ImGuiInputSource_Keyboard))
				state->OnKeyPressed((int) c);
		}

		// Process regular text input (before we check for Return because using some IME will effectively send a Return?)
		// We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
		if (io.InputQueueCharacters.Size > 0)
		{
			if (!ignore_char_inputs && !is_readonly && !input_requested_by_nav)
				for (int n = 0; n < io.InputQueueCharacters.Size; n++)
				{
					// Insert character if they pass filtering
					unsigned int c = (unsigned int) io.InputQueueCharacters[n];
					if (c == '\t') // Skip Tab, see above.
						continue;
					if (InputTextFilterCharacter(&c, flags, callback, callback_user_data, ImGuiInputSource_Keyboard))
						state->OnKeyPressed((int) c);
				}

			// Consume characters
			io.InputQueueCharacters.resize(0);
		}
	}

	// Process other shortcuts/key-presses
	bool revert_edit = false;
	if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
	{
		IM_ASSERT(state != NULL);

		const int row_count_per_page = ImMax((int) ((inner_size.y - style.FramePadding.y) / g.FontSize), 1);
		state->Stb.row_count_per_page = row_count_per_page;

		const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
		const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
		const bool is_startend_key_down = is_osx && io.KeySuper && !io.KeyCtrl && !io.KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End

		// Using Shortcut() with ImGuiInputFlags_RouteFocused (default policy) to allow routing operations for other code (e.g. calling window trying to use CTRL+A and CTRL+B: formet would be handled by InputText)
		// Otherwise we could simply assume that we own the keys as we are active.
		const ImGuiInputFlags f_repeat = ImGuiInputFlags_Repeat;
		const bool is_cut = (Shortcut(ImGuiMod_Shortcut | ImGuiKey_X, id, f_repeat) || Shortcut(ImGuiMod_Shift | ImGuiKey_Delete, id, f_repeat)) && !is_readonly && !is_password && (!is_multiline || state->HasSelection( ));
		const bool is_copy = (Shortcut(ImGuiMod_Shortcut | ImGuiKey_C, id) || Shortcut(ImGuiMod_Ctrl | ImGuiKey_Insert, id)) && !is_password && (!is_multiline || state->HasSelection( ));
		const bool is_paste = (Shortcut(ImGuiMod_Shortcut | ImGuiKey_V, id, f_repeat) || Shortcut(ImGuiMod_Shift | ImGuiKey_Insert, id, f_repeat)) && !is_readonly;
		const bool is_undo = (Shortcut(ImGuiMod_Shortcut | ImGuiKey_Z, id, f_repeat)) && !is_readonly && is_undoable;
		const bool is_redo = (Shortcut(ImGuiMod_Shortcut | ImGuiKey_Y, id, f_repeat) || (is_osx && Shortcut(ImGuiMod_Shortcut | ImGuiMod_Shift | ImGuiKey_Z, id, f_repeat))) && !is_readonly && is_undoable;
		const bool is_select_all = Shortcut(ImGuiMod_Shortcut | ImGuiKey_A, id);

		// We allow validate/cancel with Nav source (gamepad) to makes it easier to undo an accidental NavInput press with no keyboard wired, but otherwise it isn't very useful.
		const bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
		const bool is_enter_pressed = IsKeyPressed(ImGuiKey_Enter, true) || IsKeyPressed(ImGuiKey_KeypadEnter, true);
		const bool is_gamepad_validate = nav_gamepad_active && (IsKeyPressed(ImGuiKey_NavGamepadActivate, false) || IsKeyPressed(ImGuiKey_NavGamepadInput, false));
		const bool is_cancel = Shortcut(ImGuiKey_Escape, id, f_repeat) || (nav_gamepad_active && Shortcut(ImGuiKey_NavGamepadCancel, id, f_repeat));

		// FIXME: Should use more Shortcut() and reduce IsKeyPressed()+SetKeyOwner(), but requires modifiers combination to be taken account of.
		if (IsKeyPressed(ImGuiKey_LeftArrow))
		{
			state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask);
		}
		else if (IsKeyPressed(ImGuiKey_RightArrow))
		{
			state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask);
		}
		else if (IsKeyPressed(ImGuiKey_UpArrow) && is_multiline)
		{
			if (io.KeyCtrl) SetScrollY(draw_window, ImMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask);
		}
		else if (IsKeyPressed(ImGuiKey_DownArrow) && is_multiline)
		{
			if (io.KeyCtrl) SetScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY( ))); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask);
		}
		else if (IsKeyPressed(ImGuiKey_PageUp) && is_multiline)
		{
			state->OnKeyPressed(STB_TEXTEDIT_K_PGUP | k_mask); scroll_y -= row_count_per_page * g.FontSize;
		}
		else if (IsKeyPressed(ImGuiKey_PageDown) && is_multiline)
		{
			state->OnKeyPressed(STB_TEXTEDIT_K_PGDOWN | k_mask); scroll_y += row_count_per_page * g.FontSize;
		}
		else if (IsKeyPressed(ImGuiKey_Home))
		{
			state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask);
		}
		else if (IsKeyPressed(ImGuiKey_End))
		{
			state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask);
		}
		else if (IsKeyPressed(ImGuiKey_Delete) && !is_readonly && !is_cut)
		{
			if (!state->HasSelection( ))
			{
				// OSX doesn't seem to have Super+Delete to delete until end-of-line, so we don't emulate that (as opposed to Super+Backspace)
				if (is_wordmove_key_down)
					state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
			}
			state->OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask);
		}
		else if (IsKeyPressed(ImGuiKey_Backspace) && !is_readonly)
		{
			if (!state->HasSelection( ))
			{
				if (is_wordmove_key_down)
					state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
				else if (is_osx && io.KeySuper && !io.KeyAlt && !io.KeyCtrl)
					state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
			}
			state->OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask);
		}
		else if (is_enter_pressed || is_gamepad_validate)
		{
			// Determine if we turn Enter into a \n character
			bool ctrl_enter_for_new_line = (flags & ImGuiInputTextFlags_CtrlEnterForNewLine) != 0;
			if (!is_multiline || is_gamepad_validate || (ctrl_enter_for_new_line && !io.KeyCtrl) || (!ctrl_enter_for_new_line && io.KeyCtrl))
			{
				validated = true;
				if (io.ConfigInputTextEnterKeepActive && !is_multiline)
					state->SelectAll( ); // No need to scroll
				else
					clear_active_id = true;
			}
			else if (!is_readonly)
			{
				unsigned int c = '\n'; // Insert new line
				if (InputTextFilterCharacter(&c, flags, callback, callback_user_data, ImGuiInputSource_Keyboard))
					state->OnKeyPressed((int) c);
			}
		}
		else if (is_cancel)
		{
			if (flags & ImGuiInputTextFlags_EscapeClearsAll)
			{
				if (state->CurLenA > 0)
				{
					revert_edit = true;
				}
				else
				{
					render_cursor = render_selection = false;
					clear_active_id = true;
				}
			}
			else
			{
				clear_active_id = revert_edit = true;
				render_cursor = render_selection = false;
			}
		}
		else if (is_undo || is_redo)
		{
			state->OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
			state->ClearSelection( );
		}
		else if (is_select_all)
		{
			state->SelectAll( );
			state->CursorFollow = true;
		}
		else if (is_cut || is_copy)
		{
			// Cut, Copy
			if (io.SetClipboardTextFn)
			{
				const int ib = state->HasSelection( ) ? ImMin(state->Stb.select_start, state->Stb.select_end) : 0;
				const int ie = state->HasSelection( ) ? ImMax(state->Stb.select_start, state->Stb.select_end) : state->CurLenW;
				const int clipboard_data_len = ImTextCountUtf8BytesFromStr(state->TextW.Data + ib, state->TextW.Data + ie) + 1;
				char* clipboard_data = (char*) IM_ALLOC(clipboard_data_len * sizeof(char));
				ImTextStrToUtf8(clipboard_data, clipboard_data_len, state->TextW.Data + ib, state->TextW.Data + ie);
				SetClipboardText(clipboard_data);
				MemFree(clipboard_data);
			}
			if (is_cut)
			{
				if (!state->HasSelection( ))
					state->SelectAll( );
				state->CursorFollow = true;
				stb_textedit_cut(state, &state->Stb);
			}
		}
		else if (is_paste)
		{
			if (const char* clipboard = GetClipboardText( ))
			{
				// Filter pasted buffer
				const int clipboard_len = (int) strlen(clipboard);
				ImWchar* clipboard_filtered = (ImWchar*) IM_ALLOC((clipboard_len + 1) * sizeof(ImWchar));
				int clipboard_filtered_len = 0;
				for (const char* s = clipboard; *s != 0; )
				{
					unsigned int c;
					s += ImTextCharFromUtf8(&c, s, NULL);
					if (!InputTextFilterCharacter(&c, flags, callback, callback_user_data, ImGuiInputSource_Clipboard))
						continue;
					clipboard_filtered[clipboard_filtered_len++] = (ImWchar) c;
				}
				clipboard_filtered[clipboard_filtered_len] = 0;
				if (clipboard_filtered_len > 0) // If everything was filtered, ignore the pasting operation
				{
					stb_textedit_paste(state, &state->Stb, clipboard_filtered, clipboard_filtered_len);
					state->CursorFollow = true;
				}
				MemFree(clipboard_filtered);
			}
		}

		// Update render selection flag after events have been handled, so selection highlight can be displayed during the same frame.
		render_selection |= state->HasSelection( ) && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
	}
	// Process callbacks and apply result back to user's buffer.
	const char* apply_new_text = NULL;
	int apply_new_text_length = 0;
	if (g.ActiveId == id)
	{
		IM_ASSERT(state != NULL);
		if (revert_edit)
		{
			// Restore initial value. Only return true if restoring to the initial value changes the current buffer contents.
			if (!is_readonly && strcmp(buf, state->InitialTextA.Data) != 0)
			{
				// Push records into the undo stack so we can CTRL+Z the revert operation itself
				apply_new_text = state->InitialTextA.Data;
				apply_new_text_length = state->InitialTextA.Size - 1;
				ImVector<ImWchar> w_text;
				if (apply_new_text_length > 0)
				{
					w_text.resize(ImTextCountCharsFromUtf8(apply_new_text, apply_new_text + apply_new_text_length) + 1);
					ImTextStrFromUtf8(w_text.Data, w_text.Size, apply_new_text, apply_new_text + apply_new_text_length);
				}
				stb_textedit_replace(state, &state->Stb, w_text.Data, (apply_new_text_length > 0) ? (w_text.Size - 1) : 0);
			}
		}

		// When using 'ImGuiInputTextFlags_EnterReturnsTrue' as a special case we reapply the live buffer back to the input buffer before clearing ActiveId, even though strictly speaking it wasn't modified on this frame.
		// If we didn't do that, code like InputInt() with ImGuiInputTextFlags_EnterReturnsTrue would fail.
		// This also allows the user to use InputText() with ImGuiInputTextFlags_EnterReturnsTrue without maintaining any user-side storage (please note that if you use this property along ImGuiInputTextFlags_CallbackResize you can end up with your temporary string object unnecessarily allocating once a frame, either store your string data, either if you don't then don't use ImGuiInputTextFlags_CallbackResize).
		bool apply_edit_back_to_user_buffer = !revert_edit || (enter_pressed && (flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0);
		if (apply_edit_back_to_user_buffer)
		{
			// Apply new value immediately - copy modified buffer back
			// Note that as soon as the input box is active, the in-widget value gets priority over any underlying modification of the input buffer
			// FIXME: We actually always render 'buf' when calling DrawList->AddText, making the comment above incorrect.
			// FIXME-OPT: CPU waste to do this every time the widget is active, should mark dirty state from the stb_textedit callbacks.
			if (!is_readonly)
			{
				state->TextAIsValid = true;
				state->TextA.resize(state->TextW.Size * 4 + 1);
				ImTextStrToUtf8(state->TextA.Data, state->TextA.Size, state->TextW.Data, NULL);
			}

			// User callback
			if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackAlways)) != 0)
			{
				IM_ASSERT(callback != NULL);

				// The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
				ImGuiInputTextFlags event_flag = 0;
				ImGuiKey event_key = ImGuiKey_None;
				if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && IsKeyPressed(ImGuiKey_Tab))
				{
					event_flag = ImGuiInputTextFlags_CallbackCompletion;
					event_key = ImGuiKey_Tab;
				}
				else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressed(ImGuiKey_UpArrow))
				{
					event_flag = ImGuiInputTextFlags_CallbackHistory;
					event_key = ImGuiKey_UpArrow;
				}
				else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressed(ImGuiKey_DownArrow))
				{
					event_flag = ImGuiInputTextFlags_CallbackHistory;
					event_key = ImGuiKey_DownArrow;
				}
				else if ((flags & ImGuiInputTextFlags_CallbackEdit) && state->Edited)
				{
					event_flag = ImGuiInputTextFlags_CallbackEdit;
				}
				else if (flags & ImGuiInputTextFlags_CallbackAlways)
				{
					event_flag = ImGuiInputTextFlags_CallbackAlways;
				}

				if (event_flag)
				{
					ImGuiInputTextCallbackData callback_data;
					memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
					callback_data.EventFlag = event_flag;
					callback_data.Flags = flags;
					callback_data.UserData = callback_user_data;

					char* callback_buf = is_readonly ? buf : state->TextA.Data;
					callback_data.EventKey = event_key;
					callback_data.Buf = callback_buf;
					callback_data.BufTextLen = state->CurLenA;
					callback_data.BufSize = state->BufCapacityA;
					callback_data.BufDirty = false;

					// We have to convert from wchar-positions to UTF-8-positions, which can be pretty slow (an incentive to ditch the ImWchar buffer, see https://github.com/nothings/stb/issues/188)
					ImWchar* text = state->TextW.Data;
					const int utf8_cursor_pos = callback_data.CursorPos = ImTextCountUtf8BytesFromStr(text, text + state->Stb.cursor);
					const int utf8_selection_start = callback_data.SelectionStart = ImTextCountUtf8BytesFromStr(text, text + state->Stb.select_start);
					const int utf8_selection_end = callback_data.SelectionEnd = ImTextCountUtf8BytesFromStr(text, text + state->Stb.select_end);

					// Call user code
					callback(&callback_data);

					// Read back what user may have modified
					callback_buf = is_readonly ? buf : state->TextA.Data; // Pointer may have been invalidated by a resize callback
					IM_ASSERT(callback_data.Buf == callback_buf);         // Invalid to modify those fields
					IM_ASSERT(callback_data.BufSize == state->BufCapacityA);
					IM_ASSERT(callback_data.Flags == flags);
					const bool buf_dirty = callback_data.BufDirty;
					if (callback_data.CursorPos != utf8_cursor_pos || buf_dirty)
					{
						state->Stb.cursor = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.CursorPos); state->CursorFollow = true;
					}
					if (callback_data.SelectionStart != utf8_selection_start || buf_dirty)
					{
						state->Stb.select_start = (callback_data.SelectionStart == callback_data.CursorPos) ? state->Stb.cursor : ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionStart);
					}
					if (callback_data.SelectionEnd != utf8_selection_end || buf_dirty)
					{
						state->Stb.select_end = (callback_data.SelectionEnd == callback_data.SelectionStart) ? state->Stb.select_start : ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionEnd);
					}
					if (buf_dirty)
					{
						IM_ASSERT(callback_data.BufTextLen == (int) strlen(callback_data.Buf)); // You need to maintain BufTextLen if you change the text!
						if (callback_data.BufTextLen > backup_current_text_length && is_resizable)
							state->TextW.resize(state->TextW.Size + (callback_data.BufTextLen - backup_current_text_length));
						state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, state->TextW.Size, callback_data.Buf, NULL);
						state->CurLenA = callback_data.BufTextLen;  // Assume correct length and valid UTF-8 from user, saves us an extra strlen()
						state->CursorAnimReset( );
					}
				}
			}

			// Will copy result string if modified
			if (!is_readonly && strcmp(state->TextA.Data, buf) != 0)
			{
				apply_new_text = state->TextA.Data;
				apply_new_text_length = state->CurLenA;
			}
		}

		// Clear temporary user storage
		state->Flags = ImGuiInputTextFlags_None;
	}

	// Copy result to user buffer. This can currently only happen when (g.ActiveId == id)
	if (apply_new_text != NULL)
	{
		// We cannot test for 'backup_current_text_length != apply_new_text_length' here because we have no guarantee that the size
		// of our owned buffer matches the size of the string object held by the user, and by design we allow InputText() to be used
		// without any storage on user's side.
		IM_ASSERT(apply_new_text_length >= 0);
		if (is_resizable)
		{
			ImGuiInputTextCallbackData callback_data;
			callback_data.EventFlag = ImGuiInputTextFlags_CallbackResize;
			callback_data.Flags = flags;
			callback_data.Buf = buf;
			callback_data.BufTextLen = apply_new_text_length;
			callback_data.BufSize = ImMax(buf_size, apply_new_text_length + 1);
			callback_data.UserData = callback_user_data;
			callback(&callback_data);
			buf = callback_data.Buf;
			buf_size = callback_data.BufSize;
			apply_new_text_length = ImMin(callback_data.BufTextLen, buf_size - 1);
			IM_ASSERT(apply_new_text_length <= buf_size);
		}
		//IMGUI_DEBUG_LOG("InputText(\"%s\"): apply_new_text length %d\n", label, apply_new_text_length);

		// If the underlying buffer resize was denied or not carried to the next frame, apply_new_text_length+1 may be >= buf_size.
		ImStrncpy(buf, apply_new_text, ImMin(apply_new_text_length + 1, buf_size));
		value_changed = true;
	}

	// Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
	if (clear_active_id && g.ActiveId == id)
		ClearActiveID( );

	// Render frame
	if (!is_multiline)
	{
		RenderNavHighlight(frame_bb, id);
	}

	const ImVec4 clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + inner_size.x, frame_bb.Min.y + inner_size.y); // Not using frame_bb.Max because we have adjusted size
	ImVec2 draw_pos = is_multiline ? draw_window->DC.CursorPos : frame_bb.Min + style.FramePadding;
	ImVec2 text_size(0.0f, 0.0f);

	// Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
	// without any carriage return, which would makes ImFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
	// Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
	const int buf_display_max_length = 2 * 1024 * 1024;
	const char* buf_display = buf_display_from_state ? state->TextA.Data : buf; //-V595
	const char* buf_display_end = NULL; // We have specialized paths below for setting the length
	if (is_displaying_hint)
	{
		buf_display = hint;
		buf_display_end = hint + strlen(hint);
	}

	// Render text. We currently only render selection when the widget is active or while scrolling.
	// FIXME: We could remove the '&& render_cursor' to keep rendering selection when inactive.
	if (render_cursor || render_selection)
	{
		IM_ASSERT(state != NULL);
		if (!is_displaying_hint)
			buf_display_end = buf_display + state->CurLenA;

		// Render text (with cursor and selection)
		// This is going to be messy. We need to:
		// - Display the text (this alone can be more easily clipped)
		// - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
		// - Measure text height (for scrollbar)
		// We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
		// FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
		const ImWchar* text_begin = state->TextW.Data;
		ImVec2 cursor_offset, select_start_offset;

		{
			// Find lines numbers straddling 'cursor' (slot 0) and 'select_start' (slot 1) positions.
			const ImWchar* searches_input_ptr[2] = { NULL, NULL };
			int searches_result_line_no[2] = { -1000, -1000 };
			int searches_remaining = 0;
			if (render_cursor)
			{
				searches_input_ptr[0] = text_begin + state->Stb.cursor;
				searches_result_line_no[0] = -1;
				searches_remaining++;
			}
			if (render_selection)
			{
				searches_input_ptr[1] = text_begin + ImMin(state->Stb.select_start, state->Stb.select_end);
				searches_result_line_no[1] = -1;
				searches_remaining++;
			}

			// Iterate all lines to find our line numbers
			// In multi-line mode, we never exit the loop until all lines are counted, so add one extra to the searches_remaining counter.
			searches_remaining += is_multiline ? 1 : 0;
			int line_count = 0;
			//for (const ImWchar* s = text_begin; (s = (const ImWchar*)wcschr((const wchar_t*)s, (wchar_t)'\n')) != NULL; s++)  // FIXME-OPT: Could use this when wchar_t are 16-bit
			for (const ImWchar* s = text_begin; *s != 0; s++)
				if (*s == '\n')
				{
					line_count++;
					if (searches_result_line_no[0] == -1 && s >= searches_input_ptr[0])
					{
						searches_result_line_no[0] = line_count; if (--searches_remaining <= 0) break;
					}
					if (searches_result_line_no[1] == -1 && s >= searches_input_ptr[1])
					{
						searches_result_line_no[1] = line_count; if (--searches_remaining <= 0) break;
					}
				}
			line_count++;
			if (searches_result_line_no[0] == -1)
				searches_result_line_no[0] = line_count;
			if (searches_result_line_no[1] == -1)
				searches_result_line_no[1] = line_count;

			// Calculate 2d position by finding the beginning of the line and measuring distance
			cursor_offset.x = InputTextCalcTextSizeW(&g, ImStrbolW(searches_input_ptr[0], text_begin), searches_input_ptr[0]).x;
			cursor_offset.y = searches_result_line_no[0] * g.FontSize;
			if (searches_result_line_no[1] >= 0)
			{
				select_start_offset.x = InputTextCalcTextSizeW(&g, ImStrbolW(searches_input_ptr[1], text_begin), searches_input_ptr[1]).x;
				select_start_offset.y = searches_result_line_no[1] * g.FontSize;
			}

			// Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
			if (is_multiline)
				text_size = ImVec2(inner_size.x, line_count * g.FontSize);
		}

		// Scroll
		if (render_cursor && state->CursorFollow)
		{
			// Horizontal scroll in chunks of quarter width
			if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
			{
				const float scroll_increment_x = inner_size.x * 0.25f;
				const float visible_width = inner_size.x - style.FramePadding.x;
				if (cursor_offset.x < state->ScrollX)
					state->ScrollX = IM_FLOOR(ImMax(0.0f, cursor_offset.x - scroll_increment_x));
				else if (cursor_offset.x - visible_width >= state->ScrollX)
					state->ScrollX = IM_FLOOR(cursor_offset.x - visible_width + scroll_increment_x);
			}
			else
			{
				state->ScrollX = 0.0f;
			}

			// Vertical scroll
			if (is_multiline)
			{
				// Test if cursor is vertically visible
				if (cursor_offset.y - g.FontSize < scroll_y)
					scroll_y = ImMax(0.0f, cursor_offset.y - g.FontSize);
				else if (cursor_offset.y - (inner_size.y - style.FramePadding.y * 2.0f) >= scroll_y)
					scroll_y = cursor_offset.y - inner_size.y + style.FramePadding.y * 2.0f;
				const float scroll_max_y = ImMax((text_size.y + style.FramePadding.y * 2.0f) - inner_size.y, 0.0f);
				scroll_y = ImClamp(scroll_y, 0.0f, scroll_max_y);
				draw_pos.y += (draw_window->Scroll.y - scroll_y);   // Manipulate cursor pos immediately avoid a frame of lag
				draw_window->Scroll.y = scroll_y;
			}

			state->CursorFollow = false;
		}

		// Draw selection
		const ImVec2 draw_scroll = ImVec2(state->ScrollX, 0.0f);
		if (render_selection)
		{
			const ImWchar* text_selected_begin = text_begin + ImMin(state->Stb.select_start, state->Stb.select_end);
			const ImWchar* text_selected_end = text_begin + ImMax(state->Stb.select_start, state->Stb.select_end);

			ImU32 bg_color = GetColorU32(ImGuiCol_TextSelectedBg, render_cursor ? 1.0f : 0.6f); // FIXME: current code flow mandate that render_cursor is always true here, we are leaving the transparent one for tests.
			float bg_offy_up = is_multiline ? 0.0f : -1.0f;    // FIXME: those offsets should be part of the style? they don't play so well with multi-line selection.
			float bg_offy_dn = is_multiline ? 0.0f : 2.0f;
			ImVec2 rect_pos = draw_pos + select_start_offset - draw_scroll;
			for (const ImWchar* p = text_selected_begin; p < text_selected_end; )
			{
				if (rect_pos.y > clip_rect.w + g.FontSize)
					break;
				if (rect_pos.y < clip_rect.y)
				{
					//p = (const ImWchar*)wmemchr((const wchar_t*)p, '\n', text_selected_end - p);  // FIXME-OPT: Could use this when wchar_t are 16-bit
					//p = p ? p + 1 : text_selected_end;
					while (p < text_selected_end)
						if (*p++ == '\n')
							break;
				}
				else
				{
					ImVec2 rect_size = InputTextCalcTextSizeW(&g, p, text_selected_end, &p, NULL, true);
					if (rect_size.x <= 0.0f) rect_size.x = IM_FLOOR(g.Font->GetCharAdvance((ImWchar) ' ') * 0.50f); // So we can see selected empty lines
					ImRect rect(rect_pos + ImVec2(0.0f, bg_offy_up - g.FontSize), rect_pos + ImVec2(rect_size.x, bg_offy_dn));
					rect.ClipWith(clip_rect);
					if (rect.Overlaps(clip_rect))
						draw_window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_color);
				}
				rect_pos.x = draw_pos.x - draw_scroll.x;
				rect_pos.y += g.FontSize;
			}
		}

		// We test for 'buf_display_max_length' as a way to avoid some pathological cases (e.g. single-line 1 MB string) which would make ImDrawList crash.
		if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
		{
			ImU32 col = GetColorU32(is_displaying_hint ? ImGuiCol_TextDisabled : ImGuiCol_Text);
			draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos - draw_scroll, col, buf_display, buf_display_end, 0.0f, is_multiline ? NULL : &clip_rect);
		}

		// Draw blinking cursor
		if (render_cursor)
		{
			state->CursorAnim += io.DeltaTime;
			bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (state->CursorAnim <= 0.0f) || ImFmod(state->CursorAnim, 1.20f) <= 0.80f;
			ImVec2 cursor_screen_pos = ImFloor(draw_pos + cursor_offset - draw_scroll);
			ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
			if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
				draw_window->DrawList->AddLine(cursor_screen_rect.Min, cursor_screen_rect.GetBL( ), GetColorU32(ImGuiCol_Text));

			// Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
			if (!is_readonly)
			{
				g.PlatformImeData.WantVisible = true;
				g.PlatformImeData.InputPos = ImVec2(cursor_screen_pos.x - 1.0f, cursor_screen_pos.y - g.FontSize);
				g.PlatformImeData.InputLineHeight = g.FontSize;
			}
		}
	}
	else
	{
		// Render text only (no selection, no cursor)
		if (is_multiline)
			text_size = ImVec2(inner_size.x, InputTextCalcTextLenAndLineCount(buf_display, &buf_display_end) * g.FontSize); // We don't need width
		else if (!is_displaying_hint && g.ActiveId == id)
			buf_display_end = buf_display + state->CurLenA;
		else if (!is_displaying_hint)
			buf_display_end = buf_display + strlen(buf_display);

		if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
		{
			ImU32 col = GetColorU32(is_displaying_hint ? ImGuiCol_TextDisabled : ImGuiCol_Text);
			draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos, col, buf_display, buf_display_end, 0.0f, is_multiline ? NULL : &clip_rect);
		}
	}

	if (is_password && !is_displaying_hint)
		PopFont( );

	if (is_multiline)
	{
		// For focus requests to work on our multiline we need to ensure our child ItemAdd() call specifies the ImGuiItemFlags_Inputable (ref issue #4761)...
		Dummy(ImVec2(text_size.x, text_size.y + style.FramePadding.y));
		ImGuiItemFlags backup_item_flags = g.CurrentItemFlags;
		g.CurrentItemFlags |= ImGuiItemFlags_Inputable | ImGuiItemFlags_NoTabStop;
		EndChild( );
		item_data_backup.StatusFlags |= (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredWindow);
		g.CurrentItemFlags = backup_item_flags;

		// ...and then we need to undo the group overriding last item data, which gets a bit messy as EndGroup() tries to forward scrollbar being active...
		// FIXME: This quite messy/tricky, should attempt to get rid of the child window.
		EndGroup( );
		if (g.LastItemData.ID == 0)
		{
			g.LastItemData.ID = id;
			g.LastItemData.InFlags = item_data_backup.InFlags;
			g.LastItemData.StatusFlags = item_data_backup.StatusFlags;
		}
	}

	// Log as text
	if (g.LogEnabled && (!is_password || is_displaying_hint))
	{
		LogSetNextTextDecoration("{", "}");
		LogRenderedText(&draw_pos, buf_display, buf_display_end);
	}

	if (value_changed && !(flags & ImGuiInputTextFlags_NoMarkEdited))
		MarkItemEdited(id);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
		return enter_pressed;
	else
		return value_changed;
}

bool c_oink_ui::temp_input_text(const ImRect& bb, ImGuiID id, const char* label, char* buf, int buf_size, ImGuiInputTextFlags flags)
{
	// On the first frame, g.TempInputTextId == 0, then on subsequent frames it becomes == id.
	// We clear ActiveID on the first frame to allow the InputText() taking it back.
	ImGuiContext& g = *GImGui;
	const bool init = (g.TempInputId != id);
	if (init)
		ClearActiveID( );

	g.CurrentWindow->DC.CursorPos = bb.Min;

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImGuiStyle& style = g.Style;
	ImVec2 size = ImVec2(186, label_size.y + style.FramePadding.y * 2.0f * m_dpi_scaling);

	bool value_changed = input_text_ex(label, NULL, buf, buf_size, size, flags | ImGuiInputTextFlags_MergedItem, NULL, NULL, m_theme_colour_primary, m_dpi_scaling, false);
	if (init)
	{
		// First frame we started displaying the InputText widget, we expect it to take the active id.
		IM_ASSERT(g.ActiveId == id);
		g.TempInputId = g.ActiveId;
	}
	return value_changed;
};

bool c_oink_ui::input_text(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	set_cursor_pos_x(m_gap);
	IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()
	return input_text_ex(label, NULL, buf, buf_size, ImVec2(0, 0), flags, callback, user_data, m_theme_colour_primary, m_dpi_scaling);
}

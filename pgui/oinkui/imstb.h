#pragma once

#define STB_TEXTEDIT_K_LEFT         0x200000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT        0x200001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           0x200002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         0x200003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    0x200004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      0x200005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    0x200006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      0x200007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       0x200008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    0x200009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         0x20000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO         0x20000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT     0x20000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT    0x20000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_PGUP         0x20000E // keyboard input to move cursor up a page
#define STB_TEXTEDIT_K_PGDOWN       0x20000F // keyboard input to move cursor down a page
#define STB_TEXTEDIT_K_SHIFT        0x400000

#define STB_TEXT_HAS_SELECTION(s)   ((s)->select_start != (s)->select_end)

// Wrapper for stb_textedit.h to edit text (our wrapper is for: statically sized buffer, single-line, wchar characters. InputText converts between UTF-8 and wchar)
namespace ImStb
{
	extern ImWchar STB_TEXTEDIT_NEWLINE;
	extern int     STB_TEXTEDIT_STRINGLEN(const ImGuiInputTextState* obj);
	extern ImWchar STB_TEXTEDIT_GETCHAR(const ImGuiInputTextState* obj, int idx);
	extern float   STB_TEXTEDIT_GETWIDTH(ImGuiInputTextState* obj, int line_start_idx, int char_idx);
	extern int     STB_TEXTEDIT_KEYTOTEXT(int key);
	extern void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, ImGuiInputTextState* obj, int line_start_idx);
	// When ImGuiInputTextFlags_Password is set, we don't want actions such as CTRL+Arrow to leak the fact that underlying data are blanks or separators.
	extern bool is_separator(unsigned int c);
	extern int  is_word_boundary_from_right(ImGuiInputTextState* obj, int idx);
	extern int  is_word_boundary_from_left(ImGuiInputTextState* obj, int idx);
	extern int  STB_TEXTEDIT_MOVEWORDLEFT_IMPL(ImGuiInputTextState* obj, int idx);
	extern int  STB_TEXTEDIT_MOVEWORDRIGHT_MAC(ImGuiInputTextState* obj, int idx);
#define STB_TEXTEDIT_MOVEWORDLEFT   STB_TEXTEDIT_MOVEWORDLEFT_IMPL    // They need to be #define for stb_textedit.h
#ifdef __APPLE__    // FIXME: Move setting to IO structure
#define STB_TEXTEDIT_MOVEWORDRIGHT  STB_TEXTEDIT_MOVEWORDRIGHT_MAC
#else
	extern int  STB_TEXTEDIT_MOVEWORDRIGHT_WIN(ImGuiInputTextState* obj, int idx);
#define STB_TEXTEDIT_MOVEWORDRIGHT  STB_TEXTEDIT_MOVEWORDRIGHT_WIN
#endif

	extern void STB_TEXTEDIT_DELETECHARS(ImGuiInputTextState* obj, int pos, int n);
	extern bool STB_TEXTEDIT_INSERTCHARS(ImGuiInputTextState* obj, int pos, const ImWchar* new_text, int new_text_len);

#define STB_TEXTEDIT_IMPLEMENTATION
#include "../imgui/imstb_textedit.h"

	extern void stb_textedit_replace(ImGuiInputTextState* str, STB_TexteditState* state, const STB_TEXTEDIT_CHARTYPE* text, int text_len);
	extern void stb_textedit_initialize_state(STB_TexteditState* state, int is_single_line);
	extern void stb_textedit_click(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y);
	extern void stb_textedit_prep_selection_at_cursor(STB_TexteditState* state);
	extern void stb_textedit_clamp(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
	extern void stb_textedit_drag(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, float x, float y);
	extern int stb_textedit_cut(STB_TEXTEDIT_STRING* str, STB_TexteditState* state);
	extern int stb_textedit_paste(STB_TEXTEDIT_STRING* str, STB_TexteditState* state, STB_TEXTEDIT_CHARTYPE const* ctext, int len);
} // namespace ImStb

// imgui internals
extern float CalcMaxPopupHeightFromItemCount(int items_count);
extern bool Items_ArrayGetter(void* data, int idx, const char** out_text);
extern const char* PatchFormatStringFloatToInt(const char* fmt);
extern ImVec2 InputTextCalcTextSizeW(const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining = NULL, ImVec2* out_offset = NULL, bool stop_on_new_line = false);
extern int InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end);
extern bool InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data, ImGuiInputSource input_source);

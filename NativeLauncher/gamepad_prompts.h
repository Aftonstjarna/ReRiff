/* Xbox-style prompt badges for the launcher's controller mapping table. */
#ifndef RERIFF_GAMEPAD_PROMPTS_H
#define RERIFF_GAMEPAD_PROMPTS_H

#define RERIFF_PROMPTS 201

enum {
    PROMPT_A, PROMPT_B, PROMPT_X, PROMPT_Y,
    PROMPT_DPAD_UP, PROMPT_DPAD_DOWN, PROMPT_DPAD_LEFT, PROMPT_DPAD_RIGHT,
    PROMPT_LB, PROMPT_RB, PROMPT_LT, PROMPT_RT,
    PROMPT_LS, PROMPT_LS_UP, PROMPT_LS_DOWN, PROMPT_LS_LEFT,
    PROMPT_LS_RIGHT, PROMPT_L3, PROMPT_RS, PROMPT_RS_UP,
    PROMPT_RS_DOWN, PROMPT_RS_LEFT, PROMPT_RS_RIGHT, PROMPT_R3,
    PROMPT_START, PROMPT_BACK, PROMPT_GUIDE, PROMPT_MISC,
    PROMPT_P1, PROMPT_P2, PROMPT_P3, PROMPT_P4, PROMPT_TOUCHPAD,
    PROMPT_COUNT
};

typedef BOOL (WINAPI *PromptBlendFn)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);

static HBITMAP prompt_sheet;
static PromptBlendFn prompt_blend;
static HFONT prompt_font, prompt_small_font;
static BOOL prompt_blend_loaded;

static void prompt_load_blend(void) {
    if (prompt_blend_loaded) return;
    prompt_blend_loaded = TRUE;
    HMODULE library = LoadLibraryW(L"msimg32.dll");
    if (library) prompt_blend = (PromptBlendFn)GetProcAddress(library, "AlphaBlend");
}

static HBITMAP prompt_sheet_load(void) {
    if (!prompt_sheet) prompt_sheet = (HBITMAP)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(RERIFF_PROMPTS),
                                                         IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    return prompt_sheet;
}

static HFONT prompt_font_get(BOOL small) {
    HFONT *slot = small ? &prompt_small_font : &prompt_font;
    if (!*slot) *slot = CreateFontW(small ? -11 : -13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    return *slot;
}

static void prompt_draw_glyph(HDC target, int x, int y, int size, int glyph) {
    HBITMAP sheet = prompt_sheet_load();
    prompt_load_blend();
    if (!sheet || !prompt_blend || glyph < 0 || glyph >= PROMPT_COUNT) return;
    HDC memory = CreateCompatibleDC(target);
    if (!memory) return;
    HGDIOBJ previous = SelectObject(memory, sheet);
    int source_x = (glyph % 8) * 32;
    int source_y = (glyph / 8) * 32;
    BLENDFUNCTION mode = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    prompt_blend(target, x, y, size, size, memory, source_x, source_y, 32, 32, mode);
    SelectObject(memory, previous);
    DeleteDC(memory);
}

static int prompt_button_glyph(int index) {
    static const int table[21] = {
        PROMPT_A, PROMPT_B, PROMPT_X, PROMPT_Y, PROMPT_BACK,
        PROMPT_GUIDE, PROMPT_START, PROMPT_L3, PROMPT_R3, PROMPT_LB,
        PROMPT_RB, PROMPT_DPAD_UP, PROMPT_DPAD_DOWN, PROMPT_DPAD_LEFT,
        PROMPT_DPAD_RIGHT, PROMPT_MISC, PROMPT_P1, PROMPT_P2, PROMPT_P3,
        PROMPT_P4, PROMPT_TOUCHPAD
    };
    if (index < 0 || index >= 21) return -1;
    return table[index];
}

static int prompt_axis_glyph(int index, int sign, BOOL directional) {
    if (index < 0 || index > 5) return -1;
    if (!directional) {
        if (index == 0 || index == 1) return PROMPT_LS;
        if (index == 2 || index == 3) return PROMPT_RS;
        return index == 4 ? PROMPT_LT : PROMPT_RT;
    }
    if (index == 0) return sign < 0 ? PROMPT_LS_LEFT : PROMPT_LS_RIGHT;
    if (index == 1) return sign < 0 ? PROMPT_LS_UP : PROMPT_LS_DOWN;
    if (index == 2) return sign < 0 ? PROMPT_RS_LEFT : PROMPT_RS_RIGHT;
    if (index == 3) return sign < 0 ? PROMPT_RS_UP : PROMPT_RS_DOWN;
    return index == 4 ? PROMPT_LT : PROMPT_RT;
}

static BOOL gamepad_prompt_draw(const DRAWITEMSTRUCT *draw) {
    if (!draw || draw->CtlID < CONTROLLER_CAPTURE_BASE ||
        draw->CtlID >= CONTROLLER_CAPTURE_BASE + CONTROLLER_BINDING_COUNT) return FALSE;
    int row = draw->CtlID - CONTROLLER_CAPTURE_BASE;
    HDC dc = draw->hDC;
    RECT bounds = draw->rcItem;
    FillRect(dc, &bounds, GetSysColorBrush(COLOR_BTNFACE));
    BOOL selected = (draw->itemState & ODS_SELECTED) != 0;
    BOOL disabled = (draw->itemState & ODS_DISABLED) != 0;
    HBRUSH fill = CreateSolidBrush(disabled ? RGB(208,208,208) : selected ? RGB(82,82,82) : RGB(52,52,52));
    HPEN border = CreatePen(PS_SOLID, 1, disabled ? RGB(165,165,165) : RGB(120,120,120));
    HGDIOBJ old_fill = SelectObject(dc, fill), old_border = SelectObject(dc, border);
    RECT chip = bounds;
    InflateRect(&chip, -1, -1);
    RoundRect(dc, chip.left, chip.top, chip.right, chip.bottom, 6, 6);
    SelectObject(dc, old_fill);
    SelectObject(dc, old_border);
    DeleteObject(fill);
    DeleteObject(border);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, disabled ? RGB(100,100,100) : RGB(245,245,245));
    if (controls_capture_row == row || controls_binding_count[row] == 0) {
        const WCHAR *caption = controls_capture_row == row ? L"PRESS A BUTTON" : L"UNBOUND";
        HGDIOBJ old_font = SelectObject(dc, prompt_font_get(TRUE));
        DrawTextW(dc, caption, -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, old_font);
    } else {
        int glyphs[CONTROLS_MAX_CHORD];
        int glyph_count = 0;
        BOOL directional = !controls_controller_axis_row(row);
        for (int i = 0; i < controls_binding_count[row]; i++) {
            int glyph = -1;
            if (controls_binding_kind[row][i] == BINDING_BUTTON)
                glyph = prompt_button_glyph(controls_binding_index[row][i]);
            else if (controls_binding_kind[row][i] == BINDING_AXIS)
                glyph = prompt_axis_glyph(controls_binding_index[row][i], controls_binding_sign[row][i], directional);
            if (glyph >= 0) glyphs[glyph_count++] = glyph;
        }
        WCHAR coords[64];
        size_t used = 0;
        coords[0] = 0;
        for (int i = 0; i < controls_binding_count[row]; i++) {
            _snwprintf(coords + used, 64 - used, L"%ls%ls%d", i ? L"+" : L"",
                       controls_binding_kind[row][i] == BINDING_BUTTON ? L"b" : L"a",
                       controls_binding_index[row][i]);
            used = wcslen(coords);
            if (used >= 64) break;
        }
        int glyph_size = glyph_count >= 4 ? 16 : glyph_count == 3 ? 18 : glyph_count == 2 ? 20 : 22;
        int gap = glyph_count >= 4 ? 2 : 3;
        int x = bounds.left + 5;
        int y = bounds.top + (bounds.bottom - bounds.top - glyph_size) / 2;
        for (int i = 0; i < glyph_count; i++) {
            prompt_draw_glyph(dc, x, y, glyph_size, glyphs[i]);
            x += glyph_size + gap;
        }
        RECT text_bounds = { x + 2, bounds.top, bounds.right - 4, bounds.bottom };
        HGDIOBJ old_font = SelectObject(dc, prompt_font_get(glyph_count >= 3));
        DrawTextW(dc, coords, -1, &text_bounds, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, old_font);
    }
    if (draw->itemState & ODS_FOCUS) {
        HPEN focus = CreatePen(PS_DOT, 1, RGB(240,240,240));
        HGDIOBJ old_pen = SelectObject(dc, focus);
        HGDIOBJ old_brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        RECT outline = bounds;
        InflateRect(&outline, -3, -3);
        Rectangle(dc, outline.left, outline.top, outline.right, outline.bottom);
        SelectObject(dc, old_pen);
        SelectObject(dc, old_brush);
        DeleteObject(focus);
    }
    return TRUE;
}

#endif

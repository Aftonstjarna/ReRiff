/* POC XINPUT1_3 proxy backed by SDL3 gamepads.
 * Implements the four ordinals imported by Riff Racer's Game.exe:
 *   2 XInputGetState, 3 XInputSetState, 5 XInputEnable,
 *   7 XInputGetBatteryInformation.
 *
 * [controller] in client.ini maps each virtual XInput control to a physical
 * SDL gamepad control. Missing keys keep the identity mapping; "none" clears
 * the virtual control.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

#define SDL_INIT_GAMEPAD 0x00002000u
#define MAX_BIND 4

typedef struct {
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
} XINPUT_GAMEPAD;
typedef struct {
    DWORD dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE;
typedef struct {
    WORD wLeftMotorSpeed;
    WORD wRightMotorSpeed;
} XINPUT_VIBRATION;
typedef struct {
    BYTE BatteryType;
    BYTE BatteryLevel;
} XINPUT_BATTERY_INFORMATION;

typedef struct SDL_Gamepad SDL_Gamepad;
typedef uint32_t SDL_JoystickID;

typedef struct {
    HMODULE module;
    int (*Init)(uint32_t);
    void (*Quit)(void);
    void (*UpdateGamepads)(void);
    SDL_JoystickID *(*GetGamepads)(int *);
    void (*Free)(void *);
    SDL_Gamepad *(*OpenGamepad)(SDL_JoystickID);
    void (*CloseGamepad)(SDL_Gamepad *);
    int (*GamepadConnected)(SDL_Gamepad *);
    int (*GetGamepadButton)(SDL_Gamepad *, int);
    int16_t (*GetGamepadAxis)(SDL_Gamepad *, int);
    int (*RumbleGamepad)(SDL_Gamepad *, uint16_t, uint16_t, uint32_t);
    int (*GetGamepadPowerInfo)(SDL_Gamepad *, int *);
    const char *(*GetGamepadName)(SDL_Gamepad *);
    int (*SetHint)(const char *, const char *);
} SdlApi;

enum { BIND_NONE = 0, BIND_BUTTON, BIND_AXIS };
typedef struct {
    int kind;
    int index;
    int sign;
} Binding;
typedef struct {
    Binding item[MAX_BIND];
    int count;
} BindingList;

#define VIRTUAL_BUTTON_COUNT 15
#define PHYSICAL_BUTTON_COUNT 21
static const char *virtual_button_names[VIRTUAL_BUTTON_COUNT] = {
    "a", "b", "x", "y", "back", "guide", "start", "l3", "r3",
    "lb", "rb", "dup", "ddown", "dleft", "dright"
};
/* SDL_GamepadButton order. The first 15 names match the virtual XInput
 * controls, so the launcher can store physical coordinates by name. */
static const char *physical_button_names[PHYSICAL_BUTTON_COUNT] = {
    "a", "b", "x", "y", "back", "guide", "start", "l3", "r3",
    "lb", "rb", "dup", "ddown", "dleft", "dright",
    "misc1", "paddle1", "paddle2", "paddle3", "paddle4", "touchpad"
};
static const WORD button_bits[VIRTUAL_BUTTON_COUNT] = {
    0x1000, 0x2000, 0x4000, 0x8000, 0x0020, 0x0400, 0x0010, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0001, 0x0002, 0x0004, 0x0008
};
static const char *axis_names[6] = { "lx", "ly", "rx", "ry", "lt", "rt" };

static HINSTANCE proxy_instance;
static SdlApi sdl;
static SDL_Gamepad *pads[4];
static int sdl_ready, sdl_attempted, input_enabled = 1, logged_state;
static DWORD packet_number;
static char log_path[MAX_PATH];
static WCHAR dll_dir[MAX_PATH];
static BindingList button_map[15], axis_map[6];
static int mapping_ready, mapping_enabled = 1;

static void write_log(const char *text) {
    if (!log_path[0]) {
        WCHAR path[MAX_PATH];
        DWORD n = GetModuleFileNameW(proxy_instance, path, MAX_PATH);
        if (!n || n >= MAX_PATH) return;
        WCHAR *slash = wcsrchr(path, L'\\');
        if (!slash) return;
        *(slash + 1) = 0;
        wcsncpy(dll_dir, path, MAX_PATH - 1);
        dll_dir[MAX_PATH - 1] = 0;
        wcscat(path, L"logs");
        CreateDirectoryW(path, NULL);
        wcscat(path, L"\\ReRiff-xinput.log");
        WideCharToMultiByte(CP_UTF8, 0, path, -1, log_path, MAX_PATH, NULL, NULL);
    }
    HANDLE file = CreateFileA(log_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(file, text, (DWORD)strlen(text), &written, NULL);
    WriteFile(file, "\r\n", 2, &written, NULL);
    CloseHandle(file);
}

static void log_line(const char *fmt, ...) {
    char line[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    write_log(line);
}

static void *resolve(const char *name) {
    return sdl.module ? (void *)GetProcAddress(sdl.module, name) : NULL;
}

static int name_index(const char *name, const char **names, int count) {
    for (int i = 0; i < count; i++) if (!_stricmp(name, names[i])) return i;
    return -1;
}

static void identity_button(BindingList *list, int index) {
    list->count = 1;
    list->item[0].kind = BIND_BUTTON;
    list->item[0].index = index;
    list->item[0].sign = 1;
}
static void identity_axis(BindingList *list, int index) {
    list->count = 1;
    list->item[0].kind = BIND_AXIS;
    list->item[0].index = index;
    list->item[0].sign = 1;
}

/* Parses either a single binding or a chord such as
 * "button:l3+button:r3". Every part must be active for the chord to count. */
static int parse_binding_part(const char *text, Binding *out) {
    const char *colon = strchr(text, ':');
    if (!colon) return 0;
    char kind[16];
    size_t n = (size_t)(colon - text);
    if (n >= sizeof(kind)) return 0;
    memcpy(kind, text, n);
    kind[n] = 0;
    const char *value = colon + 1;
    int sign = 1;
    size_t len = strlen(value);
    if (len && (value[len - 1] == '+' || value[len - 1] == '-')) {
        if (value[len - 1] == '-') sign = -1;
        len--;
    }
    char name[32];
    if (len >= sizeof(name)) return 0;
    memcpy(name, value, len);
    name[len] = 0;
    int index;
    if (!_stricmp(kind, "button")) {
        index = name_index(name, physical_button_names, PHYSICAL_BUTTON_COUNT);
        if (index < 0) return 0;
        out->kind = BIND_BUTTON;
        out->index = index;
        out->sign = sign;
        return 1;
    }
    if (!_stricmp(kind, "axis")) {
        index = name_index(name, axis_names, 6);
        if (index < 0) return 0;
        out->kind = BIND_AXIS;
        out->index = index;
        out->sign = sign;
        return 1;
    }
    return 0;
}

static void parse_binding(const char *text, BindingList *list) {
    list->count = 0;
    if (!text || !*text) return;
    if (!_stricmp(text, "none")) return;
    const char *cursor = text;
    for (;;) {
        const char *separator = strchr(cursor, '+');
        size_t len = separator ? (size_t)(separator - cursor) : strlen(cursor);
        char part[64];
        if (!len || len >= sizeof(part)) { list->count = 0; return; }
        memcpy(part, cursor, len);
        part[len] = 0;
        if (!parse_binding_part(part, &list->item[list->count])) { list->count = 0; return; }
        list->count++;
        if (!separator || list->count >= MAX_BIND) break;
        cursor = separator + 1;
    }
}

static void load_mapping(void) {
    if (mapping_ready) return;
    mapping_ready = 1;
    for (int i = 0; i < VIRTUAL_BUTTON_COUNT; i++) identity_button(&button_map[i], i);
    for (int i = 0; i < 6; i++) identity_axis(&axis_map[i], i);
    if (!dll_dir[0]) return;
    WCHAR ini[MAX_PATH];
    wcsncpy(ini, dll_dir, MAX_PATH - 1);
    ini[MAX_PATH - 1] = 0;
    wcscat(ini, L"client.ini");
    WCHAR value[128];
    GetPrivateProfileStringW(L"controller", L"enabled", L"1", value, 128, ini);
    mapping_enabled = !_wcsicmp(value, L"1") || !_wcsicmp(value, L"true") || !_wcsicmp(value, L"yes");
    int overrides = 0;
    for (int i = 0; i < VIRTUAL_BUTTON_COUNT; i++) {
        WCHAR key[16];
        MultiByteToWideChar(CP_UTF8, 0, virtual_button_names[i], -1, key, 16);
        DWORD got = GetPrivateProfileStringW(L"controller", key, L"", value, 128, ini);
        char binding[128];
        WideCharToMultiByte(CP_UTF8, 0, value, got + 1, binding, sizeof(binding), NULL, NULL);
        if (got) {
            parse_binding(binding, &button_map[i]);
            log_line("controller %s=%s parts=%d", virtual_button_names[i], binding, button_map[i].count);
            overrides++;
        }
    }
    for (int i = 0; i < 6; i++) {
        WCHAR key[16];
        MultiByteToWideChar(CP_UTF8, 0, axis_names[i], -1, key, 16);
        DWORD got = GetPrivateProfileStringW(L"controller", key, L"", value, 128, ini);
        char binding[128];
        WideCharToMultiByte(CP_UTF8, 0, value, got + 1, binding, sizeof(binding), NULL, NULL);
        if (got) {
            parse_binding(binding, &axis_map[i]);
            log_line("controller %s=%s parts=%d", axis_names[i], binding, axis_map[i].count);
            overrides++;
        }
    }
    log_line("controller mapping enabled=%d overrides=%d", mapping_enabled, overrides);
}

static void load_sdl(void) {
    if (sdl_attempted) return;
    sdl_attempted = 1;
    WCHAR path[MAX_PATH];
    wcsncpy(path, dll_dir, MAX_PATH - 1);
    path[MAX_PATH - 1] = 0;
    wcscat(path, L"SDL3.dll");
    sdl.module = LoadLibraryW(path);
    if (!sdl.module) sdl.module = LoadLibraryW(L"SDL3.dll");
    if (!sdl.module) {
        log_line("SDL3.dll not found");
        return;
    }
    sdl.Init = resolve("SDL_Init");
    sdl.Quit = resolve("SDL_Quit");
    sdl.UpdateGamepads = resolve("SDL_UpdateGamepads");
    sdl.GetGamepads = resolve("SDL_GetGamepads");
    sdl.Free = resolve("SDL_free");
    sdl.OpenGamepad = resolve("SDL_OpenGamepad");
    sdl.CloseGamepad = resolve("SDL_CloseGamepad");
    sdl.GamepadConnected = resolve("SDL_GamepadConnected");
    sdl.GetGamepadButton = resolve("SDL_GetGamepadButton");
    sdl.GetGamepadAxis = resolve("SDL_GetGamepadAxis");
    sdl.RumbleGamepad = resolve("SDL_RumbleGamepad");
    sdl.GetGamepadPowerInfo = resolve("SDL_GetGamepadPowerInfo");
    sdl.GetGamepadName = resolve("SDL_GetGamepadName");
    sdl.SetHint = resolve("SDL_SetHint");
    if (!sdl.Init || !sdl.UpdateGamepads || !sdl.GetGamepads || !sdl.OpenGamepad ||
        !sdl.GetGamepadButton || !sdl.GetGamepadAxis) {
        log_line("SDL3 exports are incomplete");
        return;
    }
    if (sdl.SetHint) {
        sdl.SetHint("SDL_JOYSTICK_XINPUT", "0");
        sdl.SetHint("SDL_JOYSTICK_HIDAPI", "1");
        sdl.SetHint("SDL_JOYSTICK_RAWINPUT", "1");
    }
    if (!sdl.Init(SDL_INIT_GAMEPAD)) {
        log_line("SDL_Init(SDL_INIT_GAMEPAD) failed");
        return;
    }
    sdl_ready = 1;
    log_line("SDL3 gamepad backend ready");
    load_mapping();
}

static void close_pad(int i) {
    if (pads[i] && sdl.CloseGamepad) sdl.CloseGamepad(pads[i]);
    pads[i] = NULL;
}

static void refresh_pads(void) {
    if (!sdl_ready) return;
    sdl.UpdateGamepads();
    for (int i = 0; i < 4; i++) {
        if (pads[i] && sdl.GamepadConnected && !sdl.GamepadConnected(pads[i])) close_pad(i);
    }
    int count = 0;
    SDL_JoystickID *ids = sdl.GetGamepads(&count);
    static int last_count = -1;
    for (int i = 0; i < 4; i++) {
        if (i < count && !pads[i]) {
            pads[i] = sdl.OpenGamepad(ids[i]);
            if (pads[i]) log_line("SDL3 pad %d name=%s", i, sdl.GetGamepadName ? sdl.GetGamepadName(pads[i]) : "?");
        }
        if (i >= count && pads[i]) close_pad(i);
    }
    if (count != last_count) {
        last_count = count;
        log_line("SDL3 gamepads=%d", count);
    }
    if (ids && sdl.Free) sdl.Free(ids);
}

static int physical_button(SDL_Gamepad *pad, int index) {
    return sdl.GetGamepadButton(pad, index) ? 1 : 0;
}
static int physical_axis(SDL_Gamepad *pad, int index) {
    return sdl.GetGamepadAxis(pad, index);
}

static int binding_active(SDL_Gamepad *pad, const Binding *bind, int virtual_axis) {
    if (bind->kind == BIND_BUTTON) return physical_button(pad, bind->index);
    if (bind->kind == BIND_AXIS) {
        int value = physical_axis(pad, bind->index) * bind->sign;
        if (virtual_axis >= 4) return value > 16384;
        return value > 8192;
    }
    return 0;
}

static int virtual_button(SDL_Gamepad *pad, int index) {
    if (!mapping_enabled) return physical_button(pad, index);
    if (!button_map[index].count) return 0;
    for (int i = 0; i < button_map[index].count; i++) {
        if (!binding_active(pad, &button_map[index].item[i], 0)) return 0;
    }
    return 1;
}

static int virtual_axis(SDL_Gamepad *pad, int index) {
    if (!mapping_enabled) return physical_axis(pad, index);
    if (!axis_map[index].count) return 0;
    const Binding *bind = &axis_map[index].item[0];
    if (bind->kind == BIND_BUTTON) return physical_button(pad, bind->index) ? 32767 * bind->sign : 0;
    if (bind->kind == BIND_AXIS) {
        int value = physical_axis(pad, bind->index) * bind->sign;
        if (index >= 4) {
            if (bind->index >= 4) return value < 0 ? 0 : value;
            return value < 0 ? 0 : value;
        }
        return value;
    }
    return 0;
}

static BYTE trigger_value(int axis) {
    if (axis < 0) axis = 0;
    if (axis > 32767) axis = 32767;
    return (BYTE)((axis * 255) / 32767);
}
static SHORT stick_value(int axis) {
    if (axis > 32767) axis = 32767;
    if (axis < -32768) axis = -32768;
    return (SHORT)axis;
}

static void keyboard_init(void);

__declspec(dllexport) DWORD WINAPI XInputGetState(DWORD index, XINPUT_STATE *state) {
    if (!state) return ERROR_INVALID_PARAMETER;
    keyboard_init();
    if (!input_enabled) return ERROR_DEVICE_NOT_CONNECTED;
    load_sdl();
    if (!sdl_ready) return ERROR_DEVICE_NOT_CONNECTED;
    refresh_pads();
    if (index >= 4 || !pads[index]) return ERROR_DEVICE_NOT_CONNECTED;
    memset(state, 0, sizeof(*state));
    state->dwPacketNumber = ++packet_number;
    WORD buttons = 0;
    for (int i = 0; i < VIRTUAL_BUTTON_COUNT; i++) if (virtual_button(pads[index], i)) buttons |= button_bits[i];
    state->Gamepad.wButtons = buttons;
    state->Gamepad.bLeftTrigger = trigger_value(virtual_axis(pads[index], 4));
    state->Gamepad.bRightTrigger = trigger_value(virtual_axis(pads[index], 5));
    state->Gamepad.sThumbLX = stick_value(virtual_axis(pads[index], 0));
    /* SDL's Y axes grow downward; XInput's grow upward. */
    state->Gamepad.sThumbLY = stick_value(-virtual_axis(pads[index], 1));
    state->Gamepad.sThumbRX = stick_value(virtual_axis(pads[index], 2));
    state->Gamepad.sThumbRY = stick_value(-virtual_axis(pads[index], 3));
    if (!logged_state) {
        logged_state = 1;
        log_line("first state buttons=%04X lt=%u rt=%u lx=%d ly=%d rx=%d ry=%d",
                 state->Gamepad.wButtons, state->Gamepad.bLeftTrigger, state->Gamepad.bRightTrigger,
                 state->Gamepad.sThumbLX, state->Gamepad.sThumbLY,
                 state->Gamepad.sThumbRX, state->Gamepad.sThumbRY);
    }
    return ERROR_SUCCESS;
}

__declspec(dllexport) DWORD WINAPI XInputSetState(DWORD index, XINPUT_VIBRATION *vibration) {
    if (!vibration) return ERROR_INVALID_PARAMETER;
    load_sdl();
    if (!sdl_ready) return ERROR_DEVICE_NOT_CONNECTED;
    refresh_pads();
    if (index >= 4 || !pads[index] || !sdl.RumbleGamepad) return ERROR_DEVICE_NOT_CONNECTED;
    sdl.RumbleGamepad(pads[index], vibration->wLeftMotorSpeed, vibration->wRightMotorSpeed, 500);
    return ERROR_SUCCESS;
}

__declspec(dllexport) void WINAPI XInputEnable(BOOL enable) {
    input_enabled = enable ? 1 : 0;
}

__declspec(dllexport) DWORD WINAPI XInputGetBatteryInformation(DWORD index, BYTE type,
                                                              XINPUT_BATTERY_INFORMATION *battery) {
    if (!battery) return ERROR_INVALID_PARAMETER;
    if (index >= 4) return ERROR_DEVICE_NOT_CONNECTED;
    memset(battery, 0, sizeof(*battery));
    battery->BatteryType = 1;
    battery->BatteryLevel = 3;
    if (type == 0) {
        load_sdl();
        refresh_pads();
        if (index < 4 && pads[index] && sdl.GetGamepadPowerInfo) {
            int percent = 0;
            int power = sdl.GetGamepadPowerInfo(pads[index], &percent);
            if (power == 1 || power == 3 || power == 4) {
                battery->BatteryType = 0;
                battery->BatteryLevel = percent > 75 ? 3 : percent > 40 ? 2 : percent > 10 ? 1 : 0;
            }
        }
    }
    return ERROR_SUCCESS;
}

__declspec(dllexport) DWORD WINAPI XInputGetCapabilities(DWORD index, DWORD flags, void *caps) {
    (void)index; (void)flags; (void)caps;
    return ERROR_DEVICE_NOT_CONNECTED;
}
__declspec(dllexport) DWORD WINAPI XInputGetKeystroke(DWORD index, DWORD flags, void *keystroke) {
    (void)index; (void)flags; (void)keystroke;
    return ERROR_DEVICE_NOT_CONNECTED;
}
__declspec(dllexport) DWORD WINAPI XInputGetStateEx(DWORD index, XINPUT_STATE *state) {
    return XInputGetState(index, state);
}
__declspec(dllexport) DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD index, void *render, void *capture) {
    (void)index; (void)render; (void)capture;
    return ERROR_DEVICE_NOT_CONNECTED;
}


static void keyboard_init(void);

/* ------------------------------------------------------------------ */
/* Keyboard virtual mapping                                            */
/* ------------------------------------------------------------------ */
#define MAX_KEYBOARD_MAP 24
typedef struct {
    int physical;      /* physical VK, or -1 for explicitly unbound */
    int virtual_key;   /* vanilla VK the game polls */
} KeyboardMap;

static KeyboardMap keyboard_map[MAX_KEYBOARD_MAP];
static int keyboard_map_count;
static unsigned char keyboard_virtual_mapped[256];
static unsigned char keyboard_physical_used[256];
static unsigned char keyboard_edge[256];
static int keyboard_enabled = 1, keyboard_has_remap, keyboard_ready;
static volatile LONG keyboard_started;
typedef SHORT (WINAPI *GetAsyncKeyStateProc)(int);
typedef SHORT (WINAPI *GetKeyStateProc)(int);
static GetAsyncKeyStateProc real_get_async_key_state;
static GetKeyStateProc real_get_key_state;

static const struct { const char *name; int vk; } keyboard_actions[] = {
    { "accelerate", VK_UP },
    { "brake", VK_DOWN },
    { "left", VK_LEFT },
    { "right", VK_RIGHT },
    { "boost", VK_SHIFT },
    { "drift", VK_SPACE },
    { "camera", VK_TAB },
    { "confirm", VK_RETURN },
    { "pause", VK_ESCAPE },
    { "restart", 'R' }
};

static int parse_keyboard_value(const WCHAR *text) {
    if (!text || !*text || !_wcsicmp(text, L"none")) return -1;
    const WCHAR *number = text;
    if (!_wcsnicmp(number, L"key:", 4)) number += 4;
    WCHAR *end = NULL;
    unsigned long value = wcstoul(number, &end, 0);
    if (!end || *end || value == 0 || value > 255) return -2;
    return (int)value;
}

static void load_keyboard_mapping(void) {
    memset(keyboard_map, 0, sizeof(keyboard_map));
    memset(keyboard_virtual_mapped, 0, sizeof(keyboard_virtual_mapped));
    memset(keyboard_physical_used, 0, sizeof(keyboard_physical_used));
    memset(keyboard_edge, 0, sizeof(keyboard_edge));
    keyboard_map_count = 0;
    keyboard_has_remap = 0;
    if (!dll_dir[0]) return;
    WCHAR ini[MAX_PATH];
    wcsncpy(ini, dll_dir, MAX_PATH - 1);
    ini[MAX_PATH - 1] = 0;
    wcscat(ini, L"client.ini");
    WCHAR value[64];
    GetPrivateProfileStringW(L"keyboard", L"enabled", L"1", value, 64, ini);
    keyboard_enabled = !_wcsicmp(value, L"1") || !_wcsicmp(value, L"true") || !_wcsicmp(value, L"yes");
    int overrides = 0;
    for (unsigned i = 0; i < sizeof(keyboard_actions) / sizeof(keyboard_actions[0]); i++) {
        WCHAR key[32];
        MultiByteToWideChar(CP_UTF8, 0, keyboard_actions[i].name, -1, key, 32);
        DWORD got = GetPrivateProfileStringW(L"keyboard", key, L"", value, 64, ini);
        if (!got) continue;
        int physical = parse_keyboard_value(value);
        if (physical == -2) continue;
        if (keyboard_map_count < MAX_KEYBOARD_MAP) {
            keyboard_map[keyboard_map_count].physical = physical;
            keyboard_map[keyboard_map_count].virtual_key = keyboard_actions[i].vk;
            keyboard_map_count++;
            overrides++;
            log_line("keyboard %s physical=%d virtual=%d", keyboard_actions[i].name, physical, keyboard_actions[i].vk);
        }
    }
    for (int i = 0; i < keyboard_map_count; i++) {
        int physical = keyboard_map[i].physical;
        int virtual_key = keyboard_map[i].virtual_key;
        keyboard_virtual_mapped[virtual_key] = 1;
        if (physical < 0) {
            keyboard_physical_used[virtual_key] = 1;
            keyboard_has_remap = 1;
        } else if (physical != virtual_key) {
            keyboard_physical_used[physical] = 1;
            keyboard_has_remap = 1;
        }
    }
    log_line("keyboard mapping enabled=%d entries=%d remap=%d", keyboard_enabled, overrides, keyboard_has_remap);
}

static SHORT WINAPI keyboard_get_async_key_state(int key) {
    if (!keyboard_ready || !keyboard_enabled || key < 0 || key > 255)
        return real_get_async_key_state ? real_get_async_key_state(key) : 0;
    if (keyboard_virtual_mapped[key]) {
        int down = 0;
        for (int i = 0; i < keyboard_map_count; i++) {
            if (keyboard_map[i].virtual_key != key || keyboard_map[i].physical < 0) continue;
            if (real_get_async_key_state(keyboard_map[i].physical) & 0x8000) {
                down = 1;
                break;
            }
        }
        if (!down) {
            keyboard_edge[key] = 0;
            return 0;
        }
        SHORT state = (SHORT)0x8000;
        if (!keyboard_edge[key]) state |= 1;
        keyboard_edge[key] = 1;
        return state;
    }
    if (keyboard_physical_used[key]) return 0;
    return real_get_async_key_state(key);
}

static SHORT WINAPI keyboard_get_key_state(int key) {
    if (!keyboard_ready || !keyboard_enabled || key < 0 || key > 255)
        return real_get_key_state ? real_get_key_state(key) : 0;
    if (keyboard_virtual_mapped[key]) {
        for (int i = 0; i < keyboard_map_count; i++) {
            if (keyboard_map[i].virtual_key != key || keyboard_map[i].physical < 0) continue;
            if (real_get_async_key_state(keyboard_map[i].physical) & 0x8000) return (SHORT)0x8000;
        }
        return 0;
    }
    if (keyboard_physical_used[key]) return 0;
    return real_get_key_state(key);
}

static int patch_import_function(HMODULE module, const char *dll_name, const char *function_name, void *replacement) {
    if (!module) return 0;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)module;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((BYTE *)module + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
    IMAGE_DATA_DIRECTORY *directory = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!directory->VirtualAddress) return 0;
    IMAGE_IMPORT_DESCRIPTOR *descriptor = (IMAGE_IMPORT_DESCRIPTOR *)((BYTE *)module + directory->VirtualAddress);
    for (; descriptor->Name; descriptor++) {
        const char *module_name = (const char *)((BYTE *)module + descriptor->Name);
        if (_stricmp(module_name, dll_name)) continue;
        if (!descriptor->OriginalFirstThunk || !descriptor->FirstThunk) continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)((BYTE *)module + descriptor->OriginalFirstThunk);
        IMAGE_THUNK_DATA *iat = (IMAGE_THUNK_DATA *)((BYTE *)module + descriptor->FirstThunk);
        for (; names->u1.AddressOfData; names++, iat++) {
            if (names->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
            IMAGE_IMPORT_BY_NAME *import = (IMAGE_IMPORT_BY_NAME *)((BYTE *)module + names->u1.AddressOfData);
            if (strcmp((const char *)import->Name, function_name)) continue;
            DWORD old_protect;
            if (!VirtualProtect(&iat->u1.Function, sizeof(iat->u1.Function), PAGE_READWRITE, &old_protect)) return 0;
            iat->u1.Function = (DWORD)(DWORD_PTR)replacement;
            VirtualProtect(&iat->u1.Function, sizeof(iat->u1.Function), old_protect, &old_protect);
            return 1;
        }
    }
    return 0;
}

static void bind_keyboard_export(void *target, size_t size, HMODULE module, const char *name) {
    FARPROC symbol = GetProcAddress(module, name);
    memset(target, 0, size);
    memcpy(target, &symbol, size < sizeof(symbol) ? size : sizeof(symbol));
}

static void keyboard_init(void) {
    if (InterlockedCompareExchange(&keyboard_started, 1, 0) != 0) return;
    load_keyboard_mapping();
    keyboard_ready = 1;
    if (!keyboard_enabled || !keyboard_has_remap) return;
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) user32 = LoadLibraryW(L"user32.dll");
    if (!user32) {
        log_line("keyboard hook: user32.dll not available");
        return;
    }
    bind_keyboard_export(&real_get_async_key_state, sizeof(real_get_async_key_state), user32, "GetAsyncKeyState");
    bind_keyboard_export(&real_get_key_state, sizeof(real_get_key_state), user32, "GetKeyState");
    if (!real_get_async_key_state || !real_get_key_state) {
        log_line("keyboard hook: key state exports not found");
        return;
    }
    HMODULE main = GetModuleHandleW(NULL);
    int async_hooked = patch_import_function(main, "USER32.dll", "GetAsyncKeyState", (void *)keyboard_get_async_key_state);
    int key_hooked = patch_import_function(main, "USER32.dll", "GetKeyState", (void *)keyboard_get_key_state);
    log_line("keyboard IAT hooked async=%d key=%d", async_hooked, key_hooked);
}

static DWORD WINAPI keyboard_init_thread(LPVOID parameter) {
    (void)parameter;
    keyboard_init();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        proxy_instance = instance;
        DisableThreadLibraryCalls(instance);
        write_log("XINPUT1_3 SDL3 proxy loaded");
        CreateThread(NULL, 0, keyboard_init_thread, NULL, 0, NULL);
    }
    return TRUE;
}

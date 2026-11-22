/* POC XINPUT1_3 proxy backed by SDL3 gamepads.
 * Implements the four ordinals imported by Riff Racer's Game.exe:
 *   2 XInputGetState, 3 XInputSetState, 5 XInputEnable,
 *   7 XInputGetBatteryInformation.
 * Identity mapping for now; the remap table will go here later.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define SDL_INIT_GAMEPAD 0x00002000u

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
    int (*GamepadGetButton)(SDL_Gamepad *, int);
    int16_t (*GamepadGetAxis)(SDL_Gamepad *, int);
    int (*RumbleGamepad)(SDL_Gamepad *, uint16_t, uint16_t, uint32_t);
    int (*GetGamepadPowerInfo)(SDL_Gamepad *, int *);
    const char *(*GetGamepadName)(SDL_Gamepad *);
    int (*SetHint)(const char *, const char *);
} SdlApi;

static HINSTANCE proxy_instance;
static SdlApi sdl;
static SDL_Gamepad *pads[4];
static int sdl_ready, sdl_attempted, input_enabled = 1, logged_state;
static DWORD packet_number;
static char log_path[MAX_PATH];
static WCHAR dll_dir[MAX_PATH];

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
    sdl.GamepadGetButton = resolve("SDL_GetGamepadButton");
    sdl.GamepadGetAxis = resolve("SDL_GetGamepadAxis");
    sdl.RumbleGamepad = resolve("SDL_RumbleGamepad");
    sdl.GetGamepadPowerInfo = resolve("SDL_GetGamepadPowerInfo");
    sdl.GetGamepadName = resolve("SDL_GetGamepadName");
    sdl.SetHint = resolve("SDL_SetHint");
    if (!sdl.Init || !sdl.UpdateGamepads || !sdl.GetGamepads || !sdl.OpenGamepad ||
        !sdl.GamepadGetButton || !sdl.GamepadGetAxis) {
        log_line("SDL3 exports are incomplete");
        return;
    }
    /* Keep SDL away from XInput, which would recurse through this proxy. */
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

static WORD buttons(SDL_Gamepad *pad) {
    WORD b = 0;
    if (sdl.GamepadGetButton(pad, 0)) b |= 0x1000;  /* south / A */
    if (sdl.GamepadGetButton(pad, 1)) b |= 0x2000;  /* east / B */
    if (sdl.GamepadGetButton(pad, 2)) b |= 0x4000;  /* west / X */
    if (sdl.GamepadGetButton(pad, 3)) b |= 0x8000;  /* north / Y */
    if (sdl.GamepadGetButton(pad, 4)) b |= 0x0020;  /* back */
    if (sdl.GamepadGetButton(pad, 5)) b |= 0x0400;  /* guide */
    if (sdl.GamepadGetButton(pad, 6)) b |= 0x0010;  /* start */
    if (sdl.GamepadGetButton(pad, 7)) b |= 0x0040;  /* left stick */
    if (sdl.GamepadGetButton(pad, 8)) b |= 0x0080;  /* right stick */
    if (sdl.GamepadGetButton(pad, 9)) b |= 0x0100;  /* left shoulder */
    if (sdl.GamepadGetButton(pad, 10)) b |= 0x0200; /* right shoulder */
    if (sdl.GamepadGetButton(pad, 11)) b |= 0x0001; /* dpad up */
    if (sdl.GamepadGetButton(pad, 12)) b |= 0x0002; /* dpad down */
    if (sdl.GamepadGetButton(pad, 13)) b |= 0x0004; /* dpad left */
    if (sdl.GamepadGetButton(pad, 14)) b |= 0x0008; /* dpad right */
    return b;
}

static BYTE trigger(SDL_Gamepad *pad, int axis) {
    int value = sdl.GamepadGetAxis(pad, axis);
    if (value < 0) value = 0;
    return (BYTE)((value * 255) / 32767);
}

static SHORT stick(SDL_Gamepad *pad, int axis) {
    int value = sdl.GamepadGetAxis(pad, axis);
    if (value > 32767) value = 32767;
    if (value < -32768) value = -32768;
    return (SHORT)value;
}

__declspec(dllexport) DWORD WINAPI XInputGetState(DWORD index, XINPUT_STATE *state) {
    if (!state) return ERROR_INVALID_PARAMETER;
    if (!input_enabled) return ERROR_DEVICE_NOT_CONNECTED;
    load_sdl();
    if (!sdl_ready) return ERROR_DEVICE_NOT_CONNECTED;
    refresh_pads();
    if (index >= 4 || !pads[index]) return ERROR_DEVICE_NOT_CONNECTED;
    memset(state, 0, sizeof(*state));
    state->dwPacketNumber = ++packet_number;
    state->Gamepad.wButtons = buttons(pads[index]);
    state->Gamepad.bLeftTrigger = trigger(pads[index], 4);
    state->Gamepad.bRightTrigger = trigger(pads[index], 5);
    state->Gamepad.sThumbLX = stick(pads[index], 0);
    state->Gamepad.sThumbLY = stick(pads[index], 1);
    state->Gamepad.sThumbRX = stick(pads[index], 2);
    state->Gamepad.sThumbRY = stick(pads[index], 3);
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
    battery->BatteryType = 1;  /* wired */
    battery->BatteryLevel = 3; /* full */
    if (type == 0) {
        load_sdl();
        refresh_pads();
        if (index < 4 && pads[index] && sdl.GetGamepadPowerInfo) {
            int percent = 0;
            int power = sdl.GetGamepadPowerInfo(pads[index], &percent);
            if (power == 1 || power == 3 || power == 4) {
                battery->BatteryType = 0; /* battery */
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

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        proxy_instance = instance;
        DisableThreadLibraryCalls(instance);
        write_log("XINPUT1_3 SDL3 proxy loaded");
    }
    return TRUE;
}

#include "mgs_native.h"
#include "mgs_controller_ports.h"
#include "mgs_input_movie.h"
#include "mgs_ending_debug.h"
#include "mgs_mantis_device.h"
#include "game/game.h"
#include <psyz/input.h>
#include <psyz/overlay_sdl3.h>

static PsyzOverlayEventCB_SDL3 previous_event;
static SDL_JoystickID chord_controller;
static unsigned mantis_deaths;
static int death_active, shortcut_used;
static unsigned ending_playback, ending_skip;
static PsyzInputDeviceInfo mantis_baseline;
static unsigned active_kind, active_id;
static int mantis_tracking, automatic_resistance, manual_override;
static struct {
    SDL_JoystickID id;
    unsigned held;
    unsigned axes;
} mantis_pads[16];

static void RememberPad(SDL_JoystickID id)
{
    for (int i = 0; i < 16; ++i) {
        if (mantis_pads[i].id == id) return;
        if (mantis_pads[i].id) continue;
        SDL_Gamepad *pad = SDL_OpenGamepad(id);
        if (!pad) return;
        mantis_pads[i].id = id;
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b)
            if (SDL_GetGamepadButton(pad, b)) mantis_pads[i].held |= 1u << b;
        for (int a = 0; a < 6; ++a) {
            int value = SDL_GetGamepadAxis(pad, a);
            if (value >= -8000 && value <= 8000) mantis_pads[i].axes |= 1u << a;
        }
        SDL_CloseGamepad(pad);
        return;
    }
}

static void ResetMantisInput(void)
{
    if (mantis_tracking) {
        Psyz_PadsSelectDevice(0, 0);
        if (!manual_override) Psyz_PadsSetHostPortSwap(0);
    }
    mantis_baseline = (PsyzInputDeviceInfo){0};
    active_kind = active_id = 0;
    automatic_resistance = mantis_tracking = 0;
    memset(mantis_pads, 0, sizeof(mantis_pads));
}


static int EndingArea(void)
{
    const char *area = GM_GetArea(0);
    return area && (!strcmp(area, "ending") || !strcmp(area, "endingr") || !strcmp(area, "roll"));
}

void MgsEndingDebugPlayback(int kind, int active)
{
    ending_playback &= ~(unsigned)kind;
    ending_skip &= ~(unsigned)kind;
    if (active && EndingArea()) ending_playback |= (unsigned)kind;
}

int MgsEndingDebugTakeSkip(int kind)
{
    int requested = (ending_skip & (unsigned)kind) != 0;
    ending_skip &= ~(unsigned)kind;
    return requested && EndingArea() && !MgsInputMovieReplaying();
}

static int RequestEndingSkip(void)
{
    if (!ending_playback || !EndingArea()) return 0;
    /* One registered movie is consumed. Never carry a request into its successor. */
    ending_skip |= ending_playback & MGS_ENDING_DEBUG_MOVIE ? MGS_ENDING_DEBUG_MOVIE : MGS_ENDING_DEBUG_ROLL;
    puts("[debug] Skipping this ending video through its original completion callback.");
    return 1;
}

static int PortShortcut(const SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN)
        return !event->key.repeat && event->key.scancode == SDL_SCANCODE_F2;
    if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        if (event->gdevice.which == chord_controller)
            chord_controller = 0;
        return 0;
    }
    if ((event->type != SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
         event->type != SDL_EVENT_GAMEPAD_BUTTON_UP) ||
        (event->gbutton.button != SDL_GAMEPAD_BUTTON_LEFT_STICK &&
         event->gbutton.button != SDL_GAMEPAD_BUTTON_RIGHT_STICK))
        return 0;
    if (event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
        if (event->gbutton.which == chord_controller)
            chord_controller = 0;
        return 0;
    }
    SDL_Gamepad *pad = SDL_GetGamepadFromID(event->gbutton.which);
    if (!pad || chord_controller == event->gbutton.which ||
        !SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_STICK) ||
        !SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_STICK))
        return 0;
    chord_controller = event->gbutton.which;
    return 1;
}

void MgsControllerPortsUpdate(void)
{
    const char *area = GM_GetArea(0);
    int mantis = area && !strcmp(area, "s07b");
    if (!mantis || GM_GameOverTimer > 0 || MgsInputMovieReplaying()) ResetMantisInput();
    else if (!mantis_tracking && (GM_PlayerStatus & PLAYER_SECOND_AVAILABLE)) {
        mantis_tracking = 1;
        int count = 0;
        SDL_JoystickID *pads = SDL_GetGamepads(&count);
        for (int i = 0; i < count; ++i) RememberPad(pads[i]);
        SDL_free(pads);
    }
    /* Negative timer states also occur during Meryl's nonfatal scene. Count
     * only an actual Game Over countdown toward the delayed port hint. */
    int dying = mantis && GM_GameOverTimer > 0;
    if (!mantis) {
        mantis_deaths = 0;
        death_active = 0;
    } else {
        if (dying && !death_active &&
            (GM_PlayerStatus & PLAYER_SECOND_AVAILABLE) && mantis_deaths < 2)
            ++mantis_deaths;
        death_active = dying;
    }
    /* The platform consumes device events after this callback. Limit automatic
     * replacement routing to scenes that support the second controller. */
    SDL_SetHint("REI_CONTROLLER_IDENTITY_PORTS",
        "0");
}

static int GameplayKey(SDL_Scancode key)
{
    switch (key) {
    case SDL_SCANCODE_UP: case SDL_SCANCODE_DOWN:
    case SDL_SCANCODE_LEFT: case SDL_SCANCODE_RIGHT:
    case SDL_SCANCODE_W: case SDL_SCANCODE_E: case SDL_SCANCODE_Q: case SDL_SCANCODE_R:
    case SDL_SCANCODE_S: case SDL_SCANCODE_D: case SDL_SCANCODE_X: case SDL_SCANCODE_Z:
    case SDL_SCANCODE_BACKSPACE: case SDL_SCANCODE_RETURN: return 1;
    default: return 0;
    }
}

static void MantisDeviceEvent(const SDL_Event *event)
{
    if (!mantis_tracking) return;
    if (event->type == SDL_EVENT_GAMEPAD_ADDED) RememberPad(event->gdevice.which);
    if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        for (int i = 0; i < 16; ++i)
            if (mantis_pads[i].id == event->gdevice.which) memset(&mantis_pads[i], 0, sizeof(mantis_pads[i]));
        return;
    }
    unsigned kind = 0, id = 0;
    if (event->type == SDL_EVENT_KEY_DOWN && GameplayKey(event->key.scancode) &&
        !(event->key.mod & (SDL_KMOD_ALT | SDL_KMOD_CTRL | SDL_KMOD_GUI)) &&
        Psyz_PadsKeyboardFresh(event->key.which, event->key.scancode)) {
        kind = 1; id = event->key.which;
    } else if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
               event->type == SDL_EVENT_GAMEPAD_BUTTON_UP ||
               event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        SDL_JoystickID pad_id = event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION ?
            event->gaxis.which : event->gbutton.which;
        for (int i = 0; i < 16; ++i) {
            if (mantis_pads[i].id != pad_id) continue;
            if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
                unsigned axis = event->gaxis.axis;
                if (axis >= 6) break;
                int value = event->gaxis.value;
                if (value >= -8000 && value <= 8000) mantis_pads[i].axes |= 1u << axis;
                else if ((value > 16000 || value < -16000) && (mantis_pads[i].axes & (1u << axis))) {
                    mantis_pads[i].axes &= ~(1u << axis); kind = 2; id = pad_id;
                }
            } else {
                unsigned button = event->gbutton.button;
                if (button >= SDL_GAMEPAD_BUTTON_COUNT) break;
                unsigned bit = 1u << button;
                if (event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) mantis_pads[i].held &= ~bit;
                else {
                    int fresh = !(mantis_pads[i].held & bit);
                    mantis_pads[i].held |= bit;
                    if (fresh && button != SDL_GAMEPAD_BUTTON_LEFT_STICK && button != SDL_GAMEPAD_BUTTON_RIGHT_STICK) {
                        kind = 2; id = pad_id;
                    }
                }
            }
            break;
        }
    }
    /* Pauses and temporary script locks preserve identity but cannot select it. */
    if (!kind || !(GM_PlayerStatus & PLAYER_SECOND_AVAILABLE) || GV_PauseLevel ||
        (GM_GameStatus & (STATE_DEMO | STATE_PADDEMO))) return;
    PsyzInputDeviceInfo current;
    if (!Psyz_PadsDeviceInfo(kind, id, &current)) return;
    if (!mantis_baseline.kind) mantis_baseline = current;
    int connected = Psyz_PadsDeviceConnected(&mantis_baseline);
    automatic_resistance = MgsMantisDifferentDevice(&mantis_baseline, &current, connected);
    active_kind = kind; active_id = id;
    Psyz_PadsSelectDevice(kind, id);
    if (!manual_override) Psyz_PadsSetHostPortSwap(automatic_resistance);
}

static void ControllerEvent(const SDL_Event *event)
{
    MgsControllerPortsUpdate();
    if (previous_event)
        previous_event(event);
    MantisDeviceEvent(event);
    if (MgsInputMovieReplaying() || !PortShortcut(event))
        return;
    if (RequestEndingSkip())
        return;
    shortcut_used = 1;
    int swapped = !Psyz_PadsHostPort(0);
    manual_override = swapped;
    Psyz_PadsSetHostPortSwap(swapped);
    printf("[input] Primary controller and keyboard moved to port %d. F2 or L3+R3 switches ports.\n",
           swapped + 1);
}

void MgsControllerPortsInit(void)
{
    Psyz_PadsSetHostPortSwap(0);
    chord_controller = 0;
    ending_playback = ending_skip = 0;
    mantis_deaths = 0;
    manual_override = 0;
    ResetMantisInput();
    Psyz_PadsSelectDevice(0, 0);
    death_active = shortcut_used = 0;
    SDL_SetHint("REI_CONTROLLER_IDENTITY_PORTS", "0");
    previous_event = Psyz_OverlayEvent_SDL3(ControllerEvent);
}

const char *MgsControllerPortsHint(void)
{
    const char *area = GM_GetArea(0);
    int mantis = area && !strcmp(area, "s07b");
    int available = mantis && (GM_PlayerStatus & PLAYER_SECOND_AVAILABLE) != 0;
    int swapped = Psyz_PadsHostPort(0) != 0;
    /* A manual swap persists until the player reverses it. Keep the return
     * prompt after Mantis, even when the encounter's hint is otherwise hidden. */
    if (!swapped && (!available || (!shortcut_used && mantis_deaths < 2)))
        return "";
    if (MgsInputMovieReplaying()) {
        if (!available)
            return "";
        return (GM_PlayerStatus & PLAYER_SECOND_CONTROLLER) ?
            "REPLAY - CONTROLLER PORT 2" : "REPLAY - CONTROLLER PORT 1";
    }
    if (mantis_tracking && !manual_override && automatic_resistance)
        return "DIFFERENT INPUT DEVICE\nORIGINAL DEVICE/PORT: RESTORE READING";
    if (swapped ||
        (available && (GM_PlayerStatus & PLAYER_SECOND_CONTROLLER)))
        return "CONTROLLER PORT 2\nF2/L3+R3: RETURN TO PORT 1";
    return available ? "CONTROLLER PORT 1\nF2/L3+R3: SWITCH TO PORT 2" : "";
}

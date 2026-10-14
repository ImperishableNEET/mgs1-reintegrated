#include "mgs_native.h"
#undef fprintf
#include "game/game.h"
#include "mgs_controller_ports.h"
#include "mgs_ending_debug.h"
#include "mgs_mantis_device.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <libapi.h>
#include <libpad.h>
#include <libspu.h>
#include <psyz/input.h>
#include <psyz/system.h>

static Uint16 rumble_low, rumble_high;
static int rumble_calls;
static int replaying;
int GM_PlayerStatus;
int GM_GameOverTimer;
int GM_GameStatus;
int GV_PauseLevel;
int ReiReintegratedTitleVisible;
static char test_area[8];
char *GM_GetArea(int flag) { (void)flag; return test_area; }
static void SetTestArea(const char *area)
{
    snprintf(test_area, sizeof(test_area), "%s", area);
}
extern int _spu_rev_offsetaddr;

/* Pinned SDL's internal keyboard entry points keep synthetic input in-process. */
extern bool SDL_InitKeyboard(void);
extern void SDL_QuitKeyboard(void);
extern void SDL_AddKeyboard(SDL_KeyboardID id, const char *name);
extern void SDL_RemoveKeyboard(SDL_KeyboardID id);
extern bool SDL_SendKeyboardKey(Uint64 timestamp, SDL_KeyboardID id, int rawcode,
                                SDL_Scancode scancode, bool down);
extern void Psyz_PadsPoll(void);

int MgsInputMovieReplaying(void) { return replaying; }
int MgsInputMovieSupportsRumble(int port)
{
    return replaying ? port == 0 : Psyz_PadsSupportsRumble(port);
}

static bool SDLCALL RecordRumble(void *user, Uint16 low, Uint16 high)
{
    (void)user;
    rumble_low = low;
    rumble_high = high;
    ++rumble_calls;
    return true;
}

static SDL_JoystickID AttachController(int rumble)
{
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.name = "MGS SDK test controller";
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
    desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
    desc.Rumble = rumble ? RecordRumble : NULL;
    return SDL_AttachVirtualJoystick(&desc);
}

static void DisablePhysicalControllers(void)
{
    const char *drivers[] = {
        SDL_HINT_JOYSTICK_HIDAPI, SDL_HINT_JOYSTICK_RAWINPUT,
        SDL_HINT_JOYSTICK_DIRECTINPUT, SDL_HINT_JOYSTICK_WGI,
        SDL_HINT_JOYSTICK_GAMEINPUT, SDL_HINT_XINPUT_ENABLED
    };
    for (unsigned i = 0; i < sizeof(drivers) / sizeof(drivers[0]); ++i) {
        assert(SDL_SetHintWithPriority(drivers[i], "0", SDL_HINT_OVERRIDE));
    }
}

static void ReadFrame(int port, unsigned char frame[PSYZ_PAD_BUF_LEN])
{
    SDL_UpdateJoysticks();
    Psyz_PadsPoll();
    Psyz_PadsGet(port, (char *)frame, PSYZ_PAD_BUF_LEN);
}

static void PressPortKey(int repeat)
{
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = SDL_SCANCODE_F2;
    event.key.repeat = repeat != 0;
    assert(SDL_PushEvent(&event));
    Psyz_PadsPoll();
}

static void TestPortSwap(void)
{
    unsigned char original[2][PSYZ_PAD_BUF_LEN], frame[PSYZ_PAD_BUF_LEN];
    assert(Psyz_PadsHostPort(-1) == -1 && Psyz_PadsHostPort(2) == -1);
    assert(!*MgsControllerPortsHint());
    SetTestArea("s07b");
    GM_PlayerStatus = PLAYER_SECOND_AVAILABLE;
    MgsControllerPortsUpdate();
    assert(!SDL_GetHintBoolean("REI_CONTROLLER_IDENTITY_PORTS", false));
    assert(!*MgsControllerPortsHint());
    for (int i = 0; i < 2; ++i) {
        GM_GameOverTimer = -2; /* Meryl's suspended scene is not a death. */
        MgsControllerPortsUpdate();
        GM_GameOverTimer = 0;
        MgsControllerPortsUpdate();
    }
    assert(!*MgsControllerPortsHint());
    for (int i = 0; i < 2; ++i) {
        GM_GameOverTimer = 4;
        MgsControllerPortsUpdate();
        GM_GameOverTimer = 0;
        MgsControllerPortsUpdate();
    }
    assert(strstr(MgsControllerPortsHint(), "SWITCH TO PORT 2"));
    for (int port = 0; port < 2; ++port)
        ReadFrame(port, original[port]);
    PressPortKey(0);
    assert(Psyz_PadsHostPort(0) == 1 && Psyz_PadsHostPort(1) == 0);
    for (int port = 0; port < 2; ++port) {
        ReadFrame(port, frame);
        assert(!memcmp(frame, original[1 - port], sizeof(frame)));
    }
    assert(!Psyz_PadsSupportsRumble(0) && Psyz_PadsSupportsRumble(1));
    assert(Psyz_PadsRumble(1, 1234, 5678, 100));
    assert(rumble_low == 1234 && rumble_high == 5678);
    assert(strstr(MgsControllerPortsHint(), "RETURN TO PORT 1"));
    SetTestArea("s08a");
    MgsControllerPortsUpdate();
    /* Other scripts can enable the second controller; routing follows the
     * recovered availability flag even when Mantis's UI hint is hidden. */
    assert(!SDL_GetHintBoolean("REI_CONTROLLER_IDENTITY_PORTS", false));
    assert(strstr(MgsControllerPortsHint(), "RETURN TO PORT 1"));
    PressPortKey(1);
    assert(Psyz_PadsHostPort(0) == 1);
    replaying = 1;
    SetTestArea("s07b");
    MgsControllerPortsUpdate();
    GM_PlayerStatus |= PLAYER_SECOND_CONTROLLER;
    PressPortKey(0);
    assert(Psyz_PadsHostPort(0) == 1);
    assert(!strcmp(MgsControllerPortsHint(), "REPLAY - CONTROLLER PORT 2"));
    replaying = 0;
    PressPortKey(0);
    assert(Psyz_PadsHostPort(0) == 0 && Psyz_PadsHostPort(1) == 1);
    SetTestArea("s08a");
    MgsControllerPortsUpdate();
    assert(!*MgsControllerPortsHint());
    GM_PlayerStatus = 0;
    assert(!*MgsControllerPortsHint());
}

static void TestPortChord(SDL_Joystick *controller)
{
    unsigned char frame[PSYZ_PAD_BUF_LEN];
    assert(SDL_SetJoystickVirtualButton(controller, SDL_GAMEPAD_BUTTON_LEFT_STICK, true));
    ReadFrame(0, frame);
    assert(Psyz_PadsHostPort(0) == 0);
    assert(SDL_SetJoystickVirtualButton(controller, SDL_GAMEPAD_BUTTON_RIGHT_STICK, true));
    ReadFrame(0, frame);
    assert(Psyz_PadsHostPort(0) == 1);
    ReadFrame(0, frame);
    assert(Psyz_PadsHostPort(0) == 1);
    for (int pass = 0; pass < 2; ++pass) {
        for (int button = SDL_GAMEPAD_BUTTON_LEFT_STICK; button <= SDL_GAMEPAD_BUTTON_RIGHT_STICK; ++button)
            assert(SDL_SetJoystickVirtualButton(controller, button, false));
        ReadFrame(0, frame);
        replaying = pass == 0;
        for (int button = SDL_GAMEPAD_BUTTON_LEFT_STICK; button <= SDL_GAMEPAD_BUTTON_RIGHT_STICK; ++button)
            assert(SDL_SetJoystickVirtualButton(controller, button, true));
        ReadFrame(0, frame);
        assert(Psyz_PadsHostPort(0) == (pass == 0));
        replaying = 0;
    }
    for (int button = SDL_GAMEPAD_BUTTON_LEFT_STICK; button <= SDL_GAMEPAD_BUTTON_RIGHT_STICK; ++button)
        assert(SDL_SetJoystickVirtualButton(controller, button, false));
    ReadFrame(0, frame);
}

static void TestEndingDebug(SDL_Joystick *controller)
{
    unsigned char frame[PSYZ_PAD_BUF_LEN];
    int port=Psyz_PadsHostPort(0);
    SetTestArea("ending");
    MgsEndingDebugPlayback(MGS_ENDING_DEBUG_MOVIE,1);
    PressPortKey(0);
    assert(Psyz_PadsHostPort(0)==port);
    assert(MgsEndingDebugTakeSkip(MGS_ENDING_DEBUG_MOVIE));
    assert(!MgsEndingDebugTakeSkip(MGS_ENDING_DEBUG_MOVIE));
    MgsEndingDebugPlayback(MGS_ENDING_DEBUG_MOVIE,0);
    MgsEndingDebugPlayback(MGS_ENDING_DEBUG_MOVIE,1);
    PressPortKey(1); /* Held F2 repeat must not skip the successor. */
    assert(!MgsEndingDebugTakeSkip(MGS_ENDING_DEBUG_MOVIE));
    replaying=1; PressPortKey(0);
    assert(!MgsEndingDebugTakeSkip(MGS_ENDING_DEBUG_MOVIE)); replaying=0;
    MgsEndingDebugPlayback(MGS_ENDING_DEBUG_MOVIE,0);
    SetTestArea("roll"); MgsEndingDebugPlayback(MGS_ENDING_DEBUG_ROLL,1);
    assert(SDL_SetJoystickVirtualButton(controller,SDL_GAMEPAD_BUTTON_LEFT_STICK,true));
    assert(SDL_SetJoystickVirtualButton(controller,SDL_GAMEPAD_BUTTON_RIGHT_STICK,true));
    ReadFrame(0,frame);
    assert(Psyz_PadsHostPort(0)==port);
    assert(MgsEndingDebugTakeSkip(MGS_ENDING_DEBUG_ROLL));
    MgsEndingDebugPlayback(MGS_ENDING_DEBUG_ROLL,0);
    MgsEndingDebugPlayback(MGS_ENDING_DEBUG_ROLL,1);
    ReadFrame(0,frame);
    assert(!MgsEndingDebugTakeSkip(MGS_ENDING_DEBUG_ROLL));
    for(int button=SDL_GAMEPAD_BUTTON_LEFT_STICK;button<=SDL_GAMEPAD_BUTTON_RIGHT_STICK;++button)
        assert(SDL_SetJoystickVirtualButton(controller,button,false));
    ReadFrame(0,frame);
    for(int button=SDL_GAMEPAD_BUTTON_LEFT_STICK;button<=SDL_GAMEPAD_BUTTON_RIGHT_STICK;++button)
        assert(SDL_SetJoystickVirtualButton(controller,button,true));
    ReadFrame(0,frame);
    assert(MgsEndingDebugTakeSkip(MGS_ENDING_DEBUG_ROLL));
    for(int button=SDL_GAMEPAD_BUTTON_LEFT_STICK;button<=SDL_GAMEPAD_BUTTON_RIGHT_STICK;++button)
        assert(SDL_SetJoystickVirtualButton(controller,button,false));
    ReadFrame(0,frame);
    MgsEndingDebugPlayback(MGS_ENDING_DEBUG_ROLL,0);
    SetTestArea("s07b"); MgsEndingDebugPlayback(MGS_ENDING_DEBUG_MOVIE,1);
    PressPortKey(0);
    assert(Psyz_PadsHostPort(0)!=port && !MgsEndingDebugTakeSkip(MGS_ENDING_DEBUG_MOVIE));
    PressPortKey(0); assert(Psyz_PadsHostPort(0)==port);
    MgsEndingDebugPlayback(MGS_ENDING_DEBUG_MOVIE,0);
    SetTestArea("");
}

static void PushKey(unsigned keyboard, SDL_Scancode key, int down)
{
    SDL_Event event = {0}; event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.which = keyboard; event.key.scancode = key;
    assert(SDL_PushEvent(&event)); Psyz_PadsPoll();
}

static void TestMantisIdentity(void)
{
    PsyzInputDeviceInfo base = { .kind = 2, .instance = 3, .family = 1, .wired = 1 };
    PsyzInputDeviceInfo other = base;
    strcpy(base.serial, "A"); strcpy(base.usb_socket, "USB1");
    other = base; other.instance = 9;
    assert(!MgsMantisDifferentDevice(&base, &other, 0)); /* Same serial, same port. */
    strcpy(other.usb_socket, "USB2");
    assert(MgsMantisDifferentDevice(&base, &other, 0));
    strcpy(other.serial, "B"); assert(MgsMantisDifferentDevice(&base, &other, 0));
    /* Same controller, Bluetooth to wired (or back): transport is not identity. */
    other = base; other.instance = 9; other.wired = 0;
    strcpy(other.usb_socket, "RADIO2"); /* Defensive: receiver location is ignored. */
    assert(!MgsMantisDifferentDevice(&base, &other, 0));
    assert(!MgsMantisDifferentDevice(&other, &base, 0));
    /* A Bluetooth baseline never gains an original socket from later input. */
    PsyzInputDeviceInfo wireless = other;
    other.wired = 1; strcpy(other.usb_socket, "USB3");
    assert(!MgsMantisDifferentDevice(&wireless, &other, 0));
    assert(!wireless.wired && !strcmp(wireless.usb_socket, "RADIO2"));
    /* Moving a receiver grants nothing; separate proven controllers do. */
    other = wireless; other.instance = 12;
    strcpy(other.usb_socket, "RADIO4");
    assert(!MgsMantisDifferentDevice(&wireless, &other, 0));
    strcpy(other.serial, "B");
    assert(MgsMantisDifferentDevice(&wireless, &other, 0));
    strcpy(other.usb_socket, wireless.usb_socket);
    assert(MgsMantisDifferentDevice(&wireless, &other, 0)); /* Shared receiver. */
    wireless.serial[0] = other.serial[0] = 0;
    assert(!MgsMantisDifferentDevice(&wireless, &other, 0)); /* Unknown reconnect. */
    assert(MgsMantisDifferentDevice(&wireless, &other, 1)); /* Concurrent instances. */
    other = base; other.instance = 9;
    base.serial[0] = other.serial[0] = 0;
    other.usb_socket[0] = 0;
    assert(!MgsMantisDifferentDevice(&base, &other, 0)); /* Unknown reconnect. */
    assert(MgsMantisDifferentDevice(&base, &other, 1)); /* Two connected devices. */
    other.kind = 1; assert(MgsMantisDifferentDevice(&base, &other, 0));
}

static void TestMantisDevices(SDL_Joystick *controllers[2])
{
    unsigned char frame[PSYZ_PAD_BUF_LEN];
    SetTestArea("s08a"); MgsControllerPortsUpdate();
    Psyz_PadsSetHostPortSwap(0);
    SetTestArea("s07b"); GM_PlayerStatus = PLAYER_SECOND_AVAILABLE;
    MgsControllerPortsUpdate();
    /* First input is pad 2 even though it was attached second. */
    assert(SDL_SetJoystickVirtualButton(controllers[1], SDL_GAMEPAD_BUTTON_WEST, true));
    ReadFrame(0, frame);
    assert(Psyz_PadsHostPort(0) == 0 && (frame[3] & 0x80) == 0);
    assert(SDL_SetJoystickVirtualButton(controllers[0], SDL_GAMEPAD_BUTTON_SOUTH, true));
    ReadFrame(1, frame);
    assert(Psyz_PadsHostPort(0) == 1 && (frame[3] & 0x40) == 0 && (frame[3] & 0x80));
    /* A held button on the previous device cannot leak into selected input. */
    PushKey(9001, SDL_SCANCODE_UP, 1);
    ReadFrame(1, frame);
    assert(Psyz_PadsHostPort(0) == 1 && frame[3] == 0xff);
    PushKey(9001, SDL_SCANCODE_UP, 0);
    assert(SDL_SetJoystickVirtualButton(controllers[1], SDL_GAMEPAD_BUTTON_WEST, false));
    ReadFrame(1, frame);
    assert(SDL_SetJoystickVirtualButton(controllers[1], SDL_GAMEPAD_BUTTON_WEST, true));
    ReadFrame(0, frame); assert(Psyz_PadsHostPort(0) == 0);
    GV_PauseLevel = 2;
    PushKey(9001, SDL_SCANCODE_X, 1); assert(Psyz_PadsHostPort(0) == 0);
    GV_PauseLevel = 0; PushKey(9001, SDL_SCANCODE_X, 0);
    GM_GameOverTimer = 1; MgsControllerPortsUpdate();
    assert(Psyz_PadsHostPort(0) == 0);
    GM_GameOverTimer = 0;
    PushKey(9001, SDL_SCANCODE_X, 1); assert(Psyz_PadsHostPort(0) == 0);
    PushKey(9002, SDL_SCANCODE_X, 1); assert(Psyz_PadsHostPort(0) == 1);
    /* Two keyboards may hold the same key, independently. */
    PushKey(9002, SDL_SCANCODE_X, 0); ReadFrame(1, frame); assert(frame[3] == 0xff);
    PushKey(9001, SDL_SCANCODE_UP, 1); assert(Psyz_PadsHostPort(0) == 0);
    PushKey(9001, SDL_SCANCODE_X, 0); PushKey(9001, SDL_SCANCODE_UP, 0);
    SDL_JoystickID added = AttachController(0);
    SDL_Joystick *third = SDL_OpenJoystick(added); assert(third);
    ReadFrame(0, frame); /* Attachment alone must not defeat reading. */
    assert(Psyz_PadsHostPort(0) == 0);
    assert(SDL_SetJoystickVirtualButton(third, SDL_GAMEPAD_BUTTON_SOUTH, true));
    ReadFrame(1, frame);
    assert(Psyz_PadsHostPort(0) == 1 && !(frame[3] & 0x40));
    SDL_CloseJoystick(third); assert(SDL_DetachVirtualJoystick(added));
    ReadFrame(1, frame); assert(frame[0] == 0xff || frame[1] == 0xff);
    SetTestArea("s08a"); MgsControllerPortsUpdate();
    assert(Psyz_PadsHostPort(0) == 0);
    for (int i = 0; i < 2; ++i) {
        assert(SDL_SetJoystickVirtualButton(controllers[i], SDL_GAMEPAD_BUTTON_WEST, false));
        assert(SDL_SetJoystickVirtualButton(controllers[i], SDL_GAMEPAD_BUTTON_SOUTH, false));
    }
    ReadFrame(0, frame); GM_PlayerStatus = 0; SetTestArea("");
}

static void TestAnalogAxes(SDL_Joystick *controllers[2])
{
    const Sint16 axes[][4] = {
        {-32768, 32767, -16384, 16384},
        {32767, -32768, 16384, -16384},
        {-1, 0, 255, 256},
        {0, 0, 0, 0}
    };
    const unsigned char bytes[][4] = {
        {64, 192, 0, 255}, {192, 64, 255, 0},
        {128, 129, 127, 128}, {128, 128, 128, 128}
    };
    /* The established PsyZ mapping also projects extreme left-stick axes
     * into digital directions; this fixture previously ignored those bits. */
    const unsigned char directions[] = {0x3f, 0xcf, 0xff, 0xff};
    unsigned char frame[PSYZ_PAD_BUF_LEN];
    for (unsigned sample = 0; sample < sizeof(axes) / sizeof(axes[0]); ++sample) {
        for (int port = 0; port < 2; ++port) {
            for (int axis = 0; axis < 4; ++axis)
                assert(SDL_SetJoystickVirtualAxis(controllers[port], axis, axes[sample][axis]));
            assert(SDL_SetJoystickVirtualButton(controllers[port],
                port ? SDL_GAMEPAD_BUTTON_NORTH : SDL_GAMEPAD_BUTTON_SOUTH, true));
            ReadFrame(port, frame);
            assert(frame[0] == 0 && frame[1] == PSYZ_CTRL_ANALOG_PAD);
            assert(frame[2] == directions[sample] && frame[3] == (port ? 0xef : 0xbf));
            assert(memcmp(frame + 4, bytes[sample], sizeof(bytes[sample])) == 0);
        }
    }
    for (int port = 0; port < 2; ++port)
        assert(SDL_SetJoystickVirtualButton(controllers[port],
            port ? SDL_GAMEPAD_BUTTON_NORTH : SDL_GAMEPAD_BUTTON_SOUTH, false));
}

static void TestModeSwitch(SDL_Joystick *controller)
{
    unsigned char frame[PSYZ_PAD_BUF_LEN];
    const unsigned char digital[] = {0, PSYZ_CTRL_DIGITAL_PAD, 0xdf, 0xff, 0, 0, 0, 0};
    assert(SDL_SetJoystickVirtualAxis(controller, SDL_GAMEPAD_AXIS_LEFTX, 32767));
    assert(PadSetMainMode(0, 0, 3));
    ReadFrame(0, frame);
    assert(memcmp(frame, digital, sizeof(digital)) == 0);
    assert(PadInfoMode(0, InfoModeCurID, 0) == 4);
    assert(PadInfoMode(0x10, InfoModeCurID, 0) == 7);
    assert(PadSetMainMode(0, 1, 3));
    ReadFrame(0, frame);
    assert(frame[1] == PSYZ_CTRL_ANALOG_PAD && frame[6] == 255);
    assert(PadInfoMode(0, InfoModeCurID, 0) == 7);
    assert(!PadSetMainMode(0, 2, 0));
}

static void TestKeyboardOnly(void)
{
    const SDL_KeyboardID keyboard = 0x7fffffff;
    const unsigned char expected[] = {0, PSYZ_CTRL_ANALOG_PAD, 0xef, 0xbf, 128, 128, 128, 128};
    unsigned char frame[PSYZ_PAD_BUF_LEN];
    assert(SDL_InitKeyboard());
    SDL_AddKeyboard(keyboard, "MGS SDK test keyboard");
    assert(SDL_HasKeyboard());
    PadStartCom();
    assert(SDL_SendKeyboardKey(0, keyboard, 0, SDL_SCANCODE_UP, true));
    assert(SDL_SendKeyboardKey(0, keyboard, 0, SDL_SCANCODE_X, true));
    ReadFrame(0, frame);
    assert(memcmp(frame, expected, sizeof(expected)) == 0);
    ReadFrame(1, frame);
    for (unsigned i = 0; i < sizeof(frame); ++i)
        assert(frame[i] == 0xff);
    PressPortKey(0);
    ReadFrame(1, frame);
    assert(!memcmp(frame, expected, sizeof(expected)));
    ReadFrame(0, frame);
    for (unsigned i = 0; i < sizeof(frame); ++i)
        assert(frame[i] == 0xff);
    PressPortKey(0);
    assert(SDL_SendKeyboardKey(0, keyboard, 0, SDL_SCANCODE_UP, false));
    assert(SDL_SendKeyboardKey(0, keyboard, 0, SDL_SCANCODE_X, false));
    ReadFrame(0, frame);
    assert(frame[2] == 0xff && frame[3] == 0xff);
    PadStopCom();
    SDL_RemoveKeyboard(keyboard);
    SDL_QuitKeyboard();
}

static void TestRumble(PsyzVSyncCb on_frame)
{
    unsigned char actuators[8] = {1, 128};
    char alignment[6] = {0, 1, -1, -1, -1, -1};
    assert(PadGetState(0) == PadStateStable);
    assert(PadGetState(1) == PadStateDiscon);
    assert(PadInfoAct(0, -1, 0) == 2);
    assert(PadInfoAct(0, 0, InfoActSub) == 2);
    assert(PadInfoAct(0, 1, InfoActSize) == 1);
    PadSetAct(0, actuators, sizeof(actuators));
    assert(PadSetActAlign(0, alignment));
    on_frame();
    assert(rumble_calls > 0 && rumble_low == 32896 && rumble_high == 65535);
    actuators[0] = 0;
    actuators[1] = 255;
    on_frame();
    assert(rumble_low == 65535 && rumble_high == 0);
    int before_replay = rumble_calls;
    replaying = 1;
    on_frame();
    assert(rumble_calls == before_replay);
    replaying = 0;
    PadStopCom();
    assert(!Psyz_PadsRunning());
    assert(rumble_low == 0 && rumble_high == 0);
    int calls_after_stop = rumble_calls;
    on_frame();
    assert(rumble_calls == calls_after_stop);
    assert(PadGetState(0) == PadStateDiscon);
}

static void TestControllers(void)
{
    unsigned char first[PSYZ_PAD_BUF_LEN], second[PSYZ_PAD_BUF_LEN];
    SDL_JoystickID devices[2];
    SDL_Joystick *controllers[2];
    DisablePhysicalControllers();
    assert(SDL_Init(SDL_INIT_GAMEPAD));
    for (int port = 0; port < 2; ++port) {
        devices[port] = AttachController(port == 0);
        assert(devices[port] != 0 && SDL_IsGamepad(devices[port]));
        controllers[port] = SDL_OpenJoystick(devices[port]);
        assert(controllers[port]);
    }
    PadInitDirect(first, second);
    MgsControllerPortsInit();
    assert(!Psyz_PadsRunning());
    for (int port = 0; port < 2; ++port)
        assert(Psyz_PadsSetKind(port, 0, PSYZ_CTRL_QUERY_KIND) == PSYZ_CTRL_ANALOG_PAD);
    PadStartCom();
    assert(Psyz_PadsRunning());
    PsyzVSyncCb on_frame = Psyz_SetVSyncCb(NULL);
    assert(on_frame);
    TestAnalogAxes(controllers);
    TestModeSwitch(controllers[0]);
    TestPortSwap();
    TestPortChord(controllers[0]);
    TestEndingDebug(controllers[0]);
    TestRumble(on_frame);
    TestMantisIdentity();
    TestMantisDevices(controllers);
    SDL_CloseJoystick(controllers[1]);
    assert(SDL_DetachVirtualJoystick(devices[1]));
    Psyz_PadsPoll();
    for (int port = 0; port < 1; ++port) {
        SDL_CloseJoystick(controllers[port]);
        assert(SDL_DetachVirtualJoystick(devices[port]));
    }
    TestKeyboardOnly();
    SDL_Quit();
}

static void TestReverbReservation(void)
{
    long records[16];
    _spu_rev_offsetaddr = 0xf000;
    assert(SpuInitMalloc(8, (char *)records) == 8);
    assert(SpuReserveReverbWorkArea(SPU_ON) == SPU_ON);
    assert(SpuMallocWithStartAddr(0x78000, 64) == -1);
    assert(SpuReserveReverbWorkArea(SPU_OFF) == SPU_OFF);
    long allocation = SpuMallocWithStartAddr(0x78000, 64);
    assert(allocation == 0x78000);
    assert(SpuReserveReverbWorkArea(SPU_ON) == SPU_OFF);
    SpuFree((unsigned long)allocation);
    assert(SpuReserveReverbWorkArea(SPU_ON) == SPU_ON);
    assert(SpuReserveReverbWorkArea(2) == SPU_ERROR);
}

int main(void)
{
    TestControllers();
    TestReverbReservation();
    puts("Analog sticks, keyboard buttons, controller modes, SDL vibration, stop behavior, and SPU reservation passed.");
    return 0;
}

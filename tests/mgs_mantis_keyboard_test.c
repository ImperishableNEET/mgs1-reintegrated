#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef uint32_t SDL_KeyboardID;
typedef unsigned long u_long;
typedef uint64_t Uint64;
typedef uint16_t Uint16;
typedef int SDL_Scancode;
typedef struct { unsigned id; } SDL_Window;
enum { SDL_SCANCODE_UNKNOWN, KEY_A, KEY_B, SDL_SCANCODE_COUNT = 512 };
enum { SDL_EVENT_KEY_DOWN = 1, SDL_EVENT_KEY_UP, SDL_EVENT_KEYBOARD_ADDED,
       SDL_EVENT_KEYBOARD_REMOVED, SDL_EVENT_WINDOW_FOCUS_LOST };
typedef struct {
    unsigned type;
    struct { SDL_KeyboardID which; SDL_Scancode scancode; Uint64 timestamp;
             unsigned windowID; int mod, key; Uint16 raw; bool down, repeat; } key;
    struct { SDL_KeyboardID which; } kdevice;
} SDL_Event;
#define SDL_arraysize(a) (sizeof(a) / sizeof((a)[0]))
#define SDL_zero(a) memset(&(a), 0, sizeof(a))
#define SDL_HINT_WINDOWS_RAW_KEYBOARD "SDL_WINDOWS_RAW_KEYBOARD"
#define SDL_free free
static bool aggregate[SDL_SCANCODE_COUNT];
static SDL_Event events[32];
static unsigned event_count;
static bool events_enabled = true;
static SDL_Window focus = {7};
static bool SDL_SetHint(const char *name, const char *value) {
    assert(!strcmp(name, SDL_HINT_WINDOWS_RAW_KEYBOARD) && !strcmp(value, "1")); return true;
}
static SDL_KeyboardID *SDL_GetKeyboards(int *count) {
    SDL_KeyboardID *ids = malloc(2 * sizeof(*ids));
    assert(ids); ids[0] = 10; ids[1] = 20; *count = 2; return ids;
}
static const bool *SDL_GetKeyboardState(int *count) {
    (void)count; return aggregate;
}
static bool SDL_EventEnabled(unsigned type) { (void)type; return events_enabled; }
static SDL_Window *SDL_GetKeyboardFocus(void) { return &focus; }
static int SDL_GetModState(void) { return 0; }
static int SDL_GetKeyFromScancode(SDL_Scancode code, int mod, bool key_event) {
    (void)mod; (void)key_event; return code;
}
static bool SDL_PushEvent(const SDL_Event *event) {
    assert(event_count < SDL_arraysize(events)); events[event_count++] = *event; return true;
}
/* Reproduce the upstream aggregate repeat/release suppression. The adapter
 * under test must supply only the physical release this sender drops. */
static bool SDL_SendKeyboardKey(Uint64 timestamp, SDL_KeyboardID id, int raw,
                                SDL_Scancode code, bool down) {
    if (!down && !aggregate[code]) return false;
    SDL_Event event = {0};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.which = id; event.key.scancode = code;
    event.key.repeat = down && aggregate[code]; event.key.down = down;
    event.key.timestamp = timestamp; event.key.raw = (Uint16)raw;
    aggregate[code] = down;
    return events_enabled && SDL_PushEvent(&event);
}
#include "mantis_keyboard_fixture.h"

static int Input(SDL_KeyboardID id, bool down) {
    event_count = 0;
    WIN_SendRawKeyboardKey(123, id, 30, KEY_A, down);
    assert(event_count == 1);
    assert(events[0].key.which == id && events[0].key.down == down);
    return PsyzMantisKeyboardEvent(&events[0]);
}

int main(void) {
    PsyzMantisKeyboardInit();
    assert(PsyzMantisKeyboardConnected(10) && PsyzMantisKeyboardConnected(20));
    assert(Input(10, true));
    assert(PsyzMantisKeyboardFresh(10, KEY_A));
    assert(!PsyzMantisKeyboardFresh(20, KEY_A));
    const u_long mapping[] = {KEY_B, KEY_A};
    assert(PsyzMantisKeyboardRead(10, mapping, 2) == 2);
    assert(Input(20, true)); /* SDL marks this repeat, but device20 is fresh. */
    assert(!Input(20, true));
    assert(!PsyzMantisKeyboardFresh(20, KEY_A));
    assert(!Input(10, false));
    assert(!PsyzMantisKeyboardKeys(10)[KEY_A] && PsyzMantisKeyboardKeys(20)[KEY_A]);
    assert(!aggregate[KEY_A]);
    assert(!Input(20, false)); /* Suppressed upstream; adapter must deliver. */
    assert(!PsyzMantisKeyboardKeys(20)[KEY_A] && !aggregate[KEY_A]);
    assert(Input(20, true));
    assert(Input(10, true));
    assert(!Input(20, false));
    assert(!Input(10, false)); /* Reverse release order also clears correctly. */
    assert(!PsyzMantisKeyboardKeys(10)[KEY_A]);

    SDL_Event event = {0};
    event.type = SDL_EVENT_KEYBOARD_ADDED; event.kdevice.which = 30;
    PsyzMantisKeyboardEvent(&event); assert(PsyzMantisKeyboardConnected(30));
    assert(Input(30, true));
    event.type = SDL_EVENT_KEYBOARD_REMOVED;
    PsyzMantisKeyboardEvent(&event); assert(!PsyzMantisKeyboardKeys(30));
    event.type = SDL_EVENT_KEYBOARD_ADDED;
    PsyzMantisKeyboardEvent(&event); assert(!PsyzMantisKeyboardKeys(30)[KEY_A]);
    assert(Input(30, true));
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    PsyzMantisKeyboardEvent(&event); assert(!PsyzMantisKeyboardKeys(30)[KEY_A]);
    assert(!Input(30, true)); /* Held auto-repeat after refocus is not fresh. */
    assert(!PsyzMantisKeyboardKeys(30)[KEY_A]);
    assert(!Input(30, false));
    assert(Input(30, true));
    assert(!Input(30, false));
    event_count = 0; aggregate[KEY_A] = false;
    WIN_SendRawKeyboardKey(1, 0, 30, KEY_A, false);
    assert(!event_count); /* Unknown/global IDs keep old suppression. */
    events_enabled = false;
    WIN_SendRawKeyboardKey(1, 30, 30, KEY_A, false);
    assert(!event_count);
    puts("Physical keyboards: overlapping keys, repeat, hotplug and focus checks passed");
    return 0;
}

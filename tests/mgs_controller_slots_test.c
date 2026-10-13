#include <assert.h>
#include <stdio.h>
#include "../vendor/psyz/psyz/src/platform/gamepad_slots.h"

int main(void) {
    PsyzGamepadSlot slots[2] = {0};
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-A", 0, 1) == 0);
    PsyzGamepadRememberSlot(&slots[0], "pad-A", 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-A", 0, 1) == -2);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, 1) == 1);
    PsyzGamepadRememberSlot(&slots[1], "pad-B", 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-C", 0, 1) == -1);
    slots[0].connected = 0;
    assert(PsyzGamepadHostSlot(slots, 2, 0, 1) == 0);
    assert(PsyzGamepadHostSlot(slots, 2, 1, 1) == 1);
    assert(PsyzGamepadHostSlot(slots, 2, 0, 0) == 1);
    assert(PsyzGamepadHostSlot(slots, 2, 1, 0) == 0);
    /* Manual swap happens before this mapping: swapped game port1 is host1. */
    assert(PsyzGamepadHostSlot(slots, 2, 0 ^ 1, 0) == 0);
    assert(PsyzGamepadHostSlot(slots, 2, 1 ^ 1, 0) == 1);
    assert(slots[1].connected); /* Removal never moves the second pad. */
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-A", 0, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 0, 1) == 0);
    slots[1].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-A", 0, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, 0) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-C", 0, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-C", 0, 0) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "", 0, 1) == 0);
    char long_serial[140]; memset(long_serial, 'x', sizeof(long_serial));
    long_serial[139] = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, long_serial, 0, 1) == 0);
    memset(slots, 0, sizeof(slots));
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, 1) == 0);
    PsyzGamepadRememberSlot(&slots[0], NULL, 0);
    slots[0].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, 1) == 0);
    /* Serial-less Xbox replacing DualSense qualifies. USB port/cable end is
     * absent from the identity, so replugging the same family stays port 1. */
    memset(slots, 0, sizeof(slots));
    PsyzGamepadRememberSlot(&slots[0], "sony-serial", 1);
    slots[0].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 2, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 1, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 0, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 2, 0) == 0);
    PsyzGamepadRememberSlot(&slots[1], NULL, 2);
    slots[1].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 2, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 1, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "sony-serial", 2, 1) == 0);
    PsyzGamepadRememberSlot(&slots[0], NULL, 2);
    slots[0].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 2, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 1, 1) == 1);
    puts("Controller slots: stable second port, same/different serial, unknown reconnect and capacity passed.");
    return 0;
}

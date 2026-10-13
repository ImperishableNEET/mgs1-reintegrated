#include <assert.h>
#include <stdio.h>
#include "../vendor/psyz/psyz/src/platform/gamepad_slots.h"

int main(void) {
    PsyzGamepadSlot slots[2] = {0};
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-A", 0, NULL, 1) == 0);
    PsyzGamepadRememberSlot(&slots[0], "pad-A", 0, NULL);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-A", 0, NULL, 1) == -2);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, NULL, 1) == 1);
    PsyzGamepadRememberSlot(&slots[1], "pad-B", 0, NULL);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-C", 0, NULL, 1) == -1);
    slots[0].connected = 0;
    assert(PsyzGamepadHostSlot(slots, 2, 0, 1) == 0);
    assert(PsyzGamepadHostSlot(slots, 2, 1, 1) == 1);
    assert(PsyzGamepadHostSlot(slots, 2, 0, 0) == 1);
    assert(PsyzGamepadHostSlot(slots, 2, 1, 0) == 0);
    /* Manual swap happens before this mapping: swapped game port1 is host1. */
    assert(PsyzGamepadHostSlot(slots, 2, 0 ^ 1, 0) == 0);
    assert(PsyzGamepadHostSlot(slots, 2, 1 ^ 1, 0) == 1);
    assert(slots[1].connected); /* Removal never moves the second pad. */
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-A", 0, NULL, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 0, NULL, 1) == 0);
    slots[1].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-A", 0, NULL, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, NULL, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, NULL, 0) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-C", 0, NULL, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-C", 0, NULL, 0) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "", 0, NULL, 1) == 0);
    char long_serial[140]; memset(long_serial, 'x', sizeof(long_serial));
    long_serial[139] = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, long_serial, 0, NULL, 1) == 0);
    memset(slots, 0, sizeof(slots));
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, NULL, 1) == 0);
    PsyzGamepadRememberSlot(&slots[0], NULL, 0, NULL);
    slots[0].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, "pad-B", 0, NULL, 1) == 0);
    /* Serial-less Xbox replacing DualSense qualifies. USB port/cable end is
     * absent from the identity, so replugging the same family stays port 1. */
    memset(slots, 0, sizeof(slots));
    PsyzGamepadRememberSlot(&slots[0], "sony-serial", 1, NULL);
    slots[0].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 2, NULL, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 1, NULL, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 0, NULL, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 2, NULL, 0) == 0);
    PsyzGamepadRememberSlot(&slots[1], NULL, 2, NULL);
    slots[1].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 2, NULL, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 1, NULL, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "sony-serial", 2, NULL, 1) == 0);
    PsyzGamepadRememberSlot(&slots[0], NULL, 2, NULL);
    slots[0].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 2, NULL, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 1, NULL, 1) == 1);
    const char *socket_a = "PCIROOT(0)#PCI(1400)#USBROOT(0)#USB(3)";
    const char *socket_b = "PCIROOT(0)#PCI(1400)#USBROOT(0)#USB(4)";
    memset(slots, 0, sizeof(slots));
    PsyzGamepadRememberSlot(&slots[0], "same-pad", 1, socket_a);
    assert(PsyzGamepadChooseSlot(slots, 2, "same-pad", 1, socket_b, 1) == -2);
    slots[0].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, "same-pad", 1, socket_a, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "same-pad", 1, socket_b, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, "same-pad", 1, socket_b, 0) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, NULL, 1, socket_b, 1) == 0);
    assert(PsyzGamepadChooseSlot(slots, 2, "same-pad", 1, NULL, 1) == 0);
    PsyzGamepadRememberSlot(&slots[1], "same-pad", 1, socket_b);
    slots[1].connected = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, "same-pad", 1, socket_b, 1) == 1);
    assert(PsyzGamepadChooseSlot(slots, 2, "same-pad", 1, socket_a, 1) == 0);
    slots[0].usb_socket[0] = 0;
    assert(PsyzGamepadChooseSlot(slots, 2, "same-pad", 1, socket_b, 1) == 0);
    puts("Controller slots: stable second port, same/different serial, unknown reconnect and capacity passed.");
    return 0;
}

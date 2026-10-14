#ifndef MGS_MANTIS_DEVICE_H
#define MGS_MANTIS_DEVICE_H
#include <psyz/input.h>
#include <string.h>
/* Unknown reconnect identity grants no resistance. Simultaneously connected
 * instances are distinct; a transport/model string alone is not a serial.
 * Backend serial/path evidence must identify the controller, not a receiver.
 * wired means a verified direct wired endpoint, excluding wireless receivers. */
static int MgsMantisDifferentDevice(const PsyzInputDeviceInfo *base,
    const PsyzInputDeviceInfo *current, int baseline_connected)
{
    if (base->kind != current->kind) return 1;
    if (base->wired && current->wired &&
        base->usb_socket[0] && current->usb_socket[0] &&
        strcmp(base->usb_socket, current->usb_socket)) return 1;
    int same = base->instance == current->instance;
    if (base->serial[0] && current->serial[0]) {
        if (strcmp(base->serial, current->serial)) return 1;
        same = 1;
    } else if (base->path[0] && current->path[0] && !strcmp(base->path, current->path)) {
        same = 1;
    }
    /* A proven controller keeps its identity across Bluetooth/wired changes.
     * Neither an adapter location nor a later socket rebases the encounter. */
    if (same) return 0;
    if (baseline_connected) return 1;
    if (base->family && current->family && base->family != current->family) return 1;
    return 0;
}
#endif

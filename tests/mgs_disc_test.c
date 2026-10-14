#include "mgs_disc.h"
#include "mgs_safety.h"

#include <assert.h>
#include <libcd.h>
#include <stdio.h>
#include <string.h>

/* Linked modern backend references title presentation, unused by CD checks. */
int ReiReintegratedTitleVisible;

static void CheckSerial(const char *serial)
{
    CdlLOC position;
    unsigned char data[2048];
    unsigned char mode = 0;
    CdIntToPos(19, &position);
    assert(CdControl(CdlSetmode, &mode, NULL));
    assert(CdControl(CdlReadN, (unsigned char *)&position, NULL));
    assert(CdReady(0, NULL) == CdlDataReady);
    assert(CdGetSector(data, sizeof(data) / 4));
    data[sizeof(data) - 1] = 0;
    assert(strstr((const char *)data, serial));
    assert(CdControl(CdlStop, NULL, NULL));
}

static void CheckClosedDrive(void)
{
    unsigned char result[8];
    assert(CdControlB(CdlNop, NULL, result));
    assert(!(result[0] & CdlStatShellOpen));
    assert(result[0] & CdlStatStandby);
}

static void RequestDisc(int number, const char *serial)
{
    unsigned char result[8];
    /* The save-load actor must issue its own open request even when there was
     * no preceding campaign disc-change actor. Repeated polling is harmless. */
    MgsDiscChangeOpenFor(number);
    MgsDiscChangeOpenFor(number);
    /* An open shell reports the shell bit even when the command fails. */
    CdControlB(CdlNop, NULL, result);
    assert(result[0] & CdlStatShellOpen);
    MgsDiscChangeClose();
    CheckClosedDrive();
    assert(MgsSafetyDiscNumber() == number);
    CheckSerial(serial);
    MgsDiscChangeClose();
    CheckClosedDrive();
    assert(MgsSafetyDiscNumber() == number);
}

int main(int argc, char **argv)
{
    unsigned char result[8];
    if (argc != 3)
        return 64;
    assert(MgsDiscConfigure(argv[1], argv[2]) == 0);
    CheckSerial("SLPM_862.47;1");
    MgsDiscChangeClose();
    CheckSerial("SLPM_862.47;1");
    MgsDiscChangeOpen();
    CdControlB(CdlNop, NULL, result);
    assert(result[0] & CdlStatShellOpen);
    MgsDiscChangeClose();
    assert(CdControlB(CdlNop, NULL, result));
    assert(!(result[0] & CdlStatShellOpen));
    assert(result[0] & CdlStatStandby);
    assert(SafetyCheck(0, 2, 0x16) == 0);
    assert(CdControlB(CdlGetTN, NULL, result));
    assert(result[1] == 1 && result[2] == 1);
    CheckSerial("SLPM_862.48;1");
    /* A launcher can boot Disc 2 directly while retaining both configured
     * images. Plain Close must never infer a swap or accidentally insert 1. */
    assert(MgsDiscConfigurePaths(argv[2], argv[1], argv[2]) == 0);
    MgsDiscChangeClose();
    assert(MgsSafetyDiscNumber() == 2);
    CheckSerial("SLPM_862.48;1");
    RequestDisc(2, "SLPM_862.48;1"); /* Already the requested disc. */
    RequestDisc(1, "SLPM_862.47;1"); /* Disc 1 save while booted on Disc 2. */
    RequestDisc(2, "SLPM_862.48;1"); /* Disc 2 save while booted on Disc 1. */
    assert(MgsDiscConfigure(argv[2], NULL) == 0);
    MgsDiscChangeClose();
    assert(MgsSafetyDiscNumber() == 2);
    CheckSerial("SLPM_862.48;1");
    puts("Campaign and save-load disc requests, repeated polling and plain-close safety passed.");
    return 0;
}

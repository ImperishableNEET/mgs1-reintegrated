#include "mgs_disc.h"
#include "mgs_safety.h"

#include <psyz/cd.h>
#include <stdio.h>
#include <stdlib.h>

static const char *first_disc, *second_disc;
static int changing_disc;

int MgsDiscConfigurePaths(const char *initial_disc, const char *disc_one, const char *disc_two)
{
    first_disc = disc_one;
    second_disc = disc_two;
    changing_disc = 0;
    if (MgsSafetyConfigure(initial_disc) != 0)
        return -1;
    return Psyz_CdSetDiskPath(initial_disc);
}

int MgsDiscConfigure(const char *initial_disc, const char *disc_two)
{
    return MgsDiscConfigurePaths(initial_disc, initial_disc, disc_two);
}

void MgsDiscChangeOpenFor(int disc)
{
    const char *path = disc == 1 ? first_disc : disc == 2 ? second_disc : NULL;
    if (!path)
    {
        fprintf(stderr, "[native] The original game requested Disc %d. Supply %s <Integral Disc %d.cue>.\n",
                disc, disc == 1 ? "--disc" : "--disc-two", disc);
        exit(EXIT_FAILURE);
    }
    Psyz_CdShellOpen(1);
    changing_disc = disc;
    printf("[native] Original disc-change actor opened the virtual drive for Disc %d.\n", disc);
}

void MgsDiscChangeOpen(void)
{
    MgsDiscChangeOpenFor(2);
}

void MgsDiscChangeClose(void)
{
    if (!changing_disc)
        return;
    int disc = changing_disc;
    const char *path = disc == 1 ? first_disc : second_disc;
    if (MgsSafetyConfigure(path) != 0 || MgsSafetyDiscNumber() != disc || Psyz_CdSetDiskPath(path) != 0)
    {
        fprintf(stderr, "[native] Could not insert Disc %d: %s\n", disc, path);
        exit(EXIT_FAILURE);
    }
    changing_disc = 0;
    Psyz_CdShellOpen(0);
    printf("[native] Disc %d inserted. Press Start at the original disc-change screen.\n", disc);
}

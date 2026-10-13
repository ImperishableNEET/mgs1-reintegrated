/* Like Mantis: an unmodified dispatch table may become read-only during
 * optimization, but its named section also contains script-writable values. */
int OverlayActorAInitial(void);
int OverlayActorBInitial(void);
static int (*actions[2])(void) = {OverlayActorAInitial, OverlayActorBInitial};
static unsigned short levels[6] = {904, 400, 320, 240, 120, 0};
static int initialized = 83;

int OverlayActorBDispatch(unsigned index)
{
    return actions[index % 2]();
}

int OverlayActorBInitial(void)
{
    return initialized == 83 && levels[0] == 904 && levels[1] == 400 &&
           levels[2] == 320 && levels[3] == 240 && levels[4] == 120;
}

void OverlayActorBSetLevels(unsigned short value)
{
    for (int i = 0; i < 5; ++i) levels[i] = value + i;
}

void OverlayActorBMutate(void)
{
    initialized = 19;
    OverlayActorBSetLevels(200);
}

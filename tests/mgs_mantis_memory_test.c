#include "mgs_mantis_memory.h"

#include <SDL3/SDL_filesystem.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#define test_pid _getpid
#else
#include <unistd.h>
#define test_pid getpid
#endif

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--installed")) {
        printf("Installed mask: %u\n", MgsMantisMemoryScanInstalled());
        return 0;
    }
    char root[128], publisher[160], suikoden[160], castlevania[192];
    const char *roots[2];
    unsigned int mask;

    assert(MgsMantisMemoryIdentifyName("Suikoden.exe") == MGS_MANTIS_SUIKODEN);
    assert(MgsMantisMemoryIdentifyName("Other Life: Azure Dreams") == MGS_MANTIS_AZURE_DREAMS);
    assert(MgsMantisMemoryIdentifyName("Castlevania: Symphony of the Night") == MGS_MANTIS_CASTLEVANIA);
    assert(MgsMantisMemoryIdentifyName("Castlevania") == MGS_MANTIS_CASTLEVANIA);
    assert(MgsMantisMemoryIdentifyName("Other Life") == MGS_MANTIS_AZURE_DREAMS);
    assert(MgsMantisMemoryIdentifyName("Suikoden I&II HD Remaster Gate Rune and Dunan Unification Wars") == MGS_MANTIS_SUIKODEN);
    assert(MgsMantisMemoryIdentifyName("Vandal Hearts") == MGS_MANTIS_VANDAL_HEARTS);
    assert(MgsMantisMemoryIdentifyName("Castlevania: Aria of Sorrow") == 0);
    assert(MgsMantisMemoryIdentifyName("Suikoden III") == 0);

    snprintf(root, sizeof(root), ".local/mantis-scan-%d", (int)test_pid());
    snprintf(publisher, sizeof(publisher), "%s/Publisher", root);
    snprintf(suikoden, sizeof(suikoden), "%s/Suikoden", root);
    snprintf(castlevania, sizeof(castlevania), "%s/Castlevania Symphony of the Night", publisher);
    assert(SDL_CreateDirectory(root));
    assert(SDL_CreateDirectory(publisher));
    assert(SDL_CreateDirectory(suikoden));
    assert(SDL_CreateDirectory(castlevania));
    roots[0] = root;
    roots[1] = root; /* Duplicate roots do not duplicate title bits. */
    mask = MgsMantisMemoryScanRoots(roots, 2);
    assert(mask == (MGS_MANTIS_SUIKODEN | MGS_MANTIS_CASTLEVANIA));

    MgsMantisMemoryConfigureInstalled(mask);
    assert(MgsMantisMemoryInstalledEnabled());
    assert(MgsMantisMemoryInstalledMask() == mask);
    assert(MgsMantisMemorySeed() == 0);
    MgsMantisMemoryConfigureInstalled(0);
    assert(MgsMantisMemoryInstalledEnabled()); /* No recognized titles is still installed mode. */
    MgsMantisMemoryConfigure(123);
    assert(!MgsMantisMemoryInstalledEnabled());
    assert(MgsMantisMemorySeed() == 123);

    assert(SDL_RemovePath(castlevania));
    assert(SDL_RemovePath(suikoden));
    assert(SDL_RemovePath(publisher));
    assert(SDL_RemovePath(root));
    puts("mantis memory scanner passed");
    return 0;
}

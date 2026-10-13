#ifndef MGS_MANTIS_MEMORY_H
#define MGS_MANTIS_MEMORY_H

#include <stddef.h>

enum {
    MGS_MANTIS_SUIKODEN = 0x01,
    MGS_MANTIS_AZURE_DREAMS = 0x02,
    MGS_MANTIS_CASTLEVANIA = 0x10,
    MGS_MANTIS_VANDAL_HEARTS = 0x20,
    MGS_MANTIS_INSTALLED_MASK = 0x33
};

void MgsMantisMemoryConfigure(unsigned int seed);
unsigned int MgsMantisMemorySeed(void);
unsigned int MgsMantisMemoryIdentifyName(const char *name);
unsigned int MgsMantisMemoryScanRoots(const char *const *roots, size_t count);
unsigned int MgsMantisMemoryScanInstalled(void);
void MgsMantisMemoryConfigureInstalled(unsigned int mask);
int MgsMantisMemoryInstalledEnabled(void);
unsigned int MgsMantisMemoryInstalledMask(void);

#endif

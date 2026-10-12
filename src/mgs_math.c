#include "mgs_math.h"
#include "gtemac.h"
#include <assert.h>
#include <stdint.h>

extern short SQRT[];

int32_t MgsSquareRoot0(int32_t value)
{
    _Static_assert(sizeof(int32_t) == 4 && sizeof(uint32_t) == 4, "Integral SDK arithmetic requires 32-bit words");
    Psyz_GteDataWrite(30, (uint32_t)value);
    unsigned int leading = Psyz_GteDataRead(31);
    if (leading == 32)
        return 0;

    unsigned int even = leading & ~1u;
    int32_t normalized = even >= 24
        ? (int32_t)((uint32_t)value << (even - 24))
        : (int32_t)value >> (24 - even);
    int index = normalized - 64;
    int sample;
    if (index < 0)
    {
        // Integral's SDK reads the preceding sine table when the input overflowed.
        assert(index >= -320 && index <= -129);
        sample = rsin(index + 1040);
    }
    else
    {
        assert(index < 192);
        sample = SQRT[index];
    }
    return (int32_t)(((uint32_t)sample << ((31 - even) / 2)) >> 12);
}

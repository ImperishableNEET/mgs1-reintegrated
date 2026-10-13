#include <assert.h>
#include <stdlib.h>

#define MGS_NATIVE 1
#define STATE_PADDEMO 0x40000000
int GM_GameStatus;
#include "linkvar.h"

short linkvarbuf[MAX_LINKVARBUF];

int main(void)
{
    const char *value = getenv("REI_FIRST_PERSON_MOVE_SHOOT");
    int enabled = value && value[0] == '1' && value[1] == '\0';

    GM_FirstPerson = 0;
    assert(GM_FirstPersonActive == enabled);
    assert(GM_FirstPerson == 0);
    GM_GameStatus = STATE_PADDEMO;
    assert(GM_FirstPersonActive == 0);
    assert(GM_FirstPerson == 0);
    GM_FirstPerson = 2;
    assert(GM_FirstPersonActive == 2);
    assert(GM_FirstPerson == 2);
    return 0;
}

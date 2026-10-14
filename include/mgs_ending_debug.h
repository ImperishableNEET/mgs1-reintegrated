#ifndef MGS_ENDING_DEBUG_H
#define MGS_ENDING_DEBUG_H
enum { MGS_ENDING_DEBUG_MOVIE = 1, MGS_ENDING_DEBUG_ROLL = 2 };
void MgsEndingDebugPlayback(int kind, int active);
int MgsEndingDebugTakeSkip(int kind);
#endif

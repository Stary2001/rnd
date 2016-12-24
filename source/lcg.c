#include <3ds.h>

#include "lcg.h"

static u32
lcg_do(u32 seed)
{
	return (1103515245 * seed + 12345) & ((1U << 31) - 1);
}

/**
 * Returns the next unsigned 32-bit number from the LCG for the given seed.
 *
 * You do not actually want to use this, it's a bad RNG.
 *
 * In:
 * 0: Header code (0x00010040)
 * 1: seed
 *
 * Out:
 * 0: header code (0x00010080)
 * 1: Result (always 0)
 * 2: next number/seed
 */
void lcg_next(void)
{
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x0001, 2, 0);

	cmdbuf[2] = lcg_do(cmdbuf[1]);
	cmdbuf[1] = 0;
}

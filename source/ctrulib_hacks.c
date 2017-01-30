#include <3ds.h>
#include <3ds/env.h>

#include "util.h"
void __appInit();
void __appInit(void)
{
	Result res;

	/* do not initialize:
	 *
	 * - fs:USER and the SDMC archive
	 * - hid:USER or hid:SPVR
	 * - apt
	 */

	/* Apparently srv: is unavailable to system modules for a while (cf.
	 * yifan lu's 3ds_injector, which seems to be a 1:1 implementation of
	 * what the pm module does).
	 */

	while (true)
	{
		res = srvInit();

		if (R_LEVEL(res) != RL_PERMANENT
			&& R_SUMMARY(res) != RS_NOTFOUND
			&& R_DESCRIPTION(res) != RD_NOT_FOUND)
			break;

		svcSleepThread(500000);
	}

	if (R_FAILED(res))
		panic(res);
}

void __appExit();
void __appExit(void)
{
	srvExit();
}

extern char* fake_heap_start;
extern char* fake_heap_end;
extern int __stacksize__;

u32 __ctru_heap;
u32 __ctru_heap_size = 0x80000; // some arbitrary amount of memory that should be free in BASE.
u32 __ctru_linear_heap;
u32 __ctru_linear_heap_size;

void __system_allocateHeaps();
void __system_allocateHeaps()
{
	__ctru_heap_size += __stacksize__;
	__ctru_heap = 0x08000000;

	u32 tmp;

	TRY(svcControlMemory(&tmp, __ctru_heap, 0x0, __ctru_heap_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE));

	__ctru_linear_heap = 0;
	__ctru_linear_heap_size = 0;

	fake_heap_start = (char*)__ctru_heap;
	fake_heap_end = fake_heap_start + __ctru_heap_size;
}
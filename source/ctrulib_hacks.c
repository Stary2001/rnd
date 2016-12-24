#include <3ds.h>
#include <3ds/env.h>

#include "util.h"

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

void initSystem()
{
	Result __sync_init(void);
	void __system_initSyscalls(void);

	/* do not:
	 *
	 * - mess with the stack using the "fake heap"
	 */

	TRY(__sync_init());
	__system_initSyscalls();
	__appInit();
}

void __appExit(void)
{
	srvExit();
}

void __ctru_exit(int unused)
{
	(void)unused;
	Result __sync_fini(void) __attribute__((weak));
	void envDestroyHandles(void);

	/* Do not:
	 *
	 * - unmap the linear and application heap
	 * - mess with the stack
	 * - try to return to HBL
	 */

	__appExit();

	envDestroyHandles();

	if (__sync_fini)
		TRY(__sync_fini());

	svcExitProcess();
}

#include <3ds.h>

#include "util.h"

void __attribute__((noreturn))
panic(Result res)
{
	asm volatile("mov r1, %0\t\n"
				 "mov r0, #1\t\n"
				 "svc 0x3c" ::"r"(res));

	while (true)
		;
}

void write_invalid_arg_error(void)
{
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0x00000040;
	cmdbuf[1] = 0xd9001830;
}

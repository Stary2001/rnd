#include <3ds.h>

#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static Handle rndLHandle, rndCHandle;

static Result
rndLInit(void)
{
	return srvGetServiceHandle(&rndLHandle, "rnd:L");
}

static Result
rndCInit(void)
{
	return srvGetServiceHandle(&rndCHandle, "rnd:C");
}

static void
rndLExit(void)
{
	svcCloseHandle(rndLHandle);
}

static void
rndCExit(void)
{
	svcCloseHandle(rndCHandle);
}

static Result
RNDL_Next(u32 *seed_and_output)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x1,1,0);
	cmdbuf[1] = *seed_and_output;

	if (R_FAILED(ret = svcSendSyncRequest(rndLHandle)))
		return ret;

	*seed_and_output = cmdbuf[2];

	return (Result)cmdbuf[1];
}

static Result
RNDC_Next(u32 *out)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x1,0,0);

	if (R_FAILED(ret = svcSendSyncRequest(rndCHandle)))
		return ret;

	*out = cmdbuf[2];

	return (Result)cmdbuf[1];
}

static Result
RNDC_Uniform(u32 *out, u32 upper_bound)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x2,1,0);
	cmdbuf[1] = upper_bound;

	if (R_FAILED(ret = svcSendSyncRequest(rndCHandle)))
		return ret;

	*out = cmdbuf[2];

	return (Result)cmdbuf[1];
}

static Result
RNDC_Buffer(void *out, size_t len)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x3,1,2);
	cmdbuf[1] = len;
	cmdbuf[2] = IPC_Desc_Buffer(len, IPC_BUFFER_W);
	cmdbuf[3] = (u32)out;

	if (R_FAILED(ret = svcSendSyncRequest(rndCHandle)))
		return ret;

	return (Result)cmdbuf[1];
}

static void
step(void)
{
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		if (hidKeysDown() & KEY_A)
			break;

		gfxFlushBuffers();
		gfxSwapBuffers();
	}
}

int
main(void)
{
	Result res;
	u8 buf[0x10] = {0};
	u32 i = 0x12345678u;

	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("rnd testing\n");
	printf("-----------\n");

	printf("rnd:L init:\n");
	step();
	rndLInit();
	printf("rnd:C init:\n");
	step();
	rndCInit();

	printf("RNDL_Next check: ");
	step();
	if (R_FAILED(res = RNDL_Next(&i)))
		printf("error (%lx)\n", res);
	if (i == 0x0b719151)
		printf("passed\n");
	else
		printf("failed (%" PRIx32 ")\n", i);

	printf("RNDC_Next: ");
	step();
	if (R_FAILED(res = RNDC_Next(&i)))
		printf("error (%lx)\n", res);
	printf("%" PRIu32 "\n", i);

	printf("RNDC_Uniform(255): ");
	step();
	if (R_FAILED(res = RNDC_Uniform(&i, 255)))
		printf("error (%lx)\n", res);
	else
		printf("%" PRIu32 "\n", i);

	printf("RNDC_Buffer(16):\n  ");
	step();
	if (R_FAILED(res = RNDC_Buffer(buf, sizeof(buf))))
		printf("error (%lx)\n", res);
	for (i = 0; i < sizeof(buf); ++i)
		printf("%02" PRIx8, buf[i]);

	printf("\nPress START to exit.\n");

	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		if (hidKeysDown() & KEY_START)
			break;

		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	gfxExit();
	rndCExit();
	rndLExit();

	return 0;
}

#include <3ds.h>

#include "arc4random.h"
#include "csg.h"
#include "util.h"

/* DO NOT rely on cmdbuf being sane after a call to arc4random functions.
 * It may call ps:ps over IPC, breaking the header.
 */

/**
 * Returns a cryptographically secure unsigned 32-bit integer.
 *
 * Use the _buf functions for buffers or _uniform for a value cap.
 *
 * In:
 * 0: Header code (0x00010000)
 *
 * Out:
 * 0: Header code (0x00010080)
 * 1: Result (always 0)
 * 2: the random number
 */
void
csg_next(void)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[2] = arc4random();

	cmdbuf[0] = IPC_MakeHeader(0x0001, 2, 0);
	cmdbuf[1] = 0;
}

/**
 * Returns a cryptographically secure unsigned 32-bit integer in the range of
 * 0 <= value < upper bound.
 *
 * Use the _buf functions for buffers or _uniform for a value cap.
 *
 * In:
 * 0: Header code (0x00020040)
 * 1: upper bound
 *
 * Out:
 * 0: Header code (0x00020080)
 * 1: Result (always 0)
 * 2: the random number
 */
void
csg_uniform(void)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 upper_bound;

	if (cmdbuf[0] != 0x00020040) {
		write_invalid_arg_error();
		return;
	}

	upper_bound = cmdbuf[1];

	cmdbuf[2] = arc4random_uniform(upper_bound);

	cmdbuf[0] = IPC_MakeHeader(0x0002, 2, 0);
	cmdbuf[1] = 0;
}

/**
 * Fills a buffer with cryptographically secure random data.
 *
 * In:
 * 0: Header code (0x00030042)
 * 1: length in bytes (size)
 * 2: (size << 4) | IPC_BUFFER_W
 * 3: pointer to the buffer
 *
 * Out:
 * 0: Header code (0x00030042)
 * 1: Result (always 0)
 * 2/3: mirror of the input pointer desc
 */
void
csg_buf(void)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	size_t len;
	void *out;

	if (cmdbuf[0] != 0x00030042
			|| !(cmdbuf[2] & IPC_BUFFER_W)) {
		write_invalid_arg_error();
		return;
	}

	len = cmdbuf[2] >> 4;
	out = (void *)cmdbuf[3];

	arc4random_buf(out, len);

	cmdbuf[0] = IPC_MakeHeader(0x0003, 1, 2);
	cmdbuf[2] = IPC_Desc_Buffer(len, IPC_BUFFER_W);
	cmdbuf[3] = (u32)out;

	cmdbuf[1] = 0;
}


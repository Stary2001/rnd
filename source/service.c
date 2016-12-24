#include <3ds.h>

#include <string.h>

#include "csg.h"
#include "lcg.h"
#include "util.h"
#include "service.h"

#define LCG_PORT_IDX			(0)
#define CSG_PORT_IDX			(1)
#define NOTIFIER_IDX			(2)
#define BASE_HANDLES			(3)

#define MAX_SESSIONS_LCG		(10)
#define MAX_SESSIONS_CSG		(5)
#define MAX_SESSIONS_TOTAL		(MAX_SESSIONS_LCG + MAX_SESSIONS_CSG)

#define ERR_SESSION_CLOSED_BY_REMOTE	(0xc920181aU)

/* This should be an unused module value for a result code.
 *
 * There are two reserve result module codes before 90 for O3DS modules,
 * everything after that is reserved for N3DS until 254, which is reserved
 * for the application and 255, which is invalid.
 *
 * YOU WANT TO CHANGE THIS ON A FORK. DO NOT LET TWO CUSTOM SYSTEM MODULES
 * SHARE THE SAME RESULT MODULE CODE.
 */
#define RM_RND				88

struct Service {
	const char *name;
	const int max_sessions;
	void (*const *handlers)(void);
	const size_t ncommands;
	Handle handle;
};

static Handle notifier;

/* If you want to stub a handler out, set it to write_invalid_arg_error. */
static void (*const lcg_handlers[])(void) = {
	lcg_next
};

static struct Service lcg_service = {
	.name = "rnd:L",
	.max_sessions = MAX_SESSIONS_LCG,
	.ncommands = sizeof(lcg_handlers)/sizeof(*lcg_handlers),
	.handlers = lcg_handlers
};

static void (*const csg_handlers[])(void) = {
	csg_next,
	csg_uniform,
	csg_buf
};

static struct Service csg_service = {
	.name = "rnd:C",
	.max_sessions = MAX_SESSIONS_CSG,
	.ncommands = sizeof(csg_handlers)/sizeof(*csg_handlers),
	.handlers = csg_handlers
};

static s32 nclients;

void
register_service(void)
{
	TRY(srvRegisterService(&lcg_service.handle, lcg_service.name, lcg_service.max_sessions));
	TRY(srvRegisterService(&csg_service.handle, csg_service.name, csg_service.max_sessions));

	TRY(srvEnableNotification(&notifier));
}

static s32
get_closing_client(Handle service_handles[], s32 returned_idx, Handle requester)
{
	size_t i;

	/* -1 appears to be sent on some kind of error delivering the response
	 * to the requester. */

	if (returned_idx != -1)
		return returned_idx;

	for (i = 0; i < (size_t)BASE_HANDLES + nclients; ++i) {
		if (service_handles[i] == requester) {
			if (i < BASE_HANDLES)
				svcBreak(USERBREAK_ASSERT);
			else
				return i;
		}
	}

	svcBreak(USERBREAK_ASSERT);

	return -2;
}

static const struct Service *
find_service_from_index(s32 idx)
{
	switch (idx) {
	case LCG_PORT_IDX:
		return &lcg_service;
	case CSG_PORT_IDX:
		return &csg_service;
	default:
		panic(MAKERESULT(RL_REINITIALIZE, RS_INVALIDSTATE, RM_RND,
					RD_INVALID_HANDLE));
		return NULL;

	}
}

static int
accept_client(Handle *client, const struct Service *service)
{
	TRY(svcAcceptSession(client, service->handle));

	if (nclients == MAX_SESSIONS_TOTAL) {
		svcCloseHandle(*client);
		return -1;
	}

	return 0;
}

static void
handle_commands(const struct Service *service)
{
	/* Blatant assumption: Correct argument provided. */

	u32 *cmdbuf = getThreadCommandBuffer();
	/* Command ids are indexed starting with 1, but pointer arithmetic is
	 * indexed starting with 0.
	 */
	u32 id = (cmdbuf[0] >> 16) - 1;

	/* This check abuses unsigned underflow being defined to become
	 * UINT32_MAX.
	 */
	if (id >= service->ncommands) {
		/* All services seem to error out on unknown commands with
		 * header code 0x40 (cmd 0, nparams 1, tparams 0) and result
		 * code 0xd900182f.
		 */
		cmdbuf[0] = 0x00000040;
		cmdbuf[1] = 0xd900182f;

		return;
	}

	(service->handlers[id])();
}

void
run_service(void)
{
	Result res;
	s32 idx;
	u32 notification_id;

	Handle service_handles[BASE_HANDLES + MAX_SESSIONS_TOTAL] = {lcg_service.handle, csg_service.handle, notifier};
	const struct Service *client_service_map[MAX_SESSIONS_TOTAL];
	const struct Service *service;
	Handle requester = 0;

	while (true) {
		/* Service is idle and waiting for requests.
		 *
		 * 0xffff0000 is a magic command id that makes
		 * svcReplyAndReceive to listen for new requests.
		 */
		if (!requester)
			*(getThreadCommandBuffer()) = 0xffff0000;

		if (R_FAILED(res = svcReplyAndReceive(&idx, service_handles,
						BASE_HANDLES + nclients,
						requester))) {
			/* Either failure or a Handle was closed. */

			if ((u32)res != ERR_SESSION_CLOSED_BY_REMOTE)
				panic(res);

			/* get_closing_client will svcBreak if one of the base
			 * handles is closed; we can safely assume we're looking
			 * at a client closing the session.
			 */
			idx = get_closing_client(service_handles, idx,
					requester);

			svcCloseHandle(service_handles[idx]);

			memmove(&service_handles[idx], &service_handles[idx + 1],
					(BASE_HANDLES + MAX_SESSIONS_TOTAL - idx - 1)
						* sizeof(*service_handles));

			idx -= BASE_HANDLES;
			memmove(&client_service_map[idx],
					&client_service_map[idx],
					(MAX_SESSIONS_TOTAL - idx - 1)
						* sizeof(*client_service_map));

			--nclients;
			requester = 0;

			continue;
		}

		switch (idx) {
		case NOTIFIER_IDX:
			/* Listen out only for the 0x100 event, which is the
			 * 3DS equivalent of SIGTERM. Break out of the loop if
			 * we receive it.
			 */

			if (R_FAILED(srvReceiveNotification(&notification_id))
					|| notification_id != 0x100) {
				/* Clear requester because we got no service
				 * commands during this loop.
				 */
				requester = 0;

				continue;
			}

			return;

		case LCG_PORT_IDX:
		case CSG_PORT_IDX:
			if ((service = find_service_from_index(idx)) == NULL) {
				panic(MAKERESULT(RL_REINITIALIZE, RS_INVALIDSTATE, RM_RND,
					RD_INVALID_POINTER));
				continue;
			}

			if (accept_client(&service_handles[BASE_HANDLES + nclients], service) != 0)
				continue;

			client_service_map[nclients++] = service;

			requester = 0;
			continue;

		default:
			break;
		}

		requester = service_handles[idx];
		handle_commands(client_service_map[idx - BASE_HANDLES]);
	}
}

void
deregister_service(void)
{
	srvUnregisterService(lcg_service.name);
	srvUnregisterService(csg_service.name);

	svcCloseHandle(lcg_service.handle);
	svcCloseHandle(csg_service.handle);
	svcCloseHandle(notifier);
}


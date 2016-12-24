#include <3ds.h>

#include "service.h"
#include "util.h"

int main(void)
{
	TRY(psInit());

	register_service();
	run_service();
	deregister_service();

	psExit();

	return 0;
	/* Control passed on to __ctru_exit. */
}

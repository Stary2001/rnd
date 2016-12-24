#ifndef RND_UTIL_H
#define RND_UTIL_H

void __attribute__((noreturn)) panic(Result res);

void write_invalid_arg_error(void);

#define TRY(call)	do {\
	if (R_FAILED(call)) {\
		panic(*(getThreadCommandBuffer() + 1));\
	}\
} while (0)

#endif


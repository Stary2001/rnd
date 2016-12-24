#ifndef RND_ARC4RANDOM_H
#define RND_ARC4RANDOM_H

#include <stdint.h>

uint32_t arc4random(void);
uint32_t arc4random_uniform(uint32_t upper_bound);
void arc4random_buf(void* _buf, size_t n);

#endif

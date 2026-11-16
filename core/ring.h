/* SPDX-License-Identifier: GPL-3.0-only
 * A single-producer, single-consumer ring of fixed-size items: the only way two contexts talk (the main loop and
 * the audio interrupt, CPU0 and CPU1). No lock: one writer per index. The barrier is a compiler barrier on the PC
 * and `csync` on the FM-1 (hal sets GB_BARRIER). Each item is read once, by one reader (G6). */
#pragma once
#include "gb.h"

#ifndef GB_BARRIER
#define GB_BARRIER() __asm__ volatile("" ::: "memory")
#endif

#define GB_RING(name, type, cap)                                                                   \
    typedef struct { type item[cap]; volatile uint32_t head, tail; volatile uint32_t lost; } name; \
    GB_INLINE void name##_init(name *r) { r->head = r->tail = r->lost = 0; }                        \
    GB_INLINE int name##_push(name *r, const type *v)                                              \
    {                                                                                              \
        uint32_t h = r->head;                                                                      \
        if (h - r->tail >= (cap)) { r->lost++; return 0; }                                         \
        r->item[h & ((cap) - 1)] = *v; GB_BARRIER(); r->head = h + 1; return 1;                    \
    }                                                                                              \
    GB_INLINE int name##_pop(name *r, type *v)                                                     \
    {                                                                                              \
        uint32_t t = r->tail;                                                                      \
        if (t == r->head) return 0;                                                                \
        GB_BARRIER(); *v = r->item[t & ((cap) - 1)]; GB_BARRIER(); r->tail = t + 1; return 1;      \
    }                                                                                              \
    GB_INLINE uint32_t name##_count(const name *r) { return r->head - r->tail; }

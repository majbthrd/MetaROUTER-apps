#ifndef HYPERCALLS_H
#define HYPERCALLS_H

#include "console.h"
#include "veth.h"
#include "system.h"

extern int vm_create_queue(unsigned id, unsigned irq, unsigned tx, unsigned rx);
extern int vm_release_queue(unsigned id);
extern int vm_running(void);
extern int vm_setup_irqs(unsigned *irqs, unsigned count);

#define VDMA_DONE	0x80000000

struct vdma_descr {
        unsigned addr;
        unsigned size;
        unsigned next;
} __attribute__((aligned(16)));

#define __read_32bit_c0_register(source, sel)                           \
({ unsigned int __res;                                                  \
        if (sel == 0)                                                   \
                __asm__ __volatile__(                                   \
                        "mfc0\t%0, " #source "\n\t"                     \
                        : "=r" (__res));                                \
        else                                                            \
                __asm__ __volatile__(                                   \
                        ".set\tmips32\n\t"                              \
                        "mfc0\t%0, " #source ", " #sel "\n\t"           \
                        ".set\tmips0\n\t"                               \
                        : "=r" (__res));                                \
        __res;                                                          \
})

#define __write_32bit_c0_register(register, sel, value)                 \
do {                                                                    \
        if (sel == 0)                                                   \
                __asm__ __volatile__(                                   \
                        "mtc0\t%z0, " #register "\n\t"                  \
                        : : "Jr" ((unsigned int)(value)));              \
        else                                                            \
                __asm__ __volatile__(                                   \
                        ".set\tmips32\n\t"                              \
                        "mtc0\t%z0, " #register ", " #sel "\n\t"        \
                        ".set\tmips0"                                   \
                        : : "Jr" ((unsigned int)(value)));              \
} while (0)

#define read_c0_count()         __read_32bit_c0_register($9, 0)

#endif


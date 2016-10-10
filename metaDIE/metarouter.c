//#include <linux/linkage.h> /* needed for "asmlinkage" */
#define asmlinkage 

/*
  derived from Mikrotik's patches to the Linux kernel for MetaROUTER
  http://www.mikrotik.com/download/metarouter/openwrt-metarouter-1.2.patch

  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.
*/

/*
"hypercalls" are calls from the MetaROUTER guest to the MetaROUTER hypervisor

they serve as the foundation for setting up "struct vdma_descr" data queues as well
as establishing the address of an unsigned value to act as an interrupt bit-field
*/

#define hypercall(name, nr, ...)                \
        asm(                                    \
                ".global " #name ";"            \
                ".align 2;"                     \
                ".set   push;"                  \
                ".set   noreorder;"             \
                ".type " #name ",@function;"    \
                ".ent " #name ",0;"             \
                #name ": .frame $sp,0,$ra;"     \
                "li $3, " #nr ";"               \
                "li $2, -22;"                   \
                "mtc0 $0, $1;"                  \
                "jr $ra;"                       \
                "nop;"                          \
                ".end " #name ";"               \
                ".size " #name ",.-" #name ";"  \
                ".set   pop"                    \
        );                                      \
        asmlinkage extern int name(__VA_ARGS__);

/*
   (queue) id  irq (bit)  description
   1           1          console
   2           n/a        inode (file system access)
   3           3          VETH (virtual Ethernet)
*/

hypercall(vm_create_queue, 4, unsigned id, unsigned irq, unsigned tx, unsigned rx);
hypercall(vm_release_queue, 5, unsigned id);

hypercall(vm_running, 6, void);

hypercall(vm_setup_irqs, 14, unsigned *irqs, unsigned count);


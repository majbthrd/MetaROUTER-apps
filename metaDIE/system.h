#ifndef SYSTEM_H
#define SYSTEM_H

extern void vm_init_system(volatile uint32_t *virqs);
extern void vm_service_system(void);

#endif


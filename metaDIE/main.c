#include <stdint.h>
#include <stdio.h>
#include "metarouter.h"

/*
  minimal MetaROUTER app example - main

  Copyright (C) 2013,2016 Peter Lawrence

  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/*

Theory of operation:

As a single thread, we can simplify the implementation to poll the VIRQ interrupt bit-field and act upon it

using the MIPS "wait" instruction blocks in the absence of interrupts

At present, there are only two VIRQ interrupts to service:
 IRQ1: virtual console
 IRQ3: virtual Ethernet

console.c provides getch() to read console input and the underlying functions to allow printf() to work
*/

static void *get_pc(void) { return __builtin_return_address(0); }

static void user_routine(void);

static uint32_t seen_irq = 0;
static uint32_t wake_count = 0;

int main(void)
{
	static volatile uint32_t virqs;

	vm_init_system(&virqs);

	/* set up data queue for interoperating with virtual console */
	vm_init_console();

	/* set up data queue for interoperating with virtual Ethernet */
	vm_init_veth();

	printf("Hello, world! %08x\r\n", get_pc());

	for (;;)
	{
		/* service interrupts, if any */
		if (virqs)
		{
			/* service virtual Ethernet interrupt */
			if (virqs & (1UL << 3))
			{
				vm_service_veth();
			}

			vm_service_system();

			seen_irq |= virqs;

			/* acknowledge any interrupts by clearing all bits */
			virqs = 0;
		}

		user_routine();

		asm volatile("wait");
		wake_count++;
	}

}

static void user_routine(void)
{
	int ch;

	ch = getch();

	if (ch >= 0)
	{
		/* print debugging item of interest in response to keystroke */
		unsigned val;
		val = read_c0_count();
		printf("%u, %u, %08x\r\n", val, wake_count, seen_irq);
		seen_irq = 0;
	}
}


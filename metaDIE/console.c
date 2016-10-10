#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include "metarouter.h"

/*
  minimal MetaROUTER app example - console driver

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

/* temporary value of debugging interest that will be displayed via console */
extern uint32_t veth_iftrack;

#define CONS_BUF_SIZE	4096

static volatile struct vdma_descr cons_tx_descr __attribute__((aligned(16)));
static volatile struct vdma_descr cons_rx_descr __attribute__((aligned(16)));

static uint8_t cons_rx_buffer[CONS_BUF_SIZE] __attribute__((aligned(16)));
static uint8_t cons_tx_buffer[CONS_BUF_SIZE] __attribute__((aligned(16)));

void vm_init_console(void)
{
	/* the MetaROUTER virtual console consists of two buffers, one each for transmit and receive */

	cons_rx_descr.addr = (unsigned) cons_rx_buffer;
	cons_rx_descr.size = CONS_BUF_SIZE;
	cons_rx_descr.next = (unsigned) &cons_rx_descr;
	
	cons_tx_descr.addr = (unsigned) cons_tx_buffer;
	cons_tx_descr.size = 0;
	cons_tx_descr.next = (unsigned) &cons_tx_descr;

	/* inform the host as to the location of the data structures that point to the buffers */
	vm_create_queue(1, 1, (unsigned) &cons_tx_descr, (unsigned) &cons_rx_descr);
}

int getch(void)
{
	unsigned count;
	int outcome = -1;
	static unsigned t = 0;

	count = cons_rx_descr.size;

	/* check if the user has entered any keystrokes... */
	if (cons_rx_descr.size & VDMA_DONE)
	{
		count = cons_rx_descr.size & ~VDMA_DONE;

		/* ... yes, the user has typed something */
		outcome = cons_rx_buffer[t++];

		if (t >= count)
		{
			t = 0;
			/* update the data structure to tell the host all keystrokes have been received */ 
			cons_rx_descr.size = CONS_BUF_SIZE;
		}
	}

	return outcome;
}

extern int __heap_start__, __heap_end__;

void *sbrk (int nbytes)
{
	static void *heap_ptr = (void *)&__heap_start__;
	unsigned heap_size = (unsigned)&__heap_end__ - (unsigned)heap_ptr;
	void *outcome;

	if (nbytes <= heap_size)
	{
		outcome = heap_ptr;
		heap_ptr += nbytes;
		return outcome;
	}

	errno = ENOMEM;
	return (void *)-1;
}

int close (int file)
{
	errno = EBADF;
  
	return -1; /* Always fails */
}

int write (int file, char *buf, int nbytes)
{
	static unsigned t = 0;
	int i;

	/*
	if the VDMA_DONE bit has been cleared by the host, we can reset the write pointer
	if not, we keep the existing value of t (effectively appending new data to existing data)
	*/
	if (!(cons_tx_descr.size & VDMA_DONE))
		t = 0;

	/* Output character at at time */
	for (i = 0; i < nbytes; i++)
	{
		/* do not exceed buffer limits */
		if (t >= CONS_BUF_SIZE)
			break;

		cons_tx_buffer[t++] = *buf++;
	}

	/* indicate to the host that data has been written (VDMA_DONE set) and indicate how much (value of t) */
	cons_tx_descr.size = VDMA_DONE | t;
        
	return nbytes;
}

int fstat (int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int isatty (int file)
{
	return 1;
}

int lseek (int file, int offset, int whence)
{
	return 0;
}

int read (int file, char *ptr, int len)
{
	return 0; /* EOF */
}


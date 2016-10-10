#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "metarouter.h"

/*
  minimal MetaROUTER app example - system driver

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

#define BUF_SIZE	256
#define BUF_COUNT	4

static volatile struct vdma_descr tx_chain[BUF_COUNT] __attribute__((aligned(16)));
static volatile struct vdma_descr rx_chain[BUF_COUNT] __attribute__((aligned(16)));

static uint8_t tx_buffers[BUF_COUNT][BUF_SIZE] __attribute__((aligned(16)));
static uint8_t rx_buffers[BUF_COUNT][BUF_SIZE] __attribute__((aligned(16)));

static unsigned cur_rx, cur_tx;

void vm_init_system(volatile uint32_t *virqs)
{
	unsigned i;

	/* tell host where interrupt bit-mask is kept */
	vm_setup_irqs((unsigned *)virqs, 32);

	for (i = 0; i < BUF_COUNT; i++)
	{
		rx_chain[i].addr = (unsigned) rx_buffers[i];
		rx_chain[i].size = BUF_SIZE;
		rx_chain[i].next = (unsigned) &rx_chain[i + 1];
		
		tx_chain[i].addr = (unsigned) tx_buffers[i];
		tx_chain[i].size = 0;
		tx_chain[i].next = (unsigned) &tx_chain[i + 1];
	}

	rx_chain[BUF_COUNT - 1].next = (unsigned) &rx_chain[0];
	tx_chain[BUF_COUNT - 1].next = (unsigned) &tx_chain[0];

	/* inform the host as to the location of the data structures that point to the buffers */
	vm_create_queue(0, 0, (unsigned) &tx_chain[0], (unsigned) &rx_chain[0]);
}

static int send_message(const unsigned char *buf, int len)
{
	if (tx_chain[cur_tx].size & VDMA_DONE)
		return 0;

	len = (len > BUF_SIZE) ? BUF_SIZE : len;

	memcpy(tx_buffers[cur_tx], buf, len);
	tx_chain[cur_tx].size = len | VDMA_DONE;

	cur_tx = (cur_tx + 1) % BUF_COUNT;

	return len;
}

static int recv_message(char *buf, int len)
{
	int l;

	if (!(rx_chain[cur_rx].size & VDMA_DONE))
		return 0;

	l = rx_chain[cur_rx].size & ~VDMA_DONE;
	if (l < len)
		len = l;
	memcpy(buf, rx_buffers[cur_rx], len);

	rx_chain[cur_rx].size = BUF_SIZE;
	cur_rx = (cur_rx + 1) % BUF_COUNT;

	return len;
}

void vm_service_system(void)
{
	char buffer[BUF_SIZE];
	int len;

	for (;;)
	{
		len = recv_message(buffer, sizeof(buffer));

		if (len <= 0)
			break;

		send_message(buffer, len);

		len++;
		if (len >= BUF_SIZE)
			len = BUF_SIZE - 1;
		
		buffer[len] = '\0';

		printf("--> %s\r\n", buffer);

	}
}


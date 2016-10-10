#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "metarouter.h"

/*
  minimal MetaROUTER app example - network driver

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

/* the depth of packet buffer entries appears to be entirely up to the guest (and available memory) */
#define VETH_TXBUF_COUNT 128
#define VETH_RXBUF_COUNT 128

/*
Mikrotik defines the maximum packet size as 1600, but what about jumbo packets!?

there also doesn't appear to be a way to signal the MTU for a given virtual interface
presumably, the user must set this manually via the RouterOS interface?
*/
#define VETH_BUFFER_SIZE	1600 /* must be multiple of 16 to ensure aligned(16) */

/* data structure used on command (non-network) packets */
struct ctrl_msg
{
	uint32_t cmd;
	uint16_t id;
	uint8_t hwaddr[6];
} __attribute__((packed));

/* enumeration of "cmd" field values in "struct ctrl_msg" */
#define VETH_CMD_NEWIFACE	0
#define VETH_CMD_DELIFACE	1

static volatile struct vdma_descr veth_rx_descr[VETH_RXBUF_COUNT] __attribute__((aligned(16)));
static volatile struct vdma_descr veth_tx_descr[VETH_TXBUF_COUNT] __attribute__((aligned(16)));

static uint8_t veth_rx_buffer[VETH_RXBUF_COUNT][VETH_BUFFER_SIZE] __attribute__((aligned(16)));
static uint8_t veth_tx_buffer[VETH_TXBUF_COUNT][VETH_BUFFER_SIZE] __attribute__((aligned(16)));

static unsigned veth_cur_rx = 0;
static unsigned veth_cur_tx = 0;

uint32_t veth_iftrack;

void vm_init_veth(void)
{
	unsigned i;

	veth_iftrack = 0;

	/* generate a linked list of transmit buffers */

	for (i = 0; i < VETH_TXBUF_COUNT; ++i) {
		veth_tx_descr[i].addr = (unsigned) 0;
		veth_tx_descr[i].size = 0;
		veth_tx_descr[i].next = (unsigned) &veth_tx_descr[i + 1];
	}
	veth_tx_descr[VETH_TXBUF_COUNT - 1].next = (unsigned) &veth_tx_descr[0];

	/* generate a linked list of receive buffers */

	for (i = 0; i < VETH_RXBUF_COUNT; ++i) {
		veth_rx_descr[i].addr = (unsigned) veth_rx_buffer[i];
		veth_rx_descr[i].size = VETH_BUFFER_SIZE;
		veth_rx_descr[i].next = (unsigned) &veth_rx_descr[i + 1];
	}
	veth_rx_descr[VETH_RXBUF_COUNT - 1].next = (unsigned) &veth_rx_descr[0];
	
	/* provide host with buffers for virtual Ethernet data queue */
	vm_create_queue(3, 3, (unsigned) &veth_tx_descr[0], (unsigned) &veth_rx_descr[0]);
}

static uint8_t *vm_transmit_buffer(uint16_t ifnum)
{
	uint8_t *pnt;

	/* IF 0 is for management messages only */
	if (0 == ifnum)
		return NULL;

	/* we can't use the buffer if it is still queued */
	if (veth_tx_descr[veth_cur_tx].size & VDMA_DONE)
		return NULL;

	pnt = (void *)veth_tx_buffer[veth_cur_tx];
	/* first two bytes of any message contain the ifnum */
	memcpy(pnt, &ifnum, 2); pnt += 2;
	
	return pnt;
}

static void vm_transmit_trigger(unsigned length)
{
	veth_tx_descr[veth_cur_tx].addr = (unsigned)veth_tx_buffer[veth_cur_tx];
	veth_tx_descr[veth_cur_tx].size = (length + 2) | VDMA_DONE;

	/* hop to the next transmit buffer */
	veth_cur_tx = ++veth_cur_tx % VETH_TXBUF_COUNT;
}

static void service_packet(uint8_t *data, unsigned length);

void vm_service_veth(void)
{
	unsigned length;
	struct ctrl_msg *cmsg;
	uint16_t veth_ifnum, i;
	uint8_t *pktpnt;

	for (;;)
	{
		if (veth_rx_descr[veth_cur_rx].size & VDMA_DONE)
		{
			length = veth_rx_descr[veth_cur_rx].size & ~VDMA_DONE;
			if (length > sizeof(uint16_t))
			{
				veth_ifnum = *((uint16_t *)(veth_rx_descr[veth_cur_rx].addr));

				pktpnt = ((uint8_t *)veth_rx_descr[veth_cur_rx].addr) + 2;
				length -= 2;

				if (0 == veth_ifnum)
				{
					/* packet is a control (non-network) packet */

					if (length >= sizeof(struct ctrl_msg) )
					{
						cmsg = (struct ctrl_msg *)pktpnt;
						switch (cmsg->cmd)
						{

						case VETH_CMD_NEWIFACE: /* host has added a new interface */
							veth_iftrack |= (1UL << (cmsg->id - 1));
							break;

						case VETH_CMD_DELIFACE: /* host has removed an interface */
							veth_iftrack &= ~(1UL << (cmsg->id - 1));
							break;

						}
					}
				}
				else if (length <= VETH_BUFFER_SIZE)
				{
					/* packet is a network packet, and 
					veth_ifnum indicates which interface it came in on */
					service_packet(pktpnt, length);
				}
			}

			/* free current receive buffer entry */
			veth_rx_descr[veth_cur_rx].size = VETH_BUFFER_SIZE;

			/* hop to the next receive buffer */
			veth_cur_rx = ++veth_cur_rx % VETH_RXBUF_COUNT;
		}
		else
		{
			/* there are no more receive buffers to examine, so we bail the loop */
			break;
		}
	}
}

static void service_packet(uint8_t *input, unsigned length)
{
	unsigned index;
	uint8_t *output;

	/* reject packets smaller than MAC + IP packet header */
	if (length < 54)
		return;

	/* reject packets not sent to a privately assigned MAC address */
	if (0x02 != input[0])
		return;

	/* reject packets not IP over Ethernet */
	if ( (0x08 != input[12]) || (0x00 != input[13]) )
		return;

	/* reject packets not IPv4 */
	if (0x40 != (input[14] & 0xF0))
		return;

	/* reject packets not ICMP */
	if (0x01 != input[23])
		return;

//	printf("%02x\n", input[23]);

//	return;

	output = vm_transmit_buffer(1);

	if (NULL == output)
		return;

	/* MAC source becomes MAC destination */
	memcpy(output + 0, input + 6, 6);
	/* MAC destination becomes MAC source */
	memcpy(output + 6, input + 0, 6);
	/* copy first part of IP header as-is (and don't worry about header checksum) */
	memcpy(output + 12, input + 12, 14);
	/* IP src becomes dst IP */
	memcpy(output + 26, input + 30, 4);
	/* dst IP becomes IP src */
	memcpy(output + 30, input + 26, 4);
	/* ICMP Response to ICMP Request */
	output[34] = input[34] & 0x0F;
	/* rest of packet is copied as-is */
	memcpy(output + 35, input + 35, length - 35);

	vm_transmit_trigger(length);
}


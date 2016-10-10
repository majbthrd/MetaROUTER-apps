/*
  minimal MetaROUTER app example - bootloader

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

	.set	noreorder

	/*
	This is provision for exception vectors, if indeed MetaROUTER supports them.
	0 is a 'nop' instruction, so worse-case should be a restart.
	*/
	.space 0x200, 0

	.global	kernel_entry

kernel_entry:
	di

	/* zero-ize .bss section */

	la	$t0, __bss_start__
	la	$t1, __bss_end__
	b       _bss_check
	nop

_bss_init:
	sw	$zero, 0($t0)
	addu    $t0, 4
_bss_check:
	bltu	$t0, $t1, _bss_init
	nop

	/* set the stack pointer */

	la	$sp, __stack_end__

	/* branch to main */

	jal	main

loop:
	wait
	j	loop


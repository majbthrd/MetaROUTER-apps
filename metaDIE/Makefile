#
# minimal MetaROUTER app example - Makefile
#
#  Copyright (C) 2013 Peter Lawrence
#
#  All rights reserved.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License version 2.1, as
#  published by the Free Software Foundation.  This program is
#  distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
#  License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.


# adjust this to your cross-compiler path of choice
TC_PREFIX := $(HOME)/x-tools/mips-metarouter-elf/bin/mips-metarouter-elf-
NEWLIB_PATH := $(HOME)/x-tools/mips-metarouter-elf/mips-metarouter-elf/lib

ALL: test.tgz

TINCAN_OBJS := start.o main.o metarouter.o console.o veth.o system.o

OTHER_DEPENDS = Makefile tincan.ld

%.o: %.c $(OTHER_DEPENDS)
	$(TC_PREFIX)gcc -g3 -G0 -mno-gpopt -c -march=24kc -fno-pic -mno-abicalls -mlong-calls -msoft-float $< -o $@

%.o: %.S  $(OTHER_DEPENDS)
	$(TC_PREFIX)as -march=24kc -msoft-float $< -o $@

kernel: $(TINCAN_OBJS)  $(OTHER_DEPENDS)
	$(TC_PREFIX)ld -G0 -T tincan.ld -o tincan.elf $(TINCAN_OBJS) -L$(NEWLIB_PATH) -lc -lm
	$(TC_PREFIX)strip tincan.elf -o kernel
	$(TC_PREFIX)objdump -S -d tincan.elf > listing.txt

test.tgz: kernel
	tar czf test.tgz ./kernel

clean:
	rm -f *.o
	rm -f *.elf
	rm -f kernel test.tgz
	rm -f listing.txt


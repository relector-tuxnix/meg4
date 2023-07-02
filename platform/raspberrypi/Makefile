#
# meg4/platform/raspberrypi/Makefile
#
# Copyright (C) 2023 bzt
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# @brief Creating a bootable MEG-4 disk image for Raspberry Pi
#

# ----- configure your MEG-4 OS image -----
IMGSIZE?=256
KBDMAP?=gb
LANG?=en
FLOPPYDEV?=/dev/mmcblk0p1
# ----- end of configuration -----

all: meg4.iso

### compile MEG-4 as an init process ###
../fbdev_alsa/meg4:
	USE_INIT=1 KBDMAP=$(KBDMAP) LANG=$(LANG) FLOPPYDEV=$(FLOPPYDEV) make --no-print-directory -C ../fbdev_alsa all

rootfs/init: ../fbdev_alsa/meg4
	@mkdir -p rootfs 2>/dev/null || true
	cp -f ../fbdev_alsa/meg4 rootfs/init

### get Raspberrypi firmware ###
firmware.zip:
	wget -c https://github.com/raspberrypi/firmware/archive/refs/heads/master.zip -O firmware.zip

boot/kernel8.img: firmware.zip
	unzip -uo firmware.zip 'firmware-master/boot/*' 'firmware-master/modules/*'
	@rm -rf boot rootfs/lib/modules 2>/dev/null || true
	mv firmware-master/boot boot
	@mkdir -p rootfs/lib 2>/dev/null || true
	mv firmware-master/modules rootfs/lib/modules
	@rm -rf firmware-master
	@# make sure the kernel's timestamp is newer than firmware.zip's
	@touch boot/kernel8.img

### create initial ramdisk ###
boot/initrd: rootfs/init
	@mkdir rootfs/dev rootfs/mnt rootfs/proc rootfs/sys rootfs/tmp
	cd rootfs && find . | cpio -R root:root -H newc -o | gzip > ../boot/initrd

### construct disk image ###
meg4.iso: boot/kernel8.img boot/initrd
	@printf "dtparam=audio=on\nmax_framebuffers=2\narm_64bit=1\ninitramfs initrd followkernel" >boot/config.txt
	@printf "console=serial0,115200 quiet" >boot/cmdline.txt
	@mkdir -p boot/MEG-4 2>/dev/null || true
	@cp -r ../../public/floppies/* boot/MEG-4
	dd if=/dev/zero of=meg4.iso count=$(IMGSIZE) bs=1048576
	printf "n\np\n1\n2048\n\nt 1\nc\nw\nq\n" | fdisk meg4.iso
	mkfs.vfat -F 32 -n "MEG-4 OS" --offset 2048 meg4.iso
ifneq ($(wildcard /usr/bin/mcopy),)
	@# if mtools is installed, use that (available to all users)
	cd boot && mcopy -i ../meg4.iso@@1M * ::
else
	@# use standard Linux tools otherwise (unfortunately requires root priviledge...)
	@mkdir mnt 2>/dev/null
	sudo mount -o loop,offset=1048576,user,umask=000 meg4.iso mnt
	cd boot && cp -r * ../mnt
	sudo umount mnt
	rm -rf mnt
endif

clean:
	rm -rf rootfs boot firmware.zip meg4.iso 2>/dev/null || true


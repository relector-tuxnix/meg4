#
# meg4/platform/baremetal/Makefile
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
# @brief Creating a bootable MEG-4 disk image
#

# ----- configure your MEG-4 OS image -----
IMGSIZE?=256
KBDMAP?=us
LANG?=en
FLOPPYDEV?=/dev/sda1
KERNEL?=6.4.2
# ----- end of configuration -----

qemu=qemu-system-$(shell uname -m) -m 256 -drive file=meg4.iso,format=raw -device virtio-mouse -serial stdio
# 1280 x 800 x 32
FBSET=vga=0x37a

all: meg4.iso

### compile MEG-4 as an init process ###
rootfs/init:
	USE_INIT=1 KBDMAP=$(KBDMAP) LANG=$(LANG) FLOPPYDEV=$(FLOPPYDEV) make --no-print-directory -C ../fbdev_alsa clean all
	@mkdir -p rootfs 2>/dev/null || true
	cp -f ../fbdev_alsa/meg4 rootfs/init
	strip rootfs/init

### get the Linux kernel ###
kernel.tar.xz:
	wget -c http://kernel.org/pub/linux/kernel/v$(word 1,$(subst ., ,$(KERNEL))).x/linux-$(KERNEL).tar.xz -O kernel.tar.xz

linux-$(KERNEL)/Kconfig: kernel.tar.xz
	tar -xvf kernel.tar.xz
	@# make sure Kconfig is newer than kernel.tar.xz
	@touch linux-$(KERNEL)/Kconfig

linux-$(KERNEL)/.config: linux-$(KERNEL)/Kconfig
ifneq ($(wildcard $(KCONFIG)),)
	cp $(KCONFIG) linux-$(KERNEL)/.config
else
	make -C linux-${KERNEL} -j4 defconfig
	sed -i "s|.*CONFIG_NET=y.*|# CONFIG_NET is not set|" linux-$(KERNEL)/.config
	sed -i "s/^CONFIG_DEBUG_KERNEL.*/\\# CONFIG_DEBUG_KERNEL is not set/" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_KERNEL_XZ is not set.*|CONFIG_KERNEL_XZ=y|" linux-$(KERNEL)/.config
	sed -i "s|.*CONFIG_KERNEL_GZIP=y.*|# CONFIG_KERNEL_GZIP is not set|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_FB is not set.*|CONFIG_FB=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_FB_VESA is not set.*|CONFIG_FB_VESA=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_FB_EFI is not set.*|CONFIG_FB_EFI=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_FB_SIMPLE is not set.*|CONFIG_FB_SIMPLE=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM is not set*|CONFIG_DRM=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM_FBDEV_EMULATION is not set*|CONFIG_DRM_FBDEV_EMULATION=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM_BOCHS is not set*|CONFIG_DRM_BOCHS=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM_CIRRUS_QEMU is not set*|CONFIG_DRM_CIRRUS_QEMU=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM_VBOXVIDEO is not set*|CONFIG_DRM_VBOXVIDEO=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM_SIMPLEDRM is not set*|CONFIG_DRM_SIMPLEDRM=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM_RADEON is not set*|CONFIG_DRM_RADEON=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM_AMDGPU is not set*|CONFIG_DRM_AMDGPU=y|" linux-$(KERNEL)/.config
	sed -i "s|.*# CONFIG_DRM_I915 is not set*|CONFIG_DRM_I915=y|" linux-$(KERNEL)/.config
endif
	sed -i "s|.*CONFIG_DEFAULT_HOSTNAME=*|CONFIG_DEFAULT_HOSTNAME=\"MEG4\"|" linux-$(KERNEL)/.config

boot/vmlinuz: linux-$(KERNEL)/.config
	@rm -rf rootfs/lib/modules 2>/dev/null || true
	@mkdir -p boot rootfs/lib/modules 2>/dev/null || true
	make -C linux-$(KERNEL) -j4 bzImage modules
	make INSTALL_PATH=../boot INSTALL_MOD_PATH=../rootfs -C linux-$(KERNEL) install modules_install
	@rm rootfs/lib/modules/$(KERNEL)/build rootfs/lib/modules/$(KERNEL)/source 2>/dev/null || true

### create initial ramdisk ###
boot/initrd: rootfs/init
	@mkdir rootfs/dev rootfs/mnt rootfs/proc rootfs/sys rootfs/tmp 2>/dev/null || true
	@chmod +x rootfs/init
	cd rootfs && find . | cpio -R root:root -H newc -o | gzip > ../boot/initrd

### get boot loader
simpleboot/simpleboot:
	git clone https://gitlab.com/bztsrc/simpleboot

### construct disk image ###
meg4.iso: boot/vmlinuz boot/initrd simpleboot/simpleboot
	@mkdir -p boot/MEG-4 2>/dev/null || true
	@cp -r ../../public/floppies/* boot/MEG-4
	@cp ../../src/misc/logo.tga boot/meg4.tga
	@printf "kernel vmlinuz $(FBSET) console=ttyS0,115200 quiet\nmodule initrd\nbootsplash #444444 meg4.tga" >boot/simpleboot.cfg
	simpleboot/distrib/simpleboot -p $(IMGSIZE) boot meg4.iso

package: meg4.iso
	@printf "#/bin/sh\n\nmkdir -p mnt\nsudo mount -o loop,offset=1048576,user,umask=000 meg4.iso mnt\n" > mount.sh && chmod +x mount.sh
	@printf "#/bin/sh\n\n$(qemu)\n" > run.sh && chmod +x run.sh
	tar -czvf ../../meg4-$(shell uname -m).iso.tgz meg4.iso mount.sh run.sh
	@rm mount.sh run.sh

clean:
	rm -rf rootfs boot linux-* kernel.tar.xz simpleboot meg4.iso 2>/dev/null || true

run: meg4.iso
	$(qemu)

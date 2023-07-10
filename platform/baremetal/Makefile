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
SYSLINUX?=6.03
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
syslinux.tar.xz:
	wget -c http://kernel.org/pub/linux/utils/boot/syslinux/syslinux-$(SYSLINUX).tar.xz -O syslinux.tar.xz

### construct disk image ###
meg4.iso: boot/vmlinuz boot/initrd syslinux.tar.xz
	@mkdir -p boot/MEG-4 2>/dev/null || true
	@cp -r ../../public/floppies/* boot/MEG-4
	@printf "default vmlinuz initrd=/initrd $(FBSET) console=ttyS0,115200 quiet\n" >boot/syslinux.cfg
	dd if=/dev/zero of=meg4.iso count=$(IMGSIZE) bs=1048576
	printf "n\np\n1\n2048\n\nt 1\nc\na\nw\nq\n" | fdisk meg4.iso
	mkfs.vfat -F 32 -n "MEG-4 OS" --offset 2048 meg4.iso
	tar --strip-components=3 -xvf syslinux.tar.xz syslinux-$(SYSLINUX)/bios/mbr/mbr.bin && dd bs=440 count=1 conv=notrunc if=mbr.bin of=meg4.iso && rm mbr.bin
ifneq ($(wildcard /usr/bin/mcopy),)
	@# if mtools is installed, use that (available to all users)
	tar --strip-components=3 -xvf syslinux.tar.xz syslinux-$(SYSLINUX)/bios/mtools/syslinux && ./syslinux -t 1048576 -i meg4.iso && rm syslinux
	cd boot && mcopy -i ../meg4.iso@@1M -s * ::
else
	@# use standard Linux tools otherwise (unfortunately requires root priviledge...)
	tar --strip-components=3 -xvf syslinux.tar.xz syslinux-$(SYSLINUX)/bios/linux/syslinux-nomtools && ./syslinux-nomtools -t 1048576 -i meg4.iso && rm syslinux-nomtools
	@mkdir mnt 2>/dev/null
	sudo mount -o loop,offset=1048576,user,umask=000 meg4.iso mnt
	cd boot && cp -r * ../mnt
	sudo umount mnt
	rm -rf mnt
endif

## Ok, I got fed up with this buggy shitparade called grub, switching to syslinux
#	@printf "set default=0\nset timeout=1\nroot (hd0,0)\nmenuentry \"MEG-4 OS\" {\n linux /vmlinuz\n initrd /initrd\n}\n" >boot/grub/grub.cfg
#	grub-mkrescue --compress=xz -o meg4.iso boot
## AHHHHHH such a bloated epic failure... it reads my host (!) system's configs, even though I explicitly told it NOT to!
#	@printf "insmod part_gpt\ninsmod part_msdos\ninsmod fat\ninsmod all_video\nset default=0\nset timeout=1\nset root='hd0,0'\nmenuentry \"MEG-4 OS\" {\n linux /vmlinuz\n initrd /initrd\n}\n" >boot/grub/grub.cfg
#	grub-install --compress=xz --boot-directory=./boot --target=i386-pc meg4.iso

package: meg4.iso
	@printf "#/bin/sh\n\nmkdir -p mnt\nsudo mount -o loop,offset=1048576,user,umask=000 meg4.iso mnt\n" > mount.sh && chmod +x mount.sh
	@printf "#/bin/sh\n\n$(qemu)\n" > run.sh && chmod +x run.sh
	tar -czvf ../../meg4-$(shell uname -m).iso.tgz meg4.iso mount.sh run.sh
	@rm mount.sh run.sh

clean:
	rm -rf rootfs boot linux-* kernel.tar.xz syslinux.tar.xz meg4.iso 2>/dev/null || true

run: meg4.iso
	$(qemu)

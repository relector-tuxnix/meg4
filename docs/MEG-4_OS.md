MEG-4 Operating System
======================

It is possible to run MEG-4 as a standalone Operating System. See [raspberrypi](../platform/raspberrypi) and
[baremetal](../platform/baremetal) for an example. Here are the required steps:

Step 1: compile `meg4` as an init process
-----------------------------------------

Go to the [fbdev_alsa](../platform/fbdev_alsa) directory, and compile MEG-4 with the `USE_INIT=1` flag. Specify the
other parameters as well, like the keyboard layout or the floppy device (which is not an actual floppy device file,
instead it must point to the boot partition you're about to create in step 4, see below).

Step 2: compile the Linux kernel
--------------------------------

Download, configure and compile the [Linux kernel](https://github.com/torvalds/linux) for your desired hardware. This
should result in a kernel file, named `vmlinuz` (or more likely `bzImage` these days) and directories with the available
kernel modules. You can copy all .ko files into a directory with
```
make INSTALL_MOD_PATH=(destination directory) modules_install
```

Step 3: create an initrd
------------------------

First, create a temporary directory, copy `meg4` (from step 1) as `init` into. Make sure that it's executable `chmod +x init`.
Then create the directory `lib/modules/(kernel version)/` under it, and copy the kernel modules you compiled in step 2 there.
Create some more directories for mount points: `dev`, `proc`, `sys`, `tmp` and `mnt`. Finally, using `cpio -H newc > initrd`,
create an archive file from the contents of that temporary directory.

```
initrd archive:
  dev/
  lib/modules/*
  mnt/
  proc/
  sys/
  tmp/
  init
```

Step 4: create boot partition
-----------------------------

In theory you can use whatever partitioning scheme and file system you wish, but in practice this is going to be a FAT32
partition with a GPT partitioning table referencing it as "EFI System Partition".

Copy the `vmlinuz` kernel file (from step 2) and the `initrd` archive file (from step 3) into the root directory of this
partition. Also create a directory named `MEG-4` there (you can place demo floppy images into this directory if you'd like).

```
partition's root directory:
  MEG-4/
  initrd
  vmlinuz
```

Depending on your hardware, you might have to copy other files too to the boot partition (like .dtb definitions for example).

IMPORTANT NOTE: Double check that you've compiled meg4 (in step 1) with `FLOPPYDEV` being the same device file as how the kernel
sees this boot partition *when it runs* (you can also use the `UUID=` variant with the file system's unique ID instead).

Step 5: install boot loader
---------------------------

You'll also have to install a target platform specific boot loader into the image (GRUB, syslinux, isolinux, LILO, Simpleboot,
whatever). The required steps vary from loader to loader, but they usually have a simple text config file to specify what OS to
boot. Tell this boot loader to load your Linux kernel along with the initial ramdisk from the boot partition, and pass a command
line to the kernel to set up the framebuffer.

For example, if your hardware uses UEFI, then you'll probably need the loader in "EFI/BOOT/BOOTX86.EFI" (this could be the kernel
itself, if compiled with EFI-stub). On legacy BIOS machines with syslinux, you'll need "ldlinux.sys" and "ldlinux.c32", and in
"syslinux.cfg" something like:
```
default vmlinuz initrd=/initrd vga=0x37a quiet
```
For GRUB, you'll need a directory named "grub", with lots and lots of files, among which "grub/grub.cfg" being the configuration
file, containing something like:
```
menuentry "MEG-4 OS" {
    linux  /vmlinuz vga=0x37a quiet
    initrd /initrd
}
```
Because I couldn't get syslinux to work on UEFI, and GRUB is way too bloated to my taste, I've also written a boot loader,
this needs a "simpleboot.cfg" file on the boot partition like:
```
kernel /vmlinuz vga=0x37a quiet
module /initrd
```
On Raspberry Pi, the loader has multiple files too, "bootcode.bin" and "start.elf", you have to specify the initrd in "config.txt",
and you also have to rename your "vmlinuz" kernel file to "kernel8.img", otherwise it won't work.

This step is very platform and loader specific, but the end of the day it just loads the kernel and the initrd on boot.

Step 6: burn the image file to a real disk
------------------------------------------

You can share the created disk image file on the net and you can boot it in a virtual machine. But if you want to use it on an
actual real machine, then there's one more step ahead.

Pick a storage that your machine can boot from (USB stick probably, but could be an SDCard or an SSD too). Use `dd` or
[USBImager](https://bztsrc.gitlab.io/usbimager/) to write the created disk image file to that storage, and boot.

IMPORTANT NOTE: just copying the image file to the disk will not do. You MUST use a special tool that low-level writes the
image file's data to the disk, sector by sector, bit by bit.

MEG-4 OS (Bare Metal)
=====================

This directory contains a special Makefile which does not integrate MEG-4 with a library, instead it generates a
[bootable disk image](../../docs/MEG-4_OS.md) using the [fbdev port](../fbdev_alsa). (See [raspberrypi](../raspberrypi)
about creating an image specifically for the Raspberry Pi.)

IMPORTANT NOTE: no cross-compiler configured, so you must run this under the same architecture as your target's.

Compilation options
-------------------

| Command               | Description                                                           |
|-----------------------|-----------------------------------------------------------------------|
| `make`                | Generate the `meg4.iso` disk image file                               |
| `make run`            | Run MEG-4 OS, boot the generated disk image in qemu                   |
| `make package`        | Create a package from the compiled disk image                         |
| `IMGSIZE=x make`      | Specify the disk image's size in Megabytes (default 256 M)            |
| `KERNEL=x.x.x make`   | Specify the Linux kernel version to be used (default 6.4.2)           |
| `KCONFIG=x make`      | Provide your own kernel configuration file (default `make defconfig`) |
| `FLOPPYDEV=xx make`   | Select the floppy device (default `/dev/sda1`)                        |
| `LANG=xx make`        | Select interface's language (default `en`, English)                   |
| `KBDMAP=xx make`      | Select keyboard layout map (default `us`, International)              |

You can write the resulting `meg4.iso` with `dd` or [USBImager](https://bztsrc.gitlab.io/usbimager/) to a storage and
boot that on a real machine. Or alternatively `make run` will start in a virtual machine, provided you have qemu installed.

NOTE: for simplicity, the boot loader is configured for BIOS. On a real hardware with EFI, you'll have to enable EFI CSM,
or modify the Makefile to use the EFI version of syslinux. Alternatively you could try to use grub (I gave up on that,
because no matter what I did, grub read the host (!) system's configuration files, which of course will never work for the
image. And modifying the host computer's boot config just to install grub on an image is an insane risk I refuse to take).

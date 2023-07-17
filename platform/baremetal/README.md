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

NOTE: the image uses [Simpleboot](https://gitlab.com/bztsrc/simpleboot), which can boot on UEFI as well as on BIOS machines.

MEG-4 Raspberry Pi Port
=======================

This directory contains a special Makefile which does not integrate MEG-4 with a library, instead it generates a
[bootable disk image](../../docs/MEG-4_OS.md) for the Raspberry Pi using the [fbdev port](../fbdev_alsa).

IMPORTANT NOTE: no cross-compiler configured, so you must run this under RaspiOS (or any other ARM64 Linux).

Compilation options
-------------------

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Generate the `meg4.iso` disk image file                    |
| `IMGSIZE=x make`      | Specify the disk image's size in Megabytes (default 256 M) |
| `FLOPPYDEV=xx make`   | Select the floppy device (default `/dev/mmcblk0p1`)        |
| `LANG=xx make`        | Select interface's language (default `en`, English)        |
| `KBDMAP=xx make`      | Select keyboard layout map (default `gb`, British)         |

You can write the resulting `meg4.iso` with `dd` or [USBImager](https://bztsrc.gitlab.io/usbimager/) to an SDCard,
and then you'll be able to boot that card on a real RPi3+ machine like you would boot any other RaspiOS card.

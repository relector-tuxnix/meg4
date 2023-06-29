#
# meg4/platform/fbdev_alsa/Makefile
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
# @brief The Linux fbdev + ALSA Makefile
#

BACKEND=fbdev
PACKAGE=Linux
EMBED=1

ifneq ("$(wildcard alsa-lib/*)","")
 CFLAGS += -I./alsa-lib/include
 EXTRA += ./alsa-lib/src/.libs/libasound.a
else
 LIBS += -lasound
endif
LIBS += -lm -ldl -lpthread

CFLAGS=-Wno-long-long
ifneq ($(USE_INIT),)
 CFLAGS+=-DUSE_INIT=1
 LDFLAGS=-static
ifneq ($(LANG),)
 CFLAGS+=-DLANG="\"$(LANG)\""
endif
ifneq ($(FLOPPYDEV),)
 CFLAGS+=-DFLOPPYDEV="\"$(FLOPPYDEV)\""
endif
endif
ifneq ($(KBDMAP),)
 CFLAGS+=-DKBDMAP="\"$(KBDMAP).h\""
endif

include ../common.mk

ifneq ("$(wildcard alsa-lib/*)","")
alsa-lib/src/.libs/libasound.a:
ifeq ($(wildcard alsa-lib/Makefile),)
	@cd alsa-lib && GITCOMPILE_NO_MAKE=1 ./gitcompile --disable-shared --enable-static --disable-mixer --enable-pcm --disable-rawmidi --disable-seq --disable-ucm --disable-alisp --disable-old-symbols --disable-python
endif
	@make --no-print-directory -s -C alsa-lib all
endif

static-asound:
	git clone https://github.com/alsa-project/alsa-lib.git

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

LIBS += -lm -lpthread

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

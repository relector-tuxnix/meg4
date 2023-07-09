#
# meg4/platform/libretro/Makefile
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
# @brief The RetroArch / libretro Makefile
#

BACKEND=retroarch
EMBED=1

# MacOS
ifneq ("$(wildcard /Library/Frameworks/*)","")
 PACKAGE = MacOS
 LDFLAGS = -dynamiclib
 CFLAGS = -fPIC -I. -D_MACOS_=1 -D__LIBRETRO__ -x objective-c -fobjc-exceptions
 LIBS = -lm
 TARGET=meg4_libretro.dynlib
else
 # Windows MinGW
 ifneq ("$(wildcard /bin/*.exe)","")
  PACKAGE = Win
  LDFLAGS += -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=link.ld -Wl,--no-undefined
  LIBS += -Wl,--nxcompat -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
  CFLAGS += -fPIC -I. -D__LIBRETRO__
  LIBS += -luser32 -lshlwapi -lws2_32
  TARGET=meg4_libretro.dll
 else
  # Linux
  PACKAGE = Linux
  LDFLAGS = -shared -Wl,--version-script=link.ld -Wl,--no-undefined
  CFLAGS += -fPIC -I. -D__LIBRETRO__
  LIBS += -lm -lpthread
  TARGET=meg4_libretro.so
 endif
endif

USE_C99=1
include ../common.mk

libretro:
	wget https://raw.githubusercontent.com/libretro/libretro-common/master/include/libretro.h -O libretro.h

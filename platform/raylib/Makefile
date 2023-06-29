#
# meg4/platform/raylib/Makefile
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
# @brief The raylib Makefile
#

BACKEND=raylib

# MacOS
ifneq ("$(wildcard /Library/Frameworks/raylib*)","")
 PACKAGE = MacOS
 CFLAGS = -I/Library/Frameworks/raylib.framework/Headers -I/Library/Frameworks/raylib.framework/Versions/A/Headers -D_MACOS_=1 -x objective-c -fobjc-exceptions
 LIBS = -F/Library/Frameworks -framework raylib -framework GL -framework OpenAL -framework AppKit
else
 # Windows MinGW
 ifneq ("$(wildcard /bin/*.exe)","")
  PACKAGE = Win
  LDFLAGS += -mwindows -static-libgcc
  LIBS += -Wl,--nxcompat -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
  ifneq ("$(wildcard raylib/*)","")
   CFLAGS += -I./raylib/src
   EXTRA += ./raylib/src/raylib.a
  else
   LIBS += -lraylib
  endif
  LIBS += -lGL -luser32 -lcomdlg32 -lshlwapi -lws2_32
 else
  # Linux
  PACKAGE = Linux
  ifneq ("$(wildcard raylib/*)","")
   CFLAGS += -I./raylib/src
   EXTRA += ./raylib/src/libraylib.a
  else
   LIBS += -lraylib
  endif
  LIBS += -lGL -lm -ldl -lpthread
 endif
endif

USE_C99=1
include ../common.mk

ifneq ("$(wildcard raylib/*)","")
raylib/src/libraylib.a: raylib/src/config.h
	@make --no-print-directory -s -C raylib/src all PLATFORM=PLATFORM_DESKTOP
endif

static-raylib:
	git clone https://github.com/raysan5/raylib.git
	@cp config.h raylib/src

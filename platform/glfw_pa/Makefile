#
# meg4/platform/glfw_pa/Makefile
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
# @brief The GLFW + portaudio Makefile
#

BACKEND=glfw

# MacOS
ifneq ("$(wildcard /Library/Frameworks/GLFW*)","")
 PACKAGE = MacOS
 CFLAGS = -I/Library/Frameworks/GLFW.framework/Headers -I/Library/Frameworks/GLFW.framework/Versions/A/Headers -D_MACOS_=1 -x objective-c -fobjc-exceptions
 LIBS = -F/Library/Frameworks -framework GLFW -framework GL -framework OpenAL -framework AppKit
else
 # Windows MinGW
 ifneq ("$(wildcard /bin/*.exe)","")
  PACKAGE = Win
  LDFLAGS += -mwindows -static-libgcc
  LIBS += -Wl,--nxcompat -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
  ifneq ("$(wildcard portaudio/*)","")
   CFLAGS += -I./portaudio/include
   EXTRA += ./portaudio/lib/.libs/libportaudio.a
  else
   LIBS += -lportaudio
  endif
  ifneq ("$(wildcard glfw3/*)","")
   CFLAGS += -I./glfw3/include
   EXTRA += ./glfw3/build/src/libglfw3.a
  else
   LIBS += -lglfw
  endif
  LIBS += -lGL -luser32 -lcomdlg32 -lshlwapi -lws2_32
 else
  # Linux
  PACKAGE = Linux
  ifneq ("$(wildcard portaudio/*)","")
   CFLAGS += -I./portaudio/include
   EXTRA += ./portaudio/lib/.libs/libportaudio.a
  else
   LIBS += -lportaudio
  endif
  ifneq ("$(wildcard glfw3/*)","")
   CFLAGS += -I./glfw3/include
   EXTRA += ./glfw3/build/src/libglfw3.a
  else
   LIBS += -lglfw
  endif
  LIBS += -lGL -lm -ldl -lpthread
 endif
endif
ifneq ($(NOGLES),)
 CFLAGS += -DNOGLES=1
endif
ifneq ($(JOYFALLBACK),)
 CFLAGS += -DJOYFALLBACK=1
endif

ifneq ("$(wildcard portaudio/*)","")
portaudio/lib/.libs/libportaudio.a:
ifeq ($(wildcard portaudio/configure),)
	@cd portaudio && ./autogen.sh
endif
ifeq ($(wildcard portaudio/Makefile),)
	@cd portaudio && ./configure --disable-shared --enable-static
endif
	@make --no-print-directory -s -C portaudio all
endif

ifneq ("$(wildcard glfw3/*)","")
glfw3/build/src/libglfw3.a:
ifeq ($(wildcard glfw3/build/Makefile),)
	@cd glfw3 && cmake -S . -B build -D GLFW_BUILD_EXAMPLES=OFF -D GLFW_BUILD_TESTS=OFF -D GLFW_BUILD_DOCS=0FF
endif
	@make --no-print-directory -s -C glfw3/build all
endif

include ../common.mk

static-glfw3:
	git clone https://github.com/glfw/glfw.git
	mv glfw glfw3

static-pa:
	git clone https://github.com/PortAudio/portaudio.git

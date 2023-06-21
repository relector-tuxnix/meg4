#
# meg4/platform/sdl/Makefile
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
# @brief The SDL Makefile
#

#USE_MINGWSDL = ../../SDL2-mingw
BACKEND=sdl

# Emscripten
ifneq ($(USE_EMCC),)
 PACKAGE = emscripten
 CFLAGS = -s USE_SDL=2 -I/usr/include/SDL2 -I/usr/include/SDL -Wno-unknown-warning-option
# wasm crashes in browser if compiled with -g
 DEBUG=
else
 # MacOS
 ifneq ("$(wildcard /Library/Frameworks/SDL*)","")
  PACKAGE = MacOS
  CFLAGS = -I/Library/Frameworks/SDL2.framework/Headers -I/Library/Frameworks/SDL2.framework/Versions/A/Headers -D_MACOS_=1 -x objective-c -fobjc-exceptions
  LIBS = -F/Library/Frameworks -framework SDL2 -framework AppKit
 else
  # Windows MinGW
  ifneq ("$(wildcard /bin/*.exe)","")
   PACKAGE = Win
   LDFLAGS += -mwindows -static-libgcc
   LIBS += -Wl,--nxcompat -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive -luser32 -lcomdlg32 -lole32 -lshlwapi \
    -lws2_32 -loleaut32 -lwinmm -limm32 -lsetupapi -luuid -lversion -lm
   ifneq ("$(wildcard $(USE_MINGWSDL)/build/.libs/libSDL2.a)","")
    CFLAGS += -I$(USE_MINGWSDL)/include
    EXTRA += $(USE_MINGWSDL)/build/.libs/libSDL2.a
   else
    ifneq ("$(wildcard SDL3/*)","")
     CFLAGS += -I./SDL3/include/SDL3 -I./SDL3/include
     EXTRA += ./SDL3/build/libSDL3.a
    else
     ifneq ("$(wildcard SDL2/*)","")
      CFLAGS += -I./SDL2/include
      EXTRA += ./SDL2/build/.libs/libSDL2.a
     else
      CFLAGS = -I/usr/include/SDL2
      LIBS += -lSDL2
     endif
    endif
   endif
  else
   # Linux
   PACKAGE = Linux
   ifneq ($(USE_DYNSDL),)
     CFLAGS = -I/usr/include/SDL2
     LIBS += -lSDL2
   else
    ifneq ("$(wildcard SDL3/*)","")
     CFLAGS += -I./SDL3/include/SDL3 -I./SDL3/include
     EXTRA += ./SDL3/build/libSDL3.a
    else
     ifneq ("$(wildcard SDL2/*)","")
      CFLAGS += -I./SDL2/include
      EXTRA += ./SDL2/build/.libs/libSDL2.a
     else
      CFLAGS = -I/usr/include/SDL2
      LIBS += -lSDL2
     endif
    endif
   endif
   LIBS += -lm -ldl -lpthread
  endif
 endif
endif
ifneq ($(FINGEREVENTS),)
 CFLAGS += -DFINGEREVENTS=1
endif
ifneq ($(NOEDITORS),)
 CFLAGS += -DNOEDITORS=1
endif
ifneq ($(EMBED),)
 CFLAGS += -DEMBED=1
endif
EXTRA += data.o

include ../common.mk

ifneq ("$(wildcard SDL2/*)","")
SDL2/build/.libs/libSDL2.a:
ifeq ($(wildcard SDL2/configure),)
	@cd SDL2 && ./autogen.sh
endif
ifeq ($(wildcard SDL2/Makefile),)
	@#dbus call in SDL2/src/core/linux/SDL_dbus.c:140 is leaking pretty badly. We don't need dbus anyway
	@#can't compile SDL2 with the latest wayland, the wl tools are buggy. Remove --disable-video-wayland once wl gets fixed
	@cd SDL2 && ./configure --disable-shared --enable-static --disable-dbus --disable-video-wayland
endif
	@make --no-print-directory -s -C SDL2 all
endif

ifneq ("$(wildcard SDL3/*)","")
SDL3/build/libSDL3.a:
ifeq ($(wildcard SDL3/build/Makefile),)
	@cd SDL3 && mkdir build && cd build && cmake -S .. -B ../build && cmake . -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=false -DSDL_STATIC=true -DSDL_DBUS=false -DSDL_IBUS=false
endif
	@make --no-print-directory -s -C SDL3/build all
endif

gamecontrollerdb.txt:
	curl -L "https://github.com/gabomdq/SDL_GameControllerDB/raw/master/gamecontrollerdb.txt" --output gamecontrollerdb.txt

data.c: gamecontrollerdb.txt
ifneq ($(USE_EMCC),)
	@cc ../../src/bin2h.c -o bin2h
else
	@$(CC) ../../src/bin2h.c -o bin2h
endif
	@cat gamecontrollerdb.txt | grep "platform:" | gzip >gamecontrollerdb
	./bin2h gamecontrollerdb
	@rm bin2h bin2h.exe gamecontrollerdb 2>/dev/null || true

data.o: data.c

static-sdl2:
	git clone --branch SDL2 https://github.com/libsdl-org/SDL.git
	mv SDL SDL2
	printf "#ifndef SDL_dynapi_h_\n#define SDL_dynapi_h_\n#define SDL_DYNAMIC_API 0\n#endif" >SDL2/src/dynapi/SDL_dynapi.h

static-sdl3:
	git clone https://github.com/libsdl-org/SDL.git
	mv SDL SDL3

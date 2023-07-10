#
# meg4/platform/allegro/Makefile
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
# @brief The allegro Makefile
#

BACKEND=allegro

# MacOS
ifneq ("$(wildcard /Library/Frameworks/allegro*)","")
 PACKAGE = MacOS
 CFLAGS = -I/Library/Frameworks/allegro.framework/Headers -I/Library/Frameworks/allegro.framework/Versions/A/Headers -D_MACOS_=1 -x objective-c -fobjc-exceptions
 LIBS = -F/Library/Frameworks -framework allegro -framework allegro_audio -framework allegro_main -framework GL -framework OpenAL -framework AppKit
else
 # Windows MinGW
 ifneq ("$(wildcard /bin/*.exe)","")
  PACKAGE = Win
  LDFLAGS += -mwindows -static-libgcc
  LIBS += -Wl,--nxcompat -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
  ifneq ("$(wildcard allegro5/*)","")
   CFLAGS += -I./allegro5/include -I./allegro5/build/include -I./allegro5/addons/audio
   EXTRA += ./allegro5/build/lib/liballegro-static.a ./allegro5/build/lib/liballegro_audio-static.a ./allegro5/build/lib/liballegro_main-static.a
   LIBS += -lGL -lole32 -lopenal
  else
   LIBS += -lallegro -lallegro_audio
  endif
  LIBS += -luser32 -lcomdlg32 -lshlwapi -lws2_32
 else
  # Linux
  PACKAGE = Linux
  ifneq ("$(wildcard allegro5/*)","")
   CFLAGS += -I./allegro5/include -I./allegro5/build/include -I./allegro5/addons/audio
   EXTRA += ./allegro5/build/lib/liballegro-static.a ./allegro5/build/lib/liballegro_audio-static.a ./allegro5/build/lib/liballegro_main-static.a
   LIBS += -lX11 -lXinerama -lXcursor -lXrandr -lXss -lXi -lGL -lasound -lopenal
  else
   LIBS += -lallegro -lallegro_audio
  endif
  LIBS += -lm -lpthread
 endif
endif
CFLAGS+=-Wno-long-long

include ../common.mk

ifneq ("$(wildcard allegro5/*)","")
./allegro5/build/lib/liballegro-static.a:
ifeq ("$(wildcard allegro5/build/Makefile)","")
	cd allegro5 && mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DSHARED=OFF -DPREFER_STATIC_DEPS=ON \
  -DWANT_FONT=OFF -DWANT_AUDIO=ON -DWANT_FLAC=OFF -DWANT_VORBIS=OFF -DWANT_TREMOR=OFF -DWANT_OPUS=OFF -DWANT_MODAUDIO=OFF \
  -DWANT_ACODEC_DYNAMIC_LOAD=OFF -DWANT_MP3=OFF -DWANT_PULSEAUDIO=OFF -DWANT_IMAGE=OFF -DWANT_IMAGE_JPG=OFF -DWANT_IMAGE_PNG=OFF \
  -DWANT_IMAGE_FREEIMAGE=OFF -DWANT_IMAGE_WEBP=OFF -DWANT_TTF=OFF -DWANT_COLOR=OFF -DWANT_MEMFILE=OFF -DWANT_PHYSFS=OFF \
  -DWANT_PRIMITIVES=OFF -DWANT_NATIVE_DIALOG=OFF -DWANT_VIDEO=OFF -DWANT_PYTHON_WRAPPER=OFF -DWANT_LUAJIT=OFF -DWANT_DOCS=OFF \
  -DWANT_DOCS_HTML=OFF -DWANT_DOCS_MAN=OFF -DWANT_DOCS_INFO=OFF -DWANT_DOCS_PDF=OFF -DWANT_DOCS_PDF_PAPER=OFF -DWANT_RELEASE_LOGGING=OFF \
  -DWANT_DLL_TLS=OFF -DWANT_DEMO=OFF -DWANT_EXAMPLES=OFF -DWANT_POPUP_EXAMPLES=OFF -DWANT_TESTS=OFF -DWANT_WAIT_EVENT_SLEEP=OFF
endif
	@make --no-print-directory -s -C allegro5/build all
endif

static-allegro5:
	git clone https://github.com/liballeg/allegro5

#
# meg4/platform/sokol/Makefile
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
# @brief The sokol Makefile
#

BACKEND=sokol

# MacOS
ifneq ("$(wildcard /Library/Frameworks/*)","")
 PACKAGE = MacOS
 CFLAGS = -I. -D_MACOS_=1 -x objective-c -fobjc-exceptions
 LIBS = -F/Library/Frameworks -framework QuartzCore -framework Cocoa -framework MetalKit -framework Metal -framework AppKit -framework AudioToolbox -framework AVFoundation
else
 # Windows MinGW
 ifneq ("$(wildcard /bin/*.exe)","")
  PACKAGE = Win
  LDFLAGS += -mwindows -static-libgcc
  LIBS += -Wl,--nxcompat -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
  CFLAGS += -I.
  LIBS += -lGL -luser32 -lcomdlg32 -lshlwapi -lws2_32 -lole32
 else
  # Linux
  PACKAGE = Linux
  CFLAGS += -I.
  LIBS += -lasound -lX11 -lXi -lXcursor -lGL -lm -ldl -lpthread
 endif
endif

USE_C99=1
include ../common.mk

sokol:
	wget https://raw.githubusercontent.com/floooh/sokol/master/sokol_app.h -O sokol_app.h
	wget https://raw.githubusercontent.com/floooh/sokol/master/sokol_audio.h -O sokol_audio.h
	wget https://raw.githubusercontent.com/floooh/sokol/master/sokol_gfx.h -O sokol_gfx.h
	wget https://raw.githubusercontent.com/floooh/sokol/master/sokol_glue.h -O sokol_glue.h
	wget https://raw.githubusercontent.com/floooh/sokol/master/util/sokol_gl.h -O sokol_gl.h

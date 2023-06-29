#
# meg4/platform/common.mk
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
# @brief The common Makefile for all platforms
#

# --- set these according to your configuration ---
#DEBUG = 1
# -------------------------------------------------

TARGET ?= meg4
SRCS = $(filter-out data.c,$(wildcard *.c))
OBJS = $(SRCS:.c=.o)

PREFIX ?= usr/
INSTDIR=$(DESTDIR:/=)/$(PREFIX:/=)
ARCH = $(shell uname -m)
TMP = $(ARCH:x86_64=amd64)
TMP2 = $(TMP:aarch64=arm64)
DEBARCH = $(TMP2:armv7l=armhf)
VERSION = $(shell cat ../../src/meg4.c|grep meg4ver|head -1|cut -d '"' -f 2)

ifeq ($(USE_EMCC)$(USE_C99),)
 CFLAGS += -ansi -pedantic
endif
ifneq ($(USE_C99),)
 CFLAGS += --std=c99
endif
CFLAGS += -Wall -Wextra -Wno-pragmas -ffunction-sections -I../../src -DBUILD="$(shell date +"%Y%j")"
ifneq ($(NOEDITORS),)
 CFLAGS += -DNOEDITORS=1
 DEBUG =
endif
ifeq ($(DEBUG),)
 CFLAGS += -O3
else
 CFLAGS += -g -DDEBUG=1
endif
ifneq ($(EMBED),)
 CFLAGS += -DEMBED=1
endif
ifeq ("$(PACKAGE)","Win")
 OBJS += resource.o
endif

all: libmeg4 $(TARGET)

libmeg4:
ifeq ($(USE_EMCC),)
	@make -C ../../src todo all DEBUG=$(DEBUG) EMBED=$(EMBED) NOEDITORS=$(NOEDITORS) NOLUA=$(NOLUA)
else
	CC=emcc USE_EMCC=1 make -C ../../src all DEBUG=$(DEBUG) EMBED=$(EMBED) NOEDITORS=$(NOEDITORS) NOLUA=$(NOLUA)
endif

resource.o:
	windres -i ../../src/misc/resource.rc -o resource.o

%: %.c
	$(CC) $(CFLAGS) $< -c $@

$(TARGET): ../../src/libmeg4.a $(EXTRA) $(OBJS)
ifeq ($(USE_EMCC),)
	$(CC) $(LDFLAGS) $(OBJS) ../../src/libmeg4.a -o $(TARGET) $(EXTRA) $(LIBS) -Wl,--gc-sections
ifeq ($(DEBUG),)
ifeq ("$(PACKAGE)","Win")
	@strip $(TARGET).exe
else
	@strip $(TARGET)
endif
endif
endif

install: $(TARGET)
ifneq ("$(INSTDIR)","")
	@strip $(TARGET)
	install -m 755 -g bin $(TARGET) $(INSTDIR)/bin
	@mkdir -p $(INSTDIR)/share/man/man1 $(INSTDIR)/share/applications $(INSTDIR)/share/icons/hicolor/16x16/apps $(INSTDIR)/share/icons/hicolor/64x64/apps $(INSTDIR)/share/icons/hicolor/128x128/apps 2>/dev/null || true
	cp ../../src/misc/meg4.1.gz $(INSTDIR)/share/man/man1
	cp ../../src/misc/meg4.desktop $(INSTDIR)/share/applications
	cp ../../src/misc/logo16.png $(INSTDIR)/share/icons/hicolor/16x16/apps/meg4.png
	cp ../../src/misc/logo32.png $(INSTDIR)/share/icons/hicolor/32x32/apps/meg4.png
	cp ../../src/misc/logo64.png $(INSTDIR)/share/icons/hicolor/64x64/apps/meg4.png
	cp ../../src/misc/logo128.png $(INSTDIR)/share/icons/hicolor/128x128/apps/meg4.png
else
	@echo "INSTDIR variable not set, not installing."
	@false
endif

package:
ifeq ("$(TARGET)","meg4.js")
	@rm ../meg4-wasm-emscripten.zip 2>/dev/null || true
	zip -r ../../meg4-wasm-emscripten.zip meg4.js meg4.wasm
else
ifeq ("$(PACKAGE)","Linux")
	@strip $(TARGET)
	@mkdir -p bin share/man/man1 share/applications share/icons/hicolor/16x16/apps share/icons/hicolor/32x32/apps share/icons/hicolor/64x64/apps share/icons/hicolor/128x128/apps
	@cp $(TARGET) bin
	@cp ../../src/misc/meg4.1.gz share/man/man1
	@cp ../../src/misc/meg4.desktop share/applications
	@cp ../../src/misc/logo16.png share/icons/hicolor/16x16/apps/meg4.png
	@cp ../../src/misc/logo32.png share/icons/hicolor/32x32/apps/meg4.png
	@cp ../../src/misc/logo64.png share/icons/hicolor/64x64/apps/meg4.png
	@cp ../../src/misc/logo128.png share/icons/hicolor/128x128/apps/meg4.png
	tar -czvf ../../$(TARGET)-$(ARCH)-linux-$(BACKEND).tgz bin share
	@rm -rf bin share
else
ifeq ("$(PACKAGE)","Win")
	@strip $(TARGET).exe
	@mkdir MEG-4
	@cp $(TARGET).exe MEG-4/$(TARGET).exe
	@rm ../../$(TARGET)-i686-win-$(BACKEND).zip 2>/dev/null || true
	zip -r ../../$(TARGET)-i686-win-$(BACKEND).zip MEG-4
	@rm -rf MEG-4
else
ifeq ("$(PACKAGE)","MacOS")
	@strip $(TARGET)
	@mkdir MEG-4.app MEG-4.app/Contents MEG-4.app/Contents/MacOS MEG-4.app/Contents/Resources
	@cp $(TARGET) MEG-4.app/Contents/MacOS
	@cp ../../src/misc/meg4.icns MEG-4.app/Contents/Resources
	@rm ../../$(TARGET)-intel-macos-$(BACKEND).zip
	zip -r ../../$(TARGET)-intel-macos-$(BACKEND).zip MEG-4.app
	@rm -rf MEG-4.app
else
endif
endif
endif
endif

deb:
ifeq ("$(PACKAGE)$(BACKEND)","Linuxsdl")
	@strip $(TARGET)
	@rm -rf DEBIAN usr 2>/dev/null || true
	@mkdir -p DEBIAN usr/bin usr/share/applications usr/share/man/man1 usr/share/icons/hicolor/16x16/apps usr/share/icons/hicolor/32x32/apps usr/share/icons/hicolor/64x64/apps usr/share/icons/hicolor/128x128/apps
	@cp $(TARGET) usr/bin
	@cp ../../src/misc/meg4.1.gz usr/share/man/man1
	@cp ../../src/misc/meg4.desktop usr/share/applications
	@cp ../../src/misc/logo16.png usr/share/icons/hicolor/16x16/apps/meg4.png
	@cp ../../src/misc/logo32.png usr/share/icons/hicolor/32x32/apps/meg4.png
	@cp ../../src/misc/logo64.png usr/share/icons/hicolor/64x64/apps/meg4.png
	@cp ../../src/misc/logo128.png usr/share/icons/hicolor/128x128/apps/meg4.png
	@cat ../../src/misc/deb_control | sed s/ARCH/$(DEBARCH)/g | sed s/VERSION/$(VERSION)/g | sed s/SIZE/`du -s usr|cut -f 1`/g >DEBIAN/control
	@md5sum `find usr -type f` >DEBIAN/md5sums
	@cp ../../LICENSE DEBIAN/copyright
	@echo "2.0" >debian-binary
	@tar -czvf data.tar.gz usr
	@tar -C DEBIAN -czvf control.tar.gz control copyright md5sums
	ar r ../../meg4_$(VERSION)-$(DEBARCH).deb debian-binary control.tar.gz data.tar.gz
	@rm -rf debian-binary control.tar.gz data.tar.gz DEBIAN usr
else
	@echo "Only available under Linux with the SDL2 backend."
	@false
endif

clean:
	@rm $(TARGET) meg4.* *.o data.c data.h 2>/dev/null || true

distclean: clean
	@make --no-print-directory -C ../../src distclean 2>/dev/null || true

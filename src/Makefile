#
# meg4/Makefile
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
# @brief The main Makefile
#

TARGET = libmeg4.a
SRCS = $(filter-out bin2h.c data.c,$(wildcard *.c))
OBJS = $(SRCS:.c=.o)
ASSETS = misc/icons.png misc/palette.png misc/default.sfn

CFLAGS += -Wall -Wextra -Wno-pragmas -Wno-long-long -ansi -pedantic -ffunction-sections
ifeq ($(NOEDITORS),)
 ESRCS = $(wildcard editors/*.c)
 EDITORS = $(ESRCS:.c=.o)
 ASSETS += misc/floppy.png misc/editors.png misc/sounds.mod misc/theme.gpl misc/hl.json misc/memmap.md $(wildcard lang/*.md)
else
 CFLAGS += -DNOEDITORS=1
endif
ifeq ($(DEBUG),)
 CFLAGS += -O3
else
 CFLAGS += -g -DDEBUG=1
endif
ifneq ($(EMBED),)
 CFLAGS += -DEMBED=1
endif
ifeq ($(NOLUA),)
ifneq ($(wildcard lua/lvm.c),)
CFLAGS += -DLUA=1
LUA=lua/lvm.o
LUAO=lua/*.o
endif
endif

all: data.c data.o objs $(TARGET)

ifneq ($(wildcard editors/pro.c),)
PRO = -DMEG4_PRO=1
endif

data.c: $(ASSETS)
ifneq ($(USE_EMCC),)
	@cc bin2h.c -o bin2h
else
	@$(CC) bin2h.c -o bin2h
endif
ifeq ($(NOEDITORS),)
	./bin2h $(ASSETS)
else
	./bin2h $(ASSETS) game
endif
	@rm bin2h bin2h.exe 2>/dev/null || true

objs: $(OBJS)
ifeq ($(NOEDITORS),)
	@make -C editors all DEBUG=$(DEBUG) CFLAGS="$(CFLAGS) $(PRO)"
endif

todo:
	@grep -n -e 'TODO:' -e 'FIXME:' *.c *.h editors/*.c > ../TODO.txt || true

lua/lvm.o:
	@make --no-print-directory -C lua o CC="$(CC)" 2>/dev/null || true

%: %.c
	$(CC) $(CFLAGS) $< -c $@

$(TARGET): $(OBJS) $(EDITORS) $(LUA)
	@rm $@ 2>/dev/null || true
	@ar -frs $@ $(LUAO) $(EDITORS) $(OBJS) data.o

clean:
	@make --no-print-directory -C editors/mbedtls clean 2>/dev/null || true
	@make --no-print-directory -C editors clean 2>/dev/null || true
	@rm $(TARGET) data.c data.h *.o 2>/dev/null || true

distclean: clean
ifneq ($(wildcard lua/lvm.c),)
	@make --no-print-directory -C lua clean 2>/dev/null || true
endif

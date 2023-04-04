#
# meg4/platform/emscripten/Makefile
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
# @brief The emscripten Makefile
#

TARGET = meg4.js
USE_EMCC = 1

ifneq ($(NOEDITORS),)
LINK = -g0 -O3
else
FUNC=,"_meg4_insert"
EDS=../../src/editors/*.o
endif
ifeq ($(NOLUA),)
ifneq ($(wildcard ../../src/lua/lvm.c),)
LUAO=../../src/lua/*.o
endif
endif

include ../common.mk

meg4.js:
	@make --no-print-directory -C ../sdl clean
	CC=emcc USE_EMCC=1 make -C ../sdl all DEBUG=$(DEBUG) NOEDITORS=$(NOEDITORS)
	emcc -s USE_SDL=2 -s EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 -s WASM_BIGINT -s EXPORTED_FUNCTIONS='["_main"$(FUNC),"_malloc","_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' $(LINK) --pre-js=pre.js \
	../sdl/*.o ../../src/*.o $(EDS) $(LUAO) -o meg4.js
	@make --no-print-directory -C ../sdl clean
ifeq ($(NOEDITORS),)
	cp meg4.js meg4.wasm ../../pages
else
ifneq ($(wildcard ../../src/editors/pro.c),)
	@cc ../../src/bin2h.c -o bin2h
	./bin2h meg4.js meg4.wasm
	mv data.c ../../src/editors/pro.h
	@rm bin2h bin2h.exe data.h 2>/dev/null || true
endif
endif

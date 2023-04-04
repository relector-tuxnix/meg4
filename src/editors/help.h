/*
 * meg4/editors/help.h
 *
 * Copyright (C) 2023 bzt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @brief Built-in help system defines and functions
 *
 */

#include "../misc/emoji.h"

typedef struct {
    int x, y, w;
    int page;
    char *link;
} link_t;

typedef struct {
    char *content;
    char *chapter;
    char *code;
    link_t *link;
    int numlink, chapw, titw, y, h;
} help_t;
extern help_t *help_pages;
extern int help_numpages, help_current;

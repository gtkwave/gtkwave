/*
 * Copyright (c) Tony Bybell 2010-2017
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_HIERSEARCH_H
#define WAVE_HIERSEARCH_H

void hier_searchbox(char *title, GtkSignalFunc func);
void refresh_hier_tree(struct tree *t);
int hier_searchbox_is_active(void);

void recurse_fetch_high_low(struct tree *t);

struct tree *fetchhigh(struct tree *t);
struct tree *fetchlow(struct tree *t);
void fetchvex(struct tree *t, char direction);

#endif


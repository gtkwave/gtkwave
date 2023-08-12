/*
 * Copyright (c) Tony Bybell 2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_SIMPLEREQ_H
#define WAVE_SIMPLEREQ_H

void simplereqbox(const char *title, int width, const char *default_text, const char *oktext, const char *canceltext, GCallback func, int is_alert);

#endif

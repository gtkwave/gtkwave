/*
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "fgetdynamic.h"
#include <stdlib.h>

int fgetmalloc_len;

char *fgetmalloc(FILE *handle)
{
    GString *line = g_string_new(NULL);

    for (;;) {
        int ch = fgetc(handle);
        if (ch == EOF || ch == 0x00 || ch == '\n' || ch == '\r') {
            if (ch > 0 && line->len == 0) {
                continue; // skip leading CRs and LFs
            } else {
                break;
            }
        }

        g_string_append_c(line, ch);
    }

    fgetmalloc_len = line->len;

    char *ret = NULL;
    if (fgetmalloc_len > 0) {
        ret = malloc(fgetmalloc_len + 1);
        memcpy(ret, line->str, fgetmalloc_len + 1);
    }

    g_string_free(line, TRUE);

    return ret;
}
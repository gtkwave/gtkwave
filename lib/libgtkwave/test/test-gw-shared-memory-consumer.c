/*
 * Copyright (c) 2024 GTKWave Contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gw-shared-memory.h"
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define RING_BUFFER_SIZE (1024 * 1024)
#define CONSUMER_TIMEOUT_MS 2000 // 2 seconds
#define SLEEP_INTERVAL_US 10000  // 10ms

static guint8 get_8(guint8 *base, gssize offset) {
    return base[offset % RING_BUFFER_SIZE];
}

static guint32 get_32(guint8 *base, gssize offset) {
    guint32 val = 0;
    val |= (get_8(base, offset)     << 24);
    val |= (get_8(base, offset + 1) << 16);
    val |= (get_8(base, offset + 2) << 8);
    val |= (get_8(base, offset + 3));
    return val;
}

int main(void)
{
    char shm_id_str[256];
    GwSharedMemory *shm = NULL;
    guint8 *shm_data = NULL;
    gssize consume_offset = 0;
    int timeout_counter = 0;
#ifdef _WIN32
    /* Ensure stdout is in binary mode on Windows so tests compare byte-for-byte */
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    if (!fgets(shm_id_str, sizeof(shm_id_str), stdin)) {
        fprintf(stderr, "Consumer Error: Failed to read SHM ID from stdin.\n");
        return 1;
    }
    shm_id_str[strcspn(shm_id_str, "\r\n")] = 0;

    shm = gw_shared_memory_open(shm_id_str, NULL);
    if (!shm) {
        fprintf(stderr, "Consumer Error: Could not open SHM segment '%s'.\n", shm_id_str);
        return 1;
    }
    shm_data = (guint8 *)gw_shared_memory_get_data(shm);

    while (timeout_counter * SLEEP_INTERVAL_US < CONSUMER_TIMEOUT_MS * 1000) {
        if (get_8(shm_data, consume_offset) != 0) {
            guint32 len = get_32(shm_data, consume_offset + 1);

            for (guint32 i = 0; i < len; i++) {
                putchar(get_8(shm_data, consume_offset + 5 + i));
            }
            fflush(stdout);

            shm_data[consume_offset % RING_BUFFER_SIZE] = 0;
            consume_offset += (5 + len);
            timeout_counter = 0; // Reset timeout on activity
        } else {
            usleep(SLEEP_INTERVAL_US);
            timeout_counter++;
        }
    }

    gw_shared_memory_free(shm);
    return 0; // Success
}
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

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * GwSharedMemory:
 *
 * Opaque structure representing a shared memory segment.
 */
typedef struct _GwSharedMemory GwSharedMemory;

/**
 * gw_shared_memory_create:
 * @size: The size of the shared memory segment to create
 * @error: (nullable): Return location for a GError, or %NULL
 *
 * Creates a new shared memory segment of the specified size.
 *
 * Returns: (transfer full): A new #GwSharedMemory instance, or %NULL on error
 */
GwSharedMemory *gw_shared_memory_create(gsize size, GError **error);

/**
 * gw_shared_memory_open:
 * @id: The shared memory identifier as a string
 * @error: (nullable): Return location for a GError, or %NULL
 *
 * Opens an existing shared memory segment using the provided identifier.
 *
 * Returns: (transfer full): A #GwSharedMemory instance, or %NULL on error
 */
GwSharedMemory *gw_shared_memory_open(const gchar *id, GError **error);

/**
 * gw_shared_memory_get_id:
 * @shm: A #GwSharedMemory instance
 *
 * Gets the identifier string for the shared memory segment.
 *
 * Returns: (transfer none): The shared memory identifier string
 */
const gchar *gw_shared_memory_get_id(GwSharedMemory *shm);

/**
 * gw_shared_memory_get_data:
 * @shm: A #GwSharedMemory instance
 *
 * Gets a pointer to the shared memory data.
 *
 * Returns: (transfer none): Pointer to the shared memory data
 */
gpointer gw_shared_memory_get_data(GwSharedMemory *shm);

/**
 * gw_shared_memory_get_size:
 * @shm: A #GwSharedMemory instance
 *
 * Gets the size of the shared memory segment.
 *
 * Returns: The size of the shared memory segment in bytes
 */
gsize gw_shared_memory_get_size(GwSharedMemory *shm);

/**
 * gw_shared_memory_free:
 * @shm: (transfer full): A #GwSharedMemory instance
 *
 * Frees a shared memory segment and releases associated resources.
 */
void gw_shared_memory_free(GwSharedMemory *shm);

G_END_DECLS
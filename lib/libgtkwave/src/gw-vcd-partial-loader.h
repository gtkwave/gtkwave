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
 * IMPLIED, INCLUDING BUT NOT LIMITED to the WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "gw-vcd-loader.h"
#include "gw-shared-memory.h"

G_BEGIN_DECLS

#define GW_TYPE_VCD_PARTIAL_LOADER (gw_vcd_partial_loader_get_type())
G_DECLARE_FINAL_TYPE(GwVcdPartialLoader, gw_vcd_partial_loader, GW, VCD_PARTIAL_LOADER, GwVcdLoader)

/**
 * gw_vcd_partial_loader_set_shm_id:
 * @self: A #GwVcdPartialLoader instance
 * @shm_id: The shared memory identifier string
 *
 * Sets the shared memory ID for the partial loader to use.
 */
void gw_vcd_partial_loader_set_shm_id(GwVcdPartialLoader *self, const gchar *shm_id);

/**
 * gw_vcd_partial_loader_kick:
 * @self: A #GwVcdPartialLoader instance
 *
 * Processes available data from the shared memory segment.
 * This should be called periodically to update the waveform data.
 */
void gw_vcd_partial_loader_kick(GwVcdPartialLoader *self);

/**
 * gw_vcd_partial_loader_cleanup:
 * @self: A #GwVcdPartialLoader instance
 *
 * Cleans up resources associated with the partial loader.
 */
void gw_vcd_partial_loader_cleanup(GwVcdPartialLoader *self);

G_END_DECLS
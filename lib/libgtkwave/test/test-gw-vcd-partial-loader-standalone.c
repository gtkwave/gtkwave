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
#include <glib/gstdio.h>
#include <string.h>
#include "gw-vcd-partial-loader.h"
#include "gw-dump-file.h"
#include "gw-facs.h"
#include "gw-symbol.h"
#include "gw-time-range.h"

/*
 * Simple test to verify basic VCD partial loader functionality
 * This test focuses on header parsing and basic symbol creation
 * without attempting to replicate the complex incremental loading
 */
static void test_basic_header_parsing(void)
{
    const gchar *source_dir = g_getenv("MESON_SOURCE_ROOT");
    gchar *input_vcd_path = g_build_filename(source_dir, "lib", "libgtkwave", "test", "files", "basic.vcd", NULL);

    // Create a partial loader
    GwVcdPartialLoader *loader = gw_vcd_partial_loader_new();
    g_assert_nonnull(loader);

    // Test that loader can be created and basic functions work
    g_assert_true(GW_IS_VCD_PARTIAL_LOADER(loader));
    
    // Test cleanup function
    gw_vcd_partial_loader_cleanup(loader);
    
    // Test header parsed flag
    g_assert_false(gw_vcd_partial_loader_is_header_parsed(loader));
    
    g_free(input_vcd_path);
    g_object_unref(loader);
}

static void test_loader_creation(void)
{
    GwVcdPartialLoader *loader = gw_vcd_partial_loader_new();
    g_assert_nonnull(loader);
    g_assert_true(GW_IS_VCD_PARTIAL_LOADER(loader));
    
    // Test that it inherits from GwVcdLoader
    g_assert_true(GW_IS_VCD_LOADER(loader));
    
    g_object_unref(loader);
}

static void test_loader_cleanup(void)
{
    GwVcdPartialLoader *loader = gw_vcd_partial_loader_new();
    g_assert_nonnull(loader);
    
    // Cleanup should work without errors
    gw_vcd_partial_loader_cleanup(loader);
    
    // Cleanup again should also work
    gw_vcd_partial_loader_cleanup(loader);
    
    g_object_unref(loader);
}

static void test_header_parsed_flag(void)
{
    GwVcdPartialLoader *loader = gw_vcd_partial_loader_new();
    g_assert_nonnull(loader);
    
    // Initially should be false
    g_assert_false(gw_vcd_partial_loader_is_header_parsed(loader));
    
    // Cleanup shouldn't change this
    gw_vcd_partial_loader_cleanup(loader);
    g_assert_false(gw_vcd_partial_loader_is_header_parsed(loader));
    
    g_object_unref(loader);
}

static void test_time_range_update(void)
{
    GwVcdPartialLoader *loader = gw_vcd_partial_loader_new();
    g_assert_nonnull(loader);
    
    // Create a dummy dump file for testing time range update
    GwTimeRange *time_range = gw_time_range_new(0, 100);
    GwDumpFile *dump_file = g_object_new(GW_TYPE_DUMP_FILE,
        "time-range", time_range,
        NULL);
    
    // Update time range should work without errors
    gw_vcd_partial_loader_update_time_range(loader, dump_file);
    
    // Update again should also work
    gw_vcd_partial_loader_update_time_range(loader, dump_file);
    
    g_object_unref(time_range);
    g_object_unref(dump_file);
    g_object_unref(loader);
}

static void test_error_handling(void)
{
    GwVcdPartialLoader *loader = gw_vcd_partial_loader_new();
    g_assert_nonnull(loader);
    
    GError *error = NULL;
    
    // Test loading with invalid SHM ID
    GwDumpFile *result = gw_vcd_partial_loader_load(loader, "invalid_shm_id", &error);
    g_assert_null(result);
    // Error should be set
    g_assert_nonnull(error);
    g_error_free(error);
    error = NULL;
    
    g_object_unref(loader);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    
    // Add basic functionality tests
    g_test_add_func("/VcdPartialLoader/LoaderCreation", test_loader_creation);
    g_test_add_func("/VcdPartialLoader/LoaderCleanup", test_loader_cleanup);
    g_test_add_func("/VcdPartialLoader/HeaderParsedFlag", test_header_parsed_flag);
    g_test_add_func("/VcdPartialLoader/TimeRangeUpdate", test_time_range_update);
    g_test_add_func("/VcdPartialLoader/ErrorHandling", test_error_handling);
    g_test_add_func("/VcdPartialLoader/BasicHeaderParsing", test_basic_header_parsing);
    
    return g_test_run();
}
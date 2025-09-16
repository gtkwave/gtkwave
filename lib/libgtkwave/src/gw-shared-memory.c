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

#include "gw-shared-memory.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32) && !defined(__MINGW32__)
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#else
#include <windows.h>
#endif

#include <unistd.h>

struct _GwSharedMemory
{
    gchar *id;
    gpointer data;
    gsize size;
#if !defined(_WIN32) && !defined(__MINGW32__)
    gint shmid;
#else
    HANDLE hMapFile;
#endif
    gboolean is_owner;
};

GwSharedMemory *
gw_shared_memory_create(gsize size, GError **error)
{
    GwSharedMemory *shm = g_new0(GwSharedMemory, 1);
    shm->size = size;
    shm->is_owner = TRUE;

#if !defined(_WIN32) && !defined(__MINGW32__)
    /* POSIX shared memory */
    shm->shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
    if (shm->shmid < 0) {
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "Failed to create shared memory segment: %s",
                    g_strerror(errno));
        g_free(shm);
        return NULL;
    }

    shm->data = shmat(shm->shmid, NULL, 0);
    if (shm->data == (void *)-1) {
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "Failed to attach shared memory: %s",
                    g_strerror(errno));
        shmctl(shm->shmid, IPC_RMID, NULL);
        g_free(shm);
        return NULL;
    }

    /* Mark for automatic destruction when no processes are attached */
    shmctl(shm->shmid, IPC_RMID, NULL);

    /* Create ID string from shmid */
    shm->id = g_strdup_printf("%08X", shm->shmid);

#else
    /* Windows shared memory */
    gchar mapName[65];
    shm->id = g_strdup_printf("%d", getpid());
    g_snprintf(mapName, sizeof(mapName), "shmidcat%s", shm->id);

    shm->hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
                                      NULL,
                                      PAGE_READWRITE,
                                      0,
                                      size,
                                      mapName);
    if (shm->hMapFile == NULL) {
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "Failed to create shared memory mapping: error %lu",
                    GetLastError());
        g_free(shm->id);
        g_free(shm);
        return NULL;
    }

    shm->data = MapViewOfFile(shm->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (shm->data == NULL) {
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "Failed to map view of file: error %lu",
                    GetLastError());
        CloseHandle(shm->hMapFile);
        g_free(shm->id);
        g_free(shm);
        return NULL;
    }
#endif

    /* Initialize the shared memory to zeros */
    memset(shm->data, 0, size);

    return shm;
}

GwSharedMemory *
gw_shared_memory_open(const gchar *id, GError **error)
{
    GwSharedMemory *shm = g_new0(GwSharedMemory, 1);
    shm->is_owner = FALSE;
    shm->id = g_strdup(id);

#if !defined(_WIN32) && !defined(__MINGW32__)
    /* POSIX shared memory - convert hex string to integer */
    gint shmid;
    if (sscanf(id, "%X", &shmid) != 1) {
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "Invalid shared memory ID format: %s",
                    id);
        g_free(shm->id);
        g_free(shm);
        return NULL;
    }

    shm->shmid = shmid;
    shm->data = shmat(shm->shmid, NULL, 0);
    if (shm->data == (void *)-1) {
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "Failed to attach shared memory: %s",
                    g_strerror(errno));
        g_free(shm->id);
        g_free(shm);
        return NULL;
    }

    /* Get the actual size of the shared memory segment */
    struct shmid_ds ds;
    if (shmctl(shm->shmid, IPC_STAT, &ds) == 0) {
        shm->size = ds.shm_segsz;
    } else {
        /* Fallback to a reasonable size if we can't determine it */
        shm->size = 1024 * 1024; /* 1MB default */
    }

#else
    /* Windows shared memory */
    gchar mapName[65];
    g_snprintf(mapName, sizeof(mapName), "shmidcat%s", id);

    shm->hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mapName);
    if (shm->hMapFile == NULL) {
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "Failed to open shared memory mapping: error %lu",
                    GetLastError());
        g_free(shm->id);
        g_free(shm);
        return NULL;
    }

    shm->data = MapViewOfFile(shm->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (shm->data == NULL) {
        g_set_error(error,
                    G_FILE_ERROR,
                    G_FILE_ERROR_FAILED,
                    "Failed to map view of file: error %lu",
                    GetLastError());
        CloseHandle(shm->hMapFile);
        g_free(shm->id);
        g_free(shm);
        return NULL;
    }

    /* For Windows, we need to know the size from the creator */
    shm->size = 1024 * 1024; /* Default size, should match shmidcat */
#endif

    return shm;
}

const gchar *
gw_shared_memory_get_id(GwSharedMemory *shm)
{
    g_return_val_if_fail(shm != NULL, NULL);
    return shm->id;
}

gpointer
gw_shared_memory_get_data(GwSharedMemory *shm)
{
    g_return_val_if_fail(shm != NULL, NULL);
    return shm->data;
}

gsize
gw_shared_memory_get_size(GwSharedMemory *shm)
{
    g_return_val_if_fail(shm != NULL, 0);
    return shm->size;
}

void
gw_shared_memory_free(GwSharedMemory *shm)
{
    if (shm == NULL) {
        return;
    }

#if !defined(_WIN32) && !defined(__MINGW32__)
    if (shm->data != NULL && shm->data != (void *)-1) {
        shmdt(shm->data);
    }
    /* If we're the owner, the segment was already marked for destruction */
#else
    if (shm->data != NULL) {
        UnmapViewOfFile(shm->data);
    }
    if (shm->hMapFile != NULL) {
        CloseHandle(shm->hMapFile);
    }
#endif

    g_free(shm->id);
    g_free(shm);
}
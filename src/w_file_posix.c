//
// Copyright(C) 2026 Mrokkk
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  WAD I/O functions.
//

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include "i_system.h"
#include "w_file.h"
#include "z_zone.h"

typedef struct
{
    wad_file_t wad;
    int fd;
} posix_wad_file_t;

extern wad_file_class_t posix_wad_file;

static wad_file_t *W_Posix_OpenFile(char *path)
{
    posix_wad_file_t *result;

    int fd = open(path, O_RDONLY);

    if (fd == -1)
    {
        return NULL;
    }

    struct stat stat;

    if (fstat(fd, &stat) == -1)
    {
        return NULL;
    }

    void* mapped = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (mapped == MAP_FAILED)
    {
        return NULL;
    }

    result = Z_Malloc(sizeof(posix_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &posix_wad_file;
    result->wad.mapped = mapped;
    result->wad.length = stat.st_size;

    I_Printf("Mapped \"%s\" to memory at %p\n", path, mapped);

    return &result->wad;
}

static void W_Posix_CloseFile(wad_file_t *wad)
{
    posix_wad_file_t *posix_wad = (posix_wad_file_t *) wad;

    munmap(wad->mapped, wad->length);
    close(posix_wad->fd);
}

static size_t W_Posix_Read(wad_file_t *wad, unsigned int offset, void *buffer, size_t buffer_len)
{
    size_t to_read = MIN(wad->length - offset, buffer_len);
    memcpy(buffer, (byte*)wad->mapped + offset, to_read);
    return to_read;
}

wad_file_class_t posix_wad_file = {
    W_Posix_OpenFile,
    W_Posix_CloseFile,
    W_Posix_Read,
};

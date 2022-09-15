#ifndef HEADER_file_util
#define HEADER_file_util

#include <stdio.h>
#include "types.h"

int open_file(char* rel_path, int flags, mode_t mode);

int read_chunk(int fd, size_t beginning, size_t chunk_size, char* buffer);

size_t write_chunk(int fd, size_t chunk_size, char* buffer);


#endif
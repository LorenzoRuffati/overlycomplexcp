#ifndef HEADER_file_util
#define HEADER_file_util

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include "types.h"

int open_file(char* rel_path, int flags, mode_t mode);

size_t read_chunk(int fd, size_t beginning, size_t chunk_size, char* buffer, int seek_flag);

size_t write_chunk(int fd, size_t chunk_size, char* buffer);


#endif
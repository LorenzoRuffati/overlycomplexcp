#include "file_util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int open_file(char* rel_path, int flags, mode_t mode){
   /*
   Open the given (existing) file and return the file descriptor to be used
   in future function calls
   */
  int fd = open(rel_path, flags, mode);
  if (fd == -1){
   printf("File: < %s > can't be opened\n", rel_path);
   err_and_leave("", 3);
  }
  printf("%d\n", fd);
  return fd;
}

size_t read_chunk(int fd_o, size_t beginning, size_t chunk_size, char* buffer, int seek){
    /*
    Read a chunk of `chunk_size` bytes starting at `beginning` bytes in the
    file with file descriptor `fd` and write those bytes into `buffer`

    */
   int fd = dup(fd_o);
   FILE* fstream = fdopen(fd, "r");
   if (seek) {
      int sk_ret = fseek(fstream, beginning, SEEK_SET);
      if (sk_ret == -1){
      err_and_leave("Error seeking into file", 3);
      }
   }
   size_t n_read = fread(buffer, 1, chunk_size, fstream);
   fclose(fstream);
   return n_read;
}

size_t write_chunk(int fd_o, size_t chunk_size, char* buffer){
    /*
    Append the first `chunk_size` bytes of `buffer` to the file with
    file_descriptor `fd`
    */
   int fd = dup(fd_o);
   FILE* fstream = fdopen(fd, "a");
   //fseek(fstream, 0, SEEK_END);
   size_t bts_written = fwrite(buffer, 1, chunk_size, fstream);
   fclose(fstream);
   return bts_written; 
}
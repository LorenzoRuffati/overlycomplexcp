#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "pipe.h"
#include "../shared/file_util.h"

#define BUFFSZ 256

int use_pipe(setting_t settings){
    switch (settings.role) {
        case SENDER:
            return pipe_sender(settings);
            break;
        case RECEIVER:
            return pipe_receiver(settings);
            break;
        default:
            err_and_leave("Impossible role", 4);
    }
    return 0;
}

void create_fifo(char* path){
    int ret = mkfifo(path, 0666);
    if (ret == 0){
    } else if (ret == -1) {
        switch (errno)
        {
        case EEXIST:
            break;
        default:
            err_and_leave("Failed to create pipe", 4);
            break;
        }
    } else {
        err_and_leave("", 4);
    }
}

int pipe_sender(setting_t settings){
    int fd_in, fd_fifo;
    char* myfifo = "/tmp/myfifo";
    create_fifo(myfifo);
    fd_fifo = open_file(myfifo, O_WRONLY, 0);
    //printf("Opened\n");
    fd_in = open_file(settings.filename, O_RDONLY, 0);

    char buffer[BUFFSZ]; // 
    size_t read_position = 0;
    FILE* fifo_stream = fdopen(fd_fifo, "w");

    struct stat sb;
    if (stat(settings.filename, &sb) == -1) {
        err_and_leave("stat", 4);
    }
    size_t size = sb.st_size;
    while (1) {
        int n_read = read_chunk(fd_in, read_position, BUFFSZ, buffer, 1);
        int n_write = fwrite(buffer, 1, n_read, fifo_stream);
        if (!(n_write==n_read)){
            err_and_leave("Wrote different amount than read", 4);
        }

        read_position = read_position + n_read;
        //printf("Read: %.*s\n", n_read, buffer);
        if (read_position == size){
            break;
        }
    }
    fclose(fifo_stream);
    close(fd_in);
    return 0;
}

int pipe_receiver(setting_t settings){
    int fd_out, fd_fifo;
    char* myfifo = "/tmp/myfifo";
    create_fifo(myfifo);
    fd_fifo = open_file(myfifo, O_RDONLY, 0);
    //printf("Opened\n");
    fd_out = open_file(settings.filename, O_WRONLY | O_APPEND | O_CREAT, S_IWRITE | S_IREAD);
    char buffer[BUFFSZ];
    
    FILE* stream = fdopen(fd_fifo, "r");
    
    while (1) {
        int n_read = fread(buffer, 1, BUFFSZ, stream);
        if (n_read == 0) {
            break;
        }
        //printf("Read %d bytes\n", n_read);
        int n_write = write_chunk(fd_out, n_read, buffer);
        if (n_write != n_read){
            err_and_leave("Incompatible read and write", 4);
        }
        //printf("Wrote: %.*s\n", n_read, buffer);
        //sleep(3);
    }
    fclose(stream);
    close(fd_out);
    return 0;
}
/*
echo "hi pipes" > test_in.txt
builddir/ocp -f test_in.txt --pass 123 --method p -r s
builddir/ocp -f test_out.txt --pass 123 --method p -r r
cat test_out.txt
*/
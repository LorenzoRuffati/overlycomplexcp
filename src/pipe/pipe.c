#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "pipe.h"
#include "../shared/file_util.h"

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
}

int create_fifo(char* path){
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
    fd_in = open_file(settings.filename, O_RDONLY, 0);

    char buffer[256]; // 
    size_t read_position = 0;

    struct stat sb;
    if (stat(settings.filename, &sb) == -1) {
        err_and_leave("stat", 4);
    }
    size_t size = sb.st_size;

    while (1) {
        int n_read = read_chunk(fd_in, read_position, 256, buffer, 1);
        int n_write = write_chunk(fd_fifo, n_read, buffer);
        if (!(n_write==n_read)){
            err_and_leave("Wrote different amount than read", 4);
        }

        read_position = read_position + n_read;
        printf("Read: %.*s", n_read, buffer);
        if (read_position == size){
            break;
        } else {
            continue;
        }
    }
    return 1;
}

int pipe_receiver(setting_t settings){
    int fd_out, fd_fifo;
    char* myfifo = "/tmp/myfifo";
    create_fifo(myfifo);
    fd_fifo = open_file(myfifo, O_RDONLY, 0);
    fd_out = open_file(settings.filename, O_WRONLY | O_APPEND | O_CREAT, S_IWRITE | S_IREAD);
    char buffer[256];
    while (1) {
        int n_read = read_chunk(fd_fifo, 0, 256, buffer, 0);
        printf("Read %d bytes\n", n_read);
        int n_write = write_chunk(fd_out, n_read, buffer);
        printf("Wrote: %.*s\n", n_read, buffer);
        sleep(5);
    }
}
/*
echo "hi pipes" > test_in.txt
builddir/ocp -f test_in.txt --pass 123 --method p -r s
builddir/ocp -f test_out.txt --pass 123 --method p -r r
cat test_out.txt
*/
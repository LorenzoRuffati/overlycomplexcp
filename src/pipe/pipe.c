#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "pipe.h"
#include "../shared/file_util.h"

#define BUFFSZ 256

int use_pipe(setting_t settings){
    //printf("usp %s\n", settings.password);
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

char* fifo_name(char* passwd){
    //printf("ffn %s\n", passwd);
    char* myfifo = "/tmp/myfifo_";
    // We have passwd
    // We have myfifo
    // new_string
    // copy myfifo into new_string
    // concatenate passwd to new_string
    char* new_string = malloc(strlen(myfifo) + strlen(passwd) + 1);
    strcpy(new_string, myfifo);
    strcat(new_string, passwd);
    return new_string;
}

char* create_fifo(char* passwd){
    //printf("ffn %s\n", passwd);
    char* new_string = fifo_name(passwd);
    //printf("%s\n", new_string);
    int ret = mkfifo(new_string, 0666);
    if (ret == 0){
    } else if (ret == -1) {
        switch (errno)
        {
        case EEXIST:
            break;
        default:
            printf("%d\n", errno);
            err_and_leave("Failed to create pipe", 4);
            break;
        }
    } else {
        err_and_leave("", 4);
    }
    return new_string;
}

int pipe_sender(setting_t settings){
    int fd_in, fd_fifo;
    char* fifon = create_fifo(settings.password);
    fd_fifo = open_file(fifon, O_WRONLY, 0);
    //printf("Opened\n");
    fd_in = open_file(settings.filename, O_RDONLY, 0);

    char buffer[BUFFSZ]; // 
    size_t read_position = 0;
    FILE* fifo_stream = fdopen(fd_fifo, "w");
    if (fifo_stream == NULL){
       err_and_leave("Couldn't open file stream", 4);
    }

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
    char* fifon = create_fifo(settings.password);
    fd_fifo = open_file(fifon, O_RDONLY, 0);
    //printf("Opened\n");
    fd_out = open_file(settings.filename, O_WRONLY | O_APPEND | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);
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
    unlink(fifon);
    return 0;
}
/*
echo "hi pipes" > test_in.txt
builddir/ocp -f test_in.txt --pass 123 --method p -r s
builddir/ocp -f test_out.txt --pass 123 --method p -r r
cat test_out.txt
*/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h> 

int main() {
    printf("Hello, World!\n");

    int fd = shm_open("/test_shm", O_RDWR|O_CREAT|O_EXCL, 0644 );
    fprintf(stderr,"Shared Mem Descriptor: fd=%d, errno: %d\n", fd, errno);


    ftruncate(fd, 1024);
    assert (fd>0);

    char *ptr = (char *) mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    fprintf(stderr, "Shared Memory Address: %p [0..%lu]\n", ptr,(unsigned long) 1024-1);
    fprintf(stderr, "Shared Memory Path: /dev/shm/%s\n",  "test_shm");

    assert (ptr);
    shm_unlink("/test_shm");
    return 0;
}
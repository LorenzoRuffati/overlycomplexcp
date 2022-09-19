#ifndef HEADER_shared_memory
#define HEADER_shared_memory

#include <pthread.h>

#include "../shared/file_util.h"
#include "../shared/types.h"
#include "../shared/hash.h"

typedef struct {
    pthread_mutex_t lock; // TODO: be careful because mutexes and condition variables need to be explicitly enabled for inter-process
                            // synchronization
    size_t mmap_size;
    int readers;
    int start;
    sha_hexdigest hash; // Used both for integrity check and for coordination
} coord_struct;

typedef struct {
    pthread_rwlock_t setup;
    pthread_rwlock_t active[2];
    pthread_rwlock_t leaving[2];
    size_t width;
    char space[]; //contains two sections of size "width"
    
} copy_struct;

typedef struct {
    coord_struct* mem_region;
    int fd_shared;
} mmap_creat_ret_t;

int use_shared(setting_t settings);
int shared_sender(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info);

int shared_receiver(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info);
#endif
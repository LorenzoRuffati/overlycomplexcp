#ifndef HEADER_shared_memory
#define HEADER_shared_memory

#include <pthread.h>

#include "../shared/file_util.h"
#include "../shared/types.h"
#include "../shared/hash.h"
#include "sync_util.h"
#define NUM_READERS (2)
#define DEFAULT_WIDTH (2048)
#define BASEPATHSHM "/tmp/shm_ocp/"
#define SHMEMBASE "/ocp_sync"

typedef struct {
    pthread_mutex_t lock; // Only held during initialization to prevent multiple access
    volatile int abort; // A flag to be set to 1 whenever any error is detected

    cond_package reader_ready; // Once the reader joined it'll signal this value
    cond_package writer_ready;

    size_t mmap_size;
} coord_struct;

typedef struct {
    // Used either to signal that the reader arrived or that it finished reading
    cond_package signal_wrtr;

    pthread_mutex_t active[2];
    pthread_mutex_t leaving[2];
    size_t width;
    size_t data_size[2];
    char space[]; //contains two sections of size "width"
    
} copy_struct;

typedef struct {
    void* mem_region;
    int fd_shared;
    char* path;
} mmap_creat_ret_t;




int use_shared(setting_t settings);
int shared_sender(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info);

int shared_receiver(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info);
#endif
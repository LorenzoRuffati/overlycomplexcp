#ifndef HEADER_shared_memory
#define HEADER_shared_memory

#include <pthread.h>

#include "../shared/file_util.h"
#include "../shared/types.h"
#include "../shared/hash.h"
#include "sync_util.h"
#define NUM_READERS (2)
#define BASEPATHSHM "/tmp/shm_ocp/"
#define DEFAULT_WIDTH (2048)

typedef struct {
    pthread_mutex_t lock; // Only held during initialization to prevent
    
    // These two locks ensure only one sender/receiver
    pthread_mutex_t sender_lock;
    pthread_mutex_t receiver_lock;

    cond_package reader_arrived;
    cond_package writer_ready;

    size_t mmap_size;
    sha_hexdigest hash; // Used both for integrity check and for coordination
} coord_struct;

typedef struct {
    cond_package signal_rdr;
    // If started then this will be held by the writer
    cond_package reader_cnt;
    pthread_mutex_t started_lock;
    int started_flag;
    pthread_mutex_t joining_lock;

    pthread_rwlock_t active[2];
    pthread_rwlock_t leaving[2];
    size_t width;
    size_t data_size[2];
    char space[]; //contains two sections of size "width"
    
} copy_struct;

typedef struct {
    coord_struct* mem_region;
    int fd_shared;
    char* pat;
} mmap_creat_ret_t;




int use_shared(setting_t settings);
int shared_sender(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info);

int shared_receiver(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info);
#endif
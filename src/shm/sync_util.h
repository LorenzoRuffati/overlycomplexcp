#ifndef HEADER_shared_mem_sync
#define HEADER_shared_mem_sync
#include <pthread.h>

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int v;
} cond_package;

int lock(pthread_mutex_t* mutex);
int try_lock(pthread_mutex_t* mutex);

char * lockpath_from_pass(char* passw, char* base);
int lock_sync_file(char *passw, char* base);
void unlink_lock(char *passw, char* base);
int unlock_recover(pthread_mutex_t* mutex);
int init_mutex(pthread_mutex_t* mutex);
int init_rwlock(pthread_rwlock_t* rwlock);
int init_condvar(pthread_cond_t* cond);
void init_cond_pack(cond_package* cond);
#endif
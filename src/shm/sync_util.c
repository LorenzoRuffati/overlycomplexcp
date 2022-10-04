#include "sync_util.h"

#include <string.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "../shared/types.h"
#include "../shared/file_util.h"

char * lockpath_from_pass(char* passw, char* base){
    char* path = malloc(strlen(passw) + strlen(base)+ strlen(".lock")+1);
    strcpy(path, base);
    strcat(path, passw);
    strcat(path, ".lock");
    return path;
}

int lock_sync_file(char *passw, char* base){
    struct stat st = {0};
    if (stat(base, &st) == -1) {
        mkdir(base, 0770);
    }

    char* path = lockpath_from_pass(passw, base);
    int fd = open(path, O_CREAT, 0660);
    int res = flock(fd, LOCK_EX);
    free(path);

    if (res){
        printf("%d %d\n", res, errno);
        err_and_leave("Error when locking file", 5);
    }
    return fd;
}

void unlink_lock(char *passw, char* base){
    char* path = lockpath_from_pass(passw, base);
    unlink(path);
    free(path);
}

int lock(pthread_mutex_t* mutex){
    int r = pthread_mutex_lock(mutex);
    if (r==EOWNERDEAD){
        pthread_mutex_consistent(mutex);
    }
    return r;
}

int try_lock(pthread_mutex_t* mutex){
    int r = pthread_mutex_trylock(mutex);
    if (r==0){
        pthread_mutex_consistent(mutex);
    }
    return r;
}

int init_mutex(pthread_mutex_t* mutex){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    return pthread_mutex_init(mutex, &attr);
}

int init_rwlock(pthread_rwlock_t* rwlock){
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    return pthread_rwlock_init(rwlock, &attr);
}

int init_condvar(pthread_cond_t* cond){
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    return pthread_cond_init(cond, &attr);
}

void init_cond_pack(cond_package* cond){
    init_mutex(&(cond->lock));
    cond->v = 0;
    init_condvar(&(cond->cond));
}
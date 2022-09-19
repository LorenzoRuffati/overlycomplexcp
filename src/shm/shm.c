#include "shm.h"
#include <sys/mman.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#define BASEPATHSHM "/tmp/shm_ocp/"

char * lockpath_from_pass(char* passw){
    char* path = malloc(strlen(passw) + strlen(BASEPATHSHM)+ strlen(".lock"));
    strcpy(path, BASEPATHSHM);
    strcat(path, passw);
    strcat(path, ".lock");
    return path;
}

int lock_sync_file(char *passw){
    struct stat st = {0};
    if (stat(BASEPATHSHM, &st) == -1) {
        mkdir(BASEPATHSHM, 0770);
    }

    char* path = lockpath_from_pass(passw);

    int fd = open(path, O_CREAT, 0660);
    int res = flock(fd, LOCK_EX);
    if (res){
        printf("%d %d\n", res, errno);
        err_and_leave("Error when locking file", 5);
    }
    sleep(10);
    unlink(path);
    return fd;
}

int unlink_lock(char *passw){
    char* path = lockpath_from_pass(passw);
    unlink(path);
}


mmap_creat_ret_t init_or_create_mmap(char* passwd){
    char* path = "/ocp_sync";
    int fdm = shm_open(path, O_CREAT | O_EXCL | O_RDWR | O_TRUNC, 0660);
    printf("%d %d\n", fdm, errno);
    if (fdm == -1){
        if (errno == EEXIST){
            fdm = shm_open(path, O_RDWR, 0);
            coord_struct* str = (coord_struct*) mmap(NULL, sizeof(coord_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
            printf("%p %d\n", str, errno);
            return (mmap_creat_ret_t){str, fdm};
        } else {
            printf("%d\n", errno);
            err_and_leave("Error when accessing shared memory", 5);
        }
    }
    ftruncate(fdm, sizeof(coord_struct));
    coord_struct* str = /*malloc(sizeof(coord_struct));/*/ (coord_struct*) mmap(NULL, sizeof(coord_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
    // TODO: init all locks and the like here, returning will relinquish the lock and others might access
    printf("%p\n", str);
    printf("%d\n", errno);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    int ret = pthread_mutex_init(&(str->lock), &attr);
    printf("%d %d\n", ret, errno);
    return (mmap_creat_ret_t){str, fdm};
}

int use_shared(setting_t settings){
    int fd = lock_sync_file(settings.password);
    mmap_creat_ret_t shm_info = init_or_create_mmap(settings.password);

    flock(fd, LOCK_UN);

    switch (settings.role)
    {
    case SENDER:
        return shared_sender(settings, fd, shm_info);
        break;
    case RECEIVER:
        return shared_receiver(settings, fd, shm_info);
        break;
    default:
        err_and_leave("Incorrect role", 5);
        break;
    }
    return -1;
}

int shared_sender(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info){
    sha_hexdigest dig;
    hash_file(settings.filename, &dig);
    printf("%s %s\n", &(dig.x), settings.filename);
    coord_struct* shr_str = mmap_info.mem_region;
    // pthread_mutex_t lock_local;
    // pthread_mutex_init(&lock_local, 0);

    while (1){
        printf("Locking ...\n");
        pthread_mutex_lock(&shr_str->lock);
        //pthread_mutex_lock(&(shr_str->lock));
        printf("I have lock\n");
        for (int i=1; i<4; i++){
            printf("%d\n", i);
            sleep(1);
        }
        printf(":(\n");
        pthread_mutex_unlock(&shr_str->lock);
        sleep(0.5);
    }

}

int shared_receiver(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info){
    coord_struct* shr_str = mmap_info.mem_region;

    while (1){
        printf("Locking reader ...\n");
        pthread_mutex_lock(&shr_str->lock);
        printf("I have lock\n");
        for (int i=3; i>0; i--){
            printf("%d\n", i);
            sleep(1);
        }
        printf(":)\n");
        pthread_mutex_unlock(&shr_str->lock);
        sleep(0.5);
    }
}
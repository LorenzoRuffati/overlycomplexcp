#include "shm.h"
#include <sys/mman.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>


char* coord_path(char* passwd){
    char* path = malloc(strlen(SHMEMBASE)+strlen(passwd)+1);
    if (path == NULL){
        perror(NULL);
        err_and_leave("Malloc failed", 5);
    }
    strcpy(path, SHMEMBASE);
    strcat(path, passwd);
    return path;
}

// Tries to access the coordination shared memory area or creates it if
// it doesn't yet exist
mmap_creat_ret_t access_or_create_coord(char* passwd){
    char* path = coord_path(passwd);
    int fdm = shm_open(path, O_CREAT | O_EXCL | O_RDWR, 0660);
    //printf("opn_sync %d %d\n", fdm, errno);
    if (fdm == -1){
        if (errno == EEXIST){
            fdm = shm_open(path, O_RDWR, 0);
            if (fdm == -1){
                free(path);
                perror(NULL);
                err_and_leave("Error when opening existing shared memory file", 5);
            }
            coord_struct* str = (coord_struct*) mmap(NULL, sizeof(coord_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
            //printf("mmp sync %p %d\n", str, errno);
            return (mmap_creat_ret_t){.mem_region=str, .fd_shared=fdm, .path=path};
        } else {
            free(path);
            err_and_leave("Error when accessing shared memory", 5);
        }
    }
    int rt = ftruncate(fdm, sizeof(coord_struct));
    if (rt != 0){
        free(path);
        perror(NULL);
        err_and_leave("Error in ftruncate", 5);
    }
    coord_struct* str = (coord_struct*) mmap(NULL, sizeof(coord_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
    
    init_mutex(&(str->lock));
    init_cond_pack(&(str->reader_ready));
    init_cond_pack(&(str->writer_ready));
    str->abort = 0;

    return (mmap_creat_ret_t){.mem_region=str, .fd_shared=fdm, .path=path};
}

char* path_copy(char* passwd){
    char* path = malloc(2+strlen(passwd)+strlen("_copy"));
    if (path == NULL){
        perror(NULL);
        err_and_leave("Malloc failed", 5);
    }
    strcpy(path, "/");
    strcat(path, passwd);
    strcat(path,"_copy");
    return path;
}

mmap_creat_ret_t init_copy_area(char* passwd, size_t width, size_t* mmap_size){
    int fdm;
    char *path = path_copy(passwd);
    { // Create file for shared_memory
        fdm = shm_open(path, O_CREAT | O_EXCL | O_RDWR, 0660);
        //printf("opn_cp_shm %d %d\n", fdm, errno);
        if (fdm == -1){
            free(path);
            err_and_leave("Error when creating file-specific sharedmemory", 5);
        }
    }
    size_t m_sz = sizeof(copy_struct) + (2*width);
    //printf("Size of copy area: %ld + 2*%ld = %ld\n", sizeof(copy_struct), width, m_sz);
    *mmap_size = m_sz;
    int rt = ftruncate(fdm, m_sz);
    if (rt != 0){
        free(path);
        perror(NULL);
        err_and_leave("Error in ftruncate", 5);
    }
    copy_struct* copy_mem = (copy_struct*) mmap(NULL, m_sz, PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
    
    //printf("cmm %p %d\n", copy_mem, errno);
    init_cond_pack(&(copy_mem->signal_wrtr));


    init_mutex(&(copy_mem->active[0]));
    init_mutex(&(copy_mem->active[1]));
    init_mutex(&(copy_mem->leaving[0]));
    init_mutex(&(copy_mem->leaving[1]));
    copy_mem->width = width;

    return (mmap_creat_ret_t){.mem_region=copy_mem, .fd_shared=fdm, .path=path};
}


int use_shared(setting_t settings){
    if (settings.role == CLEANER){
        unlink_lock(settings.password, BASEPATHSHM);
        char *path_coord = coord_path(settings.password);
        shm_unlink(path_coord);
        char *path_cp = path_copy(settings.password);
        shm_unlink(path_cp);
        free(path_coord);
        free(path_cp);
        return 0;
    }

    int fd = lock_sync_file(settings.password, BASEPATHSHM);
    mmap_creat_ret_t shm_info = access_or_create_coord(settings.password);

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
    int fdin = open_file(settings.filename, O_RDONLY, 0);
    if (fdin == -1){
        shm_unlink(mmap_info.path);
        free(mmap_info.path);
        
        err_and_leave("Can't open input file", 5);
    }
    FILE* fstr = fdopen(fdin, "rb");
    if (fstr == NULL){
       err_and_leave("Couldn't open input stream", 5);
    }

    coord_struct* coord = (coord_struct*) mmap_info.mem_region;
    if (coord->abort != 0){
        shm_unlink(mmap_info.path);
        free(mmap_info.path);
        return 1;
    }
    pthread_mutex_lock(&(coord->lock)); // Prevent any other thread from joining
    pthread_mutex_lock(&(coord->writer_ready.lock));
    if (coord->writer_ready.v != 0){
        char *cpath = path_copy(settings.password);
        printf("A writer seems to be already using this memory area, try to use\n"
               "a different password. If the previous program crashed manually\n"
               "delete the files:\n"
               "- /dev/shm%s\n"
               "- /dev/shm%s\n", mmap_info.path, cpath);
        free(mmap_info.path);
        free(cpath);
        return 1;
    }

    size_t width = settings.max_pages * 1024;

    mmap_creat_ret_t ret_copy_init = init_copy_area(settings.password, width, &(coord->mmap_size));
    copy_struct* copy = (copy_struct*) ret_copy_init.mem_region;
    
    // Lock the first active area, now it's ready to accept readers
    pthread_mutex_lock(&(copy->active[0]));
    coord->writer_ready.v = 1;

    // Prevent the reader from signaling the writer
    pthread_mutex_lock(&(copy->signal_wrtr.lock));

    // At this point I'm holding: coord->lock, coord->writer_ready.lock, copy->active[0], copy->signal_wrtr.lock
    pthread_mutex_unlock(&(coord->lock));

    // Now check if readers have already arrived
    pthread_mutex_lock(&(coord->reader_ready.lock));
    if (coord->reader_ready.v != 0){
        // A reader already arrived, just signal that writer is ready
        pthread_mutex_unlock(&(coord->reader_ready.lock));

        pthread_cond_signal(&(coord->writer_ready.cond)); // Have already set writer_ready.v
        pthread_mutex_unlock(&(coord->writer_ready.lock));
        // Here I hold: copy->active[0], copy->signal_wrtr.lock
    } else {
        // Have to wait for reader
        pthread_mutex_unlock(&(coord->writer_ready.lock));
        while (coord->reader_ready.v == 0){
            pthread_cond_wait(&(coord->reader_ready.cond), &(coord->reader_ready.lock));
        }   
        // The writer will already have noticed that writer is ready
        // No need to signal writer ready
        pthread_mutex_unlock(&(coord->reader_ready.lock));
        // Here I hold copy->active[0], copy->signal_wrtr.lock
    }
    coord->abort = 1; // Anyone else coming needs to know this area of memory ceased being useful

    // Now I'm just holding the locks in copy, I need to subscribe to signal_wrtr until the reader arrived
    // Holding copy->active[0] and copy->signal_wrtr.lock
    copy->signal_wrtr.v = 0;
    while (copy->signal_wrtr.v == 0){
        pthread_cond_wait(&(copy->signal_wrtr.cond), &(copy->signal_wrtr.lock));
    }
    // I keep the lock on signal_wrtr, reader arrived and I can start writing
    
    int idx = 0;
    do
    { // I have lock on active[idx] (and signal_wrtr.lock)
        size_t n_r = fread(&(copy->space[copy->width * idx]),
                            1, copy->width, fstr);
        copy->data_size[idx] = n_r;

        pthread_mutex_lock(&(copy->leaving[idx]));
        pthread_mutex_unlock(&(copy->active[idx]));

        idx = !idx; // Invert

        pthread_mutex_lock(&(copy->active[idx]));
        pthread_mutex_unlock(&(copy->leaving[!idx]));
    } while (copy->data_size[!idx] != 0);
    // I have lock on active[idx] (and signal_wrtr.lock)
    pthread_mutex_unlock(&(copy->active[idx]));
    fclose(fstr);

    copy->signal_wrtr.v = 0;
    while (copy->signal_wrtr.v == 0){
        pthread_cond_wait(&(copy->signal_wrtr.cond), &(copy->signal_wrtr.lock));
    }
    // Now I'm alone, I can unlink the files and leave
    unlink_lock(settings.password, BASEPATHSHM);
    shm_unlink(mmap_info.path);
    free(mmap_info.path);
    shm_unlink(ret_copy_init.path);
    free(ret_copy_init.path);
    close(mmap_info.fd_shared);
    close(ret_copy_init.fd_shared);
    close(lockfd);
    return 0;
}

int shared_receiver(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info){
    coord_struct* coord = (coord_struct*) mmap_info.mem_region;
    if (coord->abort != 0){
        shm_unlink(mmap_info.path);
        free(mmap_info.path);
        return 1;
    }
    free(mmap_info.path);
    close(lockfd);
    
    FILE* fstr = fopen(settings.filename, "w");
    if (fstr == NULL){
        perror(NULL);
        err_and_leave("Couldn't open input stream", 5);
    }
    int rt = ftruncate(fileno(fstr), 0);
    if (rt != 0){
        perror(NULL);
        err_and_leave("Error in ftruncate", 5);
    }
    //printf("Opened file\n");
    
    pthread_mutex_lock(&(coord->lock)); // Prevent any other thread from joining
    pthread_mutex_lock(&(coord->reader_ready.lock));
    if (coord->reader_ready.v != 0){
        return 1;
    }

    coord->reader_ready.v = 1;
    pthread_mutex_unlock(&(coord->lock));

    pthread_cond_signal(&(coord->reader_ready.cond));
    pthread_mutex_unlock(&(coord->reader_ready.lock));

    pthread_mutex_lock(&(coord->writer_ready.lock));
    while (coord->writer_ready.v != 1){
        pthread_cond_wait(&(coord->writer_ready.cond), &(coord->writer_ready.lock));
    }
    pthread_mutex_unlock(&(coord->writer_ready.lock));

    int fdm;
    { // Create file for shared_memory
        char *path = path_copy(settings.password);
        fdm = shm_open(path, O_RDWR , 0660);
        free(path);
        //printf("opn_cp_shm %d %d\n", fdm, errno);
        if (fdm == -1){
            perror(NULL);
            err_and_leave("Error when accessing file-specific sharedmemory", 5);
        }
    }

    copy_struct* copy = (copy_struct*) mmap(NULL, coord->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
    
    pthread_mutex_lock(&(copy->leaving[1]));

    pthread_mutex_lock(&(copy->signal_wrtr.lock));
        copy->signal_wrtr.v = 1;
        pthread_cond_signal(&(copy->signal_wrtr.cond));
    pthread_mutex_unlock(&(copy->signal_wrtr.lock));

    close(mmap_info.fd_shared);

    // Here I'm only holding copy->leaving[1]

    int to_read = 1;
    int idx = 0;
    while (to_read){
        // At the beginning of this cycle it has to read at idx and holds the lock for leaving[!idx]
        pthread_mutex_lock(&(copy->active[idx]));
        pthread_mutex_unlock(&(copy->leaving[!idx]));

        size_t n_wr = fwrite(&(copy->space[copy->width * idx]), 1, copy->data_size[idx], fstr);
        if (n_wr != copy->data_size[idx]){
            err_and_leave("Error", 5);
        }
        to_read = (n_wr > 0);

        //printf("Read %ld bytes\n", n_wr);

        // Lock the leaving lock
        pthread_mutex_lock(&(copy->leaving[idx]));
        // Release active
        pthread_mutex_unlock(&(copy->active[idx]));
        idx = !idx;
        fflush(fstr);
        // Here I have the lock on leaving[!idx]
    }
    fclose(fstr);

    pthread_mutex_unlock(&(copy->leaving[!idx]));
    pthread_mutex_lock(&(copy->signal_wrtr.lock));
    copy->signal_wrtr.v = 1;
    pthread_cond_signal(&(copy->signal_wrtr.cond));
    pthread_mutex_unlock(&(copy->signal_wrtr.lock));

    close(fdm);
    return 0;
}
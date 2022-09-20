#include "shm.h"
#include <sys/mman.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>


mmap_creat_ret_t init_or_create_mmap(char* passwd){
    char* path = malloc(strlen("/ocp_sync")+strlen(passwd)+1);
    strcpy(path, "/ocp_sync");
    strcat(path, passwd);
    int fdm = shm_open(path, O_CREAT | O_EXCL | O_RDWR | O_TRUNC, 0660);
    printf("opn_sync %d %d\n", fdm, errno);
    if (fdm == -1){
        if (errno == EEXIST){
            fdm = shm_open(path, O_RDWR, 0);
            coord_struct* str = (coord_struct*) mmap(NULL, sizeof(coord_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
            printf("mmp sync %p %d\n", str, errno);
            return (mmap_creat_ret_t){str, fdm};
        } else {
            err_and_leave("Error when accessing shared memory", 5);
        }
    }
    ftruncate(fdm, sizeof(coord_struct));
    coord_struct* str = /*malloc(sizeof(coord_struct));/*/ (coord_struct*) mmap(NULL, sizeof(coord_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
    // TODO: init all locks and the like here, returning will relinquish the lock and others might access
    init_mutex(&(str->lock));
    init_mutex(&(str->sender_lock));
    init_mutex(&(str->receiver_lock));
    init_cond_pack(&(str->reader_arrived));
    init_cond_pack(&(str->writer_ready));

    return (mmap_creat_ret_t){str, fdm, path};
}

copy_struct* init_copy(size_t width, int fd){
    size_t size = sizeof(copy_struct) + (2 * width);
    ftruncate(fd, size);
    copy_struct* copy_mem = (copy_struct*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("cmm %p %d\n", copy_mem, errno);
    init_cond_pack(&(copy_mem->reader_cnt));
    init_cond_pack(&(copy_mem->signal_rdr));

    init_mutex(&(copy_mem->started_lock));
    init_mutex(&(copy_mem->joining_lock));

    init_rwlock(&(copy_mem->active[0]));
    init_rwlock(&(copy_mem->active[1]));
    init_rwlock(&(copy_mem->leaving[0]));
    init_rwlock(&(copy_mem->leaving[1]));
    copy_mem->width = width;
    return copy_mem;
}

int use_shared(setting_t settings){
    int fd = lock_sync_file(settings.password, BASEPATHSHM);
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

typedef struct {
    char* filename;
    coord_struct* coord;
    copy_struct* copy;
    cond_package* writer_ready;
} thrd_arg;


int writer_thread(thrd_arg* arg){
    /*
    //  subscribe to sync_var
    //      When awaken lock started_lock
    //      Check if the number of readers equals the target
    //      If it does set_started lock
    //      If not then subscribe and repeat
    // When started the assisting thread will take care of deleting the
    // coordination shared memory segment (lock the lock-file before)
    // The original thread will write until it finished writing, then
    // it'll again subscribe to number of readers but waiting until 0
    // this time
    // Writing process, starts with a wr-lock on active[i](i==0)
    // Write the data to `space+(width*i)` and the amount of data
    // to data_size[i]
    // get w-lock on leaving[i]
    // release w-lock on active[i]
    // i = !i
    // get w-lock on active[i]
    // release w-lock on leaving[!i]
    // restart
    // if the amount of data written is 0 then lock the number-of readers,
    // then release the active lock and subscribe to number-of-readers
    // Until the number of readers is 0 re-subscribe
    // When number of readers is 0 unlink the shm file and leave 

    */
    printf("thr-c %p\t%p\t%p\t%p\n", arg->filename, arg->coord, arg->copy, arg->writer_ready);

    cond_package* wr_ready = arg->writer_ready;
    copy_struct* copy_mem = arg->copy;

    // To do before allowing readers to join:
    // + Take writing lock on the first active segment (will allow them to queue in waiting[1])
    // + Take lock on reader_cnt (will prevent them from updating it)
    // + Set started flag to false
    int active_segment = 0;
    pthread_rwlock_wrlock(&(copy_mem->active[active_segment]));
    printf("\tWrite lock on active %d\n", active_segment);
    pthread_mutex_lock(&(copy_mem->reader_cnt.lock));
    
    pthread_mutex_lock(&(copy_mem->started_lock));
    copy_mem->started_flag = 0;
    pthread_mutex_unlock(&(copy_mem->started_lock));

    // Now wake up the parent thread, readers will start joining, queued by joining_lock and the
    // first will wait on reader_cnt
    pthread_mutex_lock(&(wr_ready->lock));
    wr_ready->v = 1;
    pthread_cond_signal(&(wr_ready->cond)); // Signals the parent threat to start letting 
                                            // readers in, they'll be blocked by the sync_var lock
    
    int r = pthread_mutex_unlock(&(wr_ready->lock)); // Allow parent to wake up
    printf("unlckw %d %d\n", r, errno);

    // ACTIVE LOCKS: active[0], reader_cnt
    copy_mem->reader_cnt.v = 0;
    // Repeat until enough joined:
    // 1. Signal to signal_rdr that it can continue (first iteration this will be pointless)
    // 2. Wait on reader_cnt (unlocking it)
    // If it exited before signaling reader_cnt I need to:
    // + Update the begin flag
    // + Signal the reader
    while (copy_mem->reader_cnt.v < NUM_READERS){
        printf("\tWaiting for readers\n");
        pthread_mutex_lock(&(copy_mem->signal_rdr.lock));
            copy_mem->signal_rdr.v = 1;
            pthread_cond_signal(&(copy_mem->signal_rdr.cond));
        pthread_mutex_unlock(&(copy_mem->signal_rdr.lock)); // Will let the next reader join but it'll block on cnt

        pthread_cond_wait(&(copy_mem->reader_cnt.cond), &(copy_mem->reader_cnt.lock));
        printf("\tHave %d readers\n", copy_mem->reader_cnt.v);
    }
    // Here we can start
    pthread_mutex_lock(&(copy_mem->started_lock));
        copy_mem->started_flag = 1;
    pthread_mutex_unlock(&(copy_mem->started_lock));

    pthread_mutex_lock(&(copy_mem->signal_rdr.lock));
        copy_mem->signal_rdr.v = 1;
        pthread_cond_signal(&(copy_mem->signal_rdr.cond));
    pthread_mutex_unlock(&(copy_mem->signal_rdr.lock)); // Will let the next reader join but it'll fail the begin check
    
    // ACTIVE LOCKS: active[0], reader_cnt
    
    // Before starting wake up the parent thread (pretending to be a new reader)
    coord_struct* coord = arg->coord;
    pthread_mutex_lock(&(coord->reader_arrived.lock));
    coord->reader_arrived.v = 1;
    pthread_cond_signal(&(coord->reader_arrived.cond));
    pthread_mutex_unlock(&(coord->reader_arrived.lock));

    // Then I can start writing to the memory
    // 1. Read into the buffer
    // 2. Update the size
    // 3. Take the leaving lock
    // 4. Release active lock
    // 5. Take active lock next
    // 6. Release leaving lock
    FILE* fstr = fopen(arg->filename, "rb");
    do
    { // I have wr-lock on active[active_segment]
        size_t n_r = fread(&(copy_mem->space[copy_mem->width * active_segment]),
                            1, copy_mem->width, fstr);
        copy_mem->data_size[active_segment] = n_r;

        pthread_rwlock_wrlock(&(copy_mem->leaving[active_segment]));
        pthread_rwlock_unlock(&(copy_mem->active[active_segment]));
        active_segment = !active_segment;

        pthread_rwlock_wrlock(&(copy_mem->active[active_segment]));
        pthread_rwlock_unlock(&(copy_mem->leaving[!active_segment]));
    } while (copy_mem->data_size[!active_segment] != 0);
    printf("\tFinished writing\n");

    // Here I have a lock on active[active_segment] and on reader_cnt
    // while data_size[!active_segment] is 0
    copy_mem->data_size[active_segment] = 0; // Just in case
    pthread_rwlock_unlock(&(copy_mem->active[active_segment]));

    printf("\tHave %d readers (leaving)\n", copy_mem->reader_cnt.v);
    while (copy_mem->reader_cnt.v > 0){
        pthread_cond_wait(&(copy_mem->reader_cnt.cond), &(copy_mem->reader_cnt.lock));
        printf("\tHave %d readers\n (leaving)", copy_mem->reader_cnt.v);
    }
    pthread_mutex_unlock(&(copy_mem->reader_cnt.lock));
    // TODO I'm the last in the memory segment do cleanup and return

    return 0;
}


int shared_sender(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info){
    coord_struct* shr_str = mmap_info.mem_region;

    int r = try_lock(&(shr_str->sender_lock));
    if (r!=0) {
        printf("nfw %d %d\n", r, errno);
        err_and_leave("Not the first writer", 5);
    }
    // LOCKS HERE: sender_lock

    sha_hexdigest dig;
    hash_file(settings.filename, &dig);
    printf("hxdg %s %s\n",(char*) &(dig.x), settings.filename);
    shr_str->hash = dig;
    shr_str->mmap_size = sizeof(copy_struct) + (2 * DEFAULT_WIDTH);
    
    lock(&(shr_str->writer_ready.lock));
    shr_str->writer_ready.v=0;
    pthread_mutex_unlock(&(shr_str->writer_ready.lock));

    // Init the shared memory file for copy
    int fdm;
    char* path = malloc(3+(SHA_DIGEST_LENGTH*2)+strlen(settings.password));
    { // Create file for shared_memory
        strcpy(path, "/");
        strcat(path, settings.password);
        strcat(path,"_");
        strcat(path, shr_str->hash.x);
        fdm = shm_open(path, O_CREAT | O_EXCL | O_RDWR | O_TRUNC, 0660);
        printf("opn_cp_shm %d %d\n", fdm, errno);
        if (fdm == -1){
            err_and_leave("Error when creating file-specific sharedmemory", 5);
        }
    }
    
    copy_struct* copy_mem = init_copy(DEFAULT_WIDTH, fdm);
    
    // 
    // [Assumption] start condition is a certain number of readers
    // Lock "sync_var lock" (prevent signaling for readers)
    // Spawn a thread to issue the "writer ready" broadcast for every new reader joining
    //      until started_flag is 1 
    pthread_t child_thread;

    cond_package* writer_thr_ready = malloc(sizeof(cond_package));
    init_cond_pack(writer_thr_ready);
    thrd_arg arg = {settings.filename, shr_str, copy_mem, writer_thr_ready};
    printf("thr-a %p\t%p\t%p\t%p\n", settings.filename, shr_str, copy_mem, writer_thr_ready);

    pthread_mutex_lock(&(writer_thr_ready->lock));
    pthread_create(&child_thread, NULL, (void * (*)(void *)) writer_thread, &arg);

    printf("Now waiting for ready\n");
    while (writer_thr_ready->v != 1){
        pthread_cond_wait(&(writer_thr_ready->cond), &(writer_thr_ready->lock));
        printf("waiting\n");
    }
    int r_un = pthread_mutex_unlock(&(writer_thr_ready->lock));
    printf("ulk %d %d\n", r_un, errno);
    
    pthread_mutex_lock(&(copy_mem->started_lock));
    // LOCKS HERE: sender_lock, copy_mem->started_lock
    while (copy_mem->started_flag != 1){
        printf("Unlocking started_lock\n");
        pthread_mutex_unlock(&(copy_mem->started_lock));

        printf("Locking writer_ready.lock - ");
        pthread_mutex_lock(&(shr_str->writer_ready.lock));
        printf("ok\n");
        shr_str->writer_ready.v = 1;

        printf("Locking reader_arrived.lock - ");
        pthread_mutex_lock(&(shr_str->reader_arrived.lock));
        printf("ok \n");
        shr_str->reader_arrived.v = 0;

        pthread_cond_broadcast(&(shr_str->writer_ready.cond));
        pthread_mutex_unlock(&(shr_str->writer_ready.lock));
        while (shr_str->reader_arrived.v != 1){
            pthread_cond_wait(&(shr_str->reader_arrived.cond), &(shr_str->reader_arrived.lock));
            printf("Have woken\n");
        }
        printf("Left inner while\n");
        pthread_mutex_unlock(&(shr_str->reader_arrived.lock));

        printf("Locking started_lock - ");
        pthread_mutex_lock(&(copy_mem->started_lock));
        printf("ok\n");
        // LOCKS HERE have to be: sender_lock, copy_mem->started_lock
    }
    printf("Writing started\n");

    // Unlink the file
    printf("lock_sync\n");
    lock_sync_file(settings.password, BASEPATHSHM);
    
    void *ret = NULL;
    pthread_join(child_thread, &ret);
    printf("Joined thread\n");

    printf("del lock\n");
    unlink_lock(settings.password, BASEPATHSHM);
    ftruncate(mmap_info.fd_shared, 0);
    printf("unlink shm\n");
    int r_unl = shm_unlink(mmap_info.pat);
    printf("unlkshm %d %d\n", r_unl, errno);
    pthread_cond_broadcast(&(shr_str->writer_ready.cond));
    printf("Waiting for child thread\n");
    munmap(shr_str, sizeof(coord_struct));
    shm_unlink(path);
    return 1;
}

int shared_receiver(setting_t settings, int lockfd, mmap_creat_ret_t mmap_info){
    coord_struct* shr_str = mmap_info.mem_region;

    // Lock reader joined lock
    // Lock writer ready lock (in this order to avoid possible deadlocks)
    // Signal that a reader joined (writer will wake up and wait for writer-ready to be released)
    // Wait on writer ready

    // Reset the writer ready flag (anyway I'll signal the writer that a new reader joined)
    lock(&(shr_str->writer_ready.lock));
        shr_str->writer_ready.v = 0;
    pthread_mutex_unlock(&(shr_str->writer_ready.lock));

    pthread_mutex_lock(&(shr_str->writer_ready.lock));
    while (shr_str->writer_ready.v != 1){   // Since I have reset it earlier if it's 1 it means the writer
                                            // thread is present and ready
        // Here I'm holding the writer_ready lock so no other thread can signal
        pthread_mutex_lock(&(shr_str->reader_arrived.lock));
        shr_str->reader_arrived.v = 1; // Signal to the writer that there is at least one reader ready
        pthread_cond_signal(&(shr_str->reader_arrived.cond));
        pthread_mutex_unlock(&(shr_str->reader_arrived.lock));  // Unlock this so that the writer can actually
                                                                // since I'm holding the writer lock it won't be able to
                                                                // signal yet
        pthread_cond_wait(&(shr_str->writer_ready.cond), &(shr_str->writer_ready.lock)); // Wait and release lock
        // When I wake up I'll hold the lock
    }
    pthread_mutex_unlock(&(shr_str->writer_ready.lock));
    // Writer was ready so I can access the copy_shm segment

    int fdm;
    { // Access file for shared_memory
        char* path = malloc(3+(SHA_DIGEST_LENGTH*2)+strlen(settings.password));
        strcpy(path, "/");
        strcat(path, settings.password);
        strcat(path,"_");
        strcat(path, shr_str->hash.x);
        fdm = shm_open(path, O_RDWR, 0660);
        printf("opn_cp_shm %d %d\n", fdm, errno);
        if (fdm == -1){
            err_and_leave("Error when creating file-specific sharedmemory", 5);
        }
    }

    copy_struct* copy_mem = (copy_struct*) mmap(NULL, shr_str->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdm, 0);
    printf("rc_mmp_cp %p %d\n", copy_mem, errno);

    
    //
    // Access shared memory
    // Signal 

    // When joining the actual copy struct lock the joining_lock to serialize access
    printf("Locking joining_lock - ");
    pthread_mutex_lock(&(copy_mem->joining_lock));
    printf("ok\n");

    // When awoken it opens the shared memory, takes the lock for a new reader
    // then it acquires the lock for the
    // started flag, if the copy started it means this reader is unneeded and exits
        printf("Locking started_lock -");
        pthread_mutex_lock(&(copy_mem->started_lock));
        printf(" ok\n");
        if (copy_mem->started_flag){
            pthread_mutex_unlock(&(copy_mem->started_lock));
            err_and_leave("Copying already started", 5);
        } 
        pthread_mutex_unlock(&(copy_mem->started_lock));
        printf("Unlocking started_lock");

    // If it didn't start then it increments the counter for number of readers
    // and signals that it changed (all this process needs to be guarded by the new 
    //      reader flag to serialize access)
        printf("Locking reader_cnt.lock -");
        pthread_mutex_lock(&(copy_mem->reader_cnt.lock));
        printf("ok\n");
        copy_mem->reader_cnt.v += 1;

    printf("Locking rdlock %d -", 1);
    pthread_rwlock_rdlock(&(copy_mem->leaving[1])); // The signal might cause the writer to start writing
                                                    // so I need to lock before signaling
    printf(" ok\n");
        printf("Locking signal_rdr.lock -");
        pthread_mutex_lock(&(copy_mem->signal_rdr.lock));
        printf(" ok\n");
        copy_mem->signal_rdr.v = 0;

        pthread_cond_signal(&(copy_mem->reader_cnt.cond));
        pthread_mutex_unlock(&(copy_mem->reader_cnt.lock));
        
        while (copy_mem->signal_rdr.v == 0){
            pthread_cond_wait(&(copy_mem->signal_rdr.cond), &(copy_mem->signal_rdr.lock));
        }
        pthread_mutex_unlock(&(copy_mem->signal_rdr.lock));
    
    pthread_mutex_unlock(&(copy_mem->joining_lock)); // Now I can let others join
    // This allows others to join and possibly lock the started_lock before the writer can update it
    // it shouldn't deadlock but could allow more readers than expected

    FILE* fstr = fopen(settings.filename, "w");
    ftruncate(fileno(fstr), 0);
    printf("Opened file\n");

    int to_read = 1;
    int idx = 0;
    // Then it acquires a reading lock on waiting[1] waits for the reading lock on the first segment (i=0)
    // When it wakes up it reads segment "i" (up to data_size[i]) to the file
    while (to_read)
    {
        // At the beginning of this cycle it has to read at idx and holds the lock for leaving[!idx]
        pthread_rwlock_rdlock(&(copy_mem->active[idx]));
            printf("Read lock on active %d\n", idx);
            pthread_rwlock_unlock(&(copy_mem->leaving[!idx]));

        // and then waits for a reading lock on leaving[i], when it acquires it
        // it releases its reading lock on active[i], and inverts i (first iteration i=1)
        // then it waits for a reading lock on active[i], when it acquires it it
        // releases the lock on leaving[!i]
            size_t n_wr = fwrite(&(copy_mem->space[copy_mem->width * idx]), 1, copy_mem->data_size[idx], fstr);
            if (n_wr != copy_mem->data_size[idx]){
                err_and_leave("Error", 5);
            }
            printf("Read %ld\n", n_wr);
            to_read = (n_wr > 0);
            pthread_rwlock_rdlock(&(copy_mem->leaving[idx]));
            printf("Read lock on leaving %d\n", idx);
        pthread_rwlock_unlock(&(copy_mem->active[idx]));
        idx = !idx;
        fflush(fstr);
    }
    pthread_rwlock_unlock(&(copy_mem->leaving[!idx]));
    fclose(fstr);
    
    printf("Leaving\n");
    //
    // if data_size[i] is 0 then the reading ended, so it (with locks) decrements
    pthread_mutex_lock(&(copy_mem->reader_cnt.lock));
    copy_mem->reader_cnt.v -= 1;
    // the reader counter and signals the change in readers, then it leaves
    pthread_cond_signal(&(copy_mem->reader_cnt.cond));
    pthread_mutex_unlock(&(copy_mem->reader_cnt.lock));
    return 0;
}
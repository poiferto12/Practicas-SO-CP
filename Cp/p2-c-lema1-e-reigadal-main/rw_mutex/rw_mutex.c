typedef struct rw_mutex_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int readers;
    int writers;
}rw_mutex_t;

#include <pthread.h>
#include "rw_mutex.h"

int rw_mutex_init(rw_mutex_t *m){
    pthread_mutex_init(&m->mutex, NULL);
    pthread_cond_init(&m->cond, NULL);
    m->readers = 0;
    m->writers = 0;
    return 0;
}

int rw_mutex_destroy(rw_mutex_t *m) {
    pthread_mutex_destroy(&m->mutex);
    pthread_cond_destroy(&m->cond);
    return 0;
}

int rw_mutex_readlock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->mutex);

    while (m->writers > 0){
        pthread_cond_wait(&m->cond, &m->mutex);
    }

    m->readers++;

    pthread_mutex_unlock(&m->mutex);
    return 0;
}

int rw_mutex_writelock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->mutex);

    while (m->writers > 0 || m->readers > 0) {
        pthread_cond_wait(&m->cond, &m->mutex);
    }

    m->writers = 1;

    pthread_mutex_unlock(&m->mutex);
    return 0;
}

int rw_mutex_readunlock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->mutex);

    m->readers--;

    if (m->readers == 0) {
        pthread_cond_broadcast(&m->cond);
    }

    pthread_mutex_unlock(&m->mutex);
    return 0;
}
   

int rw_mutex_writeunlock(rw_mutex_t *m) {
    
    pthread_mutex_lock(&m->mutex);

    m->writers = 0;

    pthread_cond_broadcast(&m->cond);

    pthread_mutex_unlock(&m->mutex);
    return 0;
}
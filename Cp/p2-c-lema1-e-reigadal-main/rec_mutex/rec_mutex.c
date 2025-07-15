#include <pthread.h>

struct rec_mutex_t {
    pthread_mutex_t mutex;
    int lock_counter;
    int thread_creador;
};

#include "rec_mutex.h"

int rec_mutex_init(rec_mutex_t *m) {
    pthread_mutex_init(&m->mutex, NULL);
    m->lock_counter = 0;
    m->thread_creador = 0;
    return 0;
}

int rec_mutex_destroy(rec_mutex_t *m) {
    pthread_mutex_destroy(&m->mutex);
    return 0;
}

int rec_mutex_lock(rec_mutex_t *m) {
    if(m->lock_counter > 0 && m->thread_creador == pthread_self()){
        m->lock_counter ++;    
    }else{
        pthread_mutex_lock(&m->mutex);
        m->lock_counter = 1;
        m->thread_creador = pthread_self();
    }
    return 0;
};

int rec_mutex_unlock(rec_mutex_t *m) {
    if(m->lock_counter > 1 && m->thread_creador == pthread_self()){
        m->lock_counter --;    
    }else{
        pthread_mutex_unlock(&m->mutex);
    }
    return 0;
};

int rec_mutex_trylock(rec_mutex_t *m) {
    if(m->lock_counter > 0 && m->thread_creador == pthread_self()){
        // Si el mutex ya está bloqueado por este hilo, incrementamos el contador
        m->lock_counter++;
        return 0;
    } else if (m->lock_counter > 0) {
        // Si el mutex está bloqueado por otro hilo, fallamos
        return -1;
    } else {
        // Si el mutex no está bloqueado, intentamos adquirirlo sin bloquear
        if (pthread_mutex_trylock(&m->mutex) == 0) {
            m->lock_counter = 1;
            m->thread_creador = pthread_self();
            return 0;
        } else {
            return -1;
        }
    }
}


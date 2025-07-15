#include <stdlib.h>
#include <pthread.h>
#include "sem.h"

int sem_init(sem_t *s, int value) {
    s->value = value;
    int result = pthread_mutex_init(&s->mutex, NULL);
    if (result != 0) return result;
    
    result = pthread_cond_init(&s->cond, NULL);
    if (result != 0) {
        pthread_mutex_destroy(&s->mutex);
        return result;
    }
    
    return 0;
}

int sem_destroy(sem_t *s) {
    int result = pthread_cond_destroy(&s->cond);
    int result2 = pthread_mutex_destroy(&s->mutex);
    return (result != 0) ? result : result2;
}

int sem_p(sem_t *s) { // wait() or P()
    pthread_mutex_lock(&s->mutex);
    
    while (s->value <= 0) {
        pthread_cond_wait(&s->cond, &s->mutex);
    }
    
    s->value--;
    pthread_mutex_unlock(&s->mutex);
    
    return 0;
}

int sem_v(sem_t *s) { // signal V()
    pthread_mutex_lock(&s->mutex);
    
    s->value++;
    pthread_cond_signal(&s->cond);
    
    pthread_mutex_unlock(&s->mutex);
    
    return 0;
}

int sem_tryp(sem_t *s) { // 0 on success, -1 if already locked
    pthread_mutex_lock(&s->mutex);
    
    if (s->value > 0) {
        s->value--;
        pthread_mutex_unlock(&s->mutex);
        return 0;
    } else {
        pthread_mutex_unlock(&s->mutex);
        return -1;
    }
}
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "op_count.h"
#include "options.h"

struct buffer {
    int *data;
    pthread_mutex_t *mutexes;
    int size;
    pthread_mutex_t print_mutex;
    bool print_thread_running;
    int iterations;
    pthread_mutex_t iterations_mutex;
};

struct thread_info {
    pthread_t       thread_id;        // id returned by pthread_create()
    int             thread_num;       // application defined thread #
};

struct args {
    int             thread_num;       // application defined thread #
    int             delay;            // delay between operations
    struct buffer   *buffer;          // Shared buffer
    int             print_wait;       // delay between prints of the array
};

void *swap(void *ptr)
{
    struct args *args =  ptr;

    while (true) {
        pthread_mutex_lock(&args->buffer->iterations_mutex);
        if (args->buffer->iterations <= 0) {
            pthread_mutex_unlock(&args->buffer->iterations_mutex);
            break;
        }
        args->buffer->iterations--;
        pthread_mutex_unlock(&args->buffer->iterations_mutex);

        int i, j, tmp;
        i = rand() % args->buffer->size;
        j = rand() % args->buffer->size;

        if (i == j) {
            pthread_mutex_lock(&args->buffer->iterations_mutex);
            args->buffer->iterations++;
            pthread_mutex_unlock(&args->buffer->iterations_mutex);
            continue;
        }

        int first = (i < j) ? i : j;
        int second = (i > j) ? i : j;

        pthread_mutex_lock(&args->buffer->mutexes[first]);
        pthread_mutex_lock(&args->buffer->mutexes[second]);

        pthread_mutex_lock(&args->buffer->print_mutex);
        printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
            args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);
        pthread_mutex_unlock(&args->buffer->print_mutex);

        tmp = args->buffer->data[i];
        if (args->delay) usleep(args->delay);

        args->buffer->data[i] = args->buffer->data[j];
        if (args->delay) usleep(args->delay);

        args->buffer->data[j] = tmp;
        if (args->delay) usleep(args->delay);

        pthread_mutex_unlock(&args->buffer->mutexes[second]);
        pthread_mutex_unlock(&args->buffer->mutexes[first]);

        inc_count();
    }
    return NULL;
}

void *print_thread(void *ptr)
{
    struct args *args = ptr;
    int *local_copy = malloc(args->buffer->size * sizeof(int));

    while (true) {
        usleep(args->print_wait);

        pthread_mutex_lock(&args->buffer->print_mutex);
        if (!args->buffer->print_thread_running) {
            pthread_mutex_unlock(&args->buffer->print_mutex);
            break;
        }
        pthread_mutex_unlock(&args->buffer->print_mutex);

        for (int i = 0; i < args->buffer->size; i++) {
            pthread_mutex_lock(&args->buffer->mutexes[i]);
            local_copy[i] = args->buffer->data[i];
            pthread_mutex_unlock(&args->buffer->mutexes[i]);
        }

        pthread_mutex_lock(&args->buffer->print_mutex);
        printf("Buffer content: ");
        for (int i = 0; i < args->buffer->size; i++) {
            printf("%d ", local_copy[i]);
        }
        printf("\n");
        pthread_mutex_unlock(&args->buffer->print_mutex);
    }

    free(local_copy);
    return NULL;
}

int cmp(int *e1, int *e2) {
    if(*e1==*e2) return 0;
    if(*e1<*e2) return -1;
    return 1;
}

void print_buffer(struct buffer buffer) {
    int i;

    for (i = 0; i < buffer.size; i++)
        printf("%i ", buffer.data[i]);
    printf("\n");
}

void start_threads(struct options opt)
{
    int i;
    struct thread_info *threads;
    struct args *args;
    struct buffer buffer;

    srand(time(NULL));

    if((buffer.data = malloc(opt.buffer_size * sizeof(int))) == NULL) {
        printf("Out of memory\n");
        exit(1);
    }
    if((buffer.mutexes = malloc(opt.buffer_size * sizeof(pthread_mutex_t))) == NULL) {
        printf("Out of memory\n");
        exit(1);
    }
    buffer.size = opt.buffer_size;
    buffer.print_thread_running = true;
    pthread_mutex_init(&buffer.print_mutex, NULL);
    pthread_mutex_init(&buffer.iterations_mutex, NULL);
    buffer.iterations = opt.iterations;

    for(i = 0; i < buffer.size; i++) {
        buffer.data[i] = i;
        pthread_mutex_init(&buffer.mutexes[i], NULL);
    }

    printf("creating %d threads\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * (opt.num_threads + 1));
    args = malloc(sizeof(struct args) * (opt.num_threads + 1));

    if (threads == NULL || args == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    printf("Buffer before: ");
    print_buffer(buffer);

    // Print thread
    args[opt.num_threads].buffer = &buffer;
    args[opt.num_threads].print_wait = opt.print_wait;
    if (0 != pthread_create(&threads[opt.num_threads].thread_id, NULL, print_thread, &args[opt.num_threads])) {
        printf("Could not create print thread");
        exit(1);
    }

    // Create num_thread threads running swap()
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].thread_num = i;

        args[i].thread_num = i;
        args[i].buffer     = &buffer;
        args[i].delay      = opt.delay;

        if (0 != pthread_create(&threads[i].thread_id, NULL, swap, &args[i])) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    // Wait for the swap threads to finish
    for (i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].thread_id, NULL);

    // Stop the print thread
    pthread_mutex_lock(&buffer.print_mutex);
    buffer.print_thread_running = false;
    pthread_mutex_unlock(&buffer.print_mutex);

    pthread_join(threads[opt.num_threads].thread_id, NULL);

    // Print the buffer
    printf("Buffer after:  ");
    qsort(buffer.data, opt.buffer_size, sizeof(int), (int (*)(const void *, const void *)) cmp);
    print_buffer(buffer);
    
    printf("iterations: %d\n", get_count());

    for(i = 0; i < buffer.size; i++) {
        pthread_mutex_destroy(&buffer.mutexes[i]);
    }
    pthread_mutex_destroy(&buffer.print_mutex);
    pthread_mutex_destroy(&buffer.iterations_mutex);

    free(args);
    free(threads);
    free(buffer.data);
    free(buffer.mutexes);

    pthread_exit(NULL);
}

int main (int argc, char **argv)
{
    struct options opt;

    // Default values for the options
    opt.num_threads = 10;
    opt.buffer_size = 10;
    opt.iterations  = 100;
    opt.delay       = 10;
    opt.print_wait  = 1000; // Default print wait time in milliseconds

    read_options(argc, argv, &opt);

    start_threads(opt);

    pthread_exit(NULL);  // Asegurarse de que todos los hilos terminen antes de salir
}
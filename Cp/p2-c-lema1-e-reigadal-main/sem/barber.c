#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include "options.h"
#include "sem.h"

volatile bool running = true;

sem_t clientes;
sem_t barberos;
sem_t mutex_sillas;
int sillas_ocupadas = 0;
int capacidad_sala;

struct options opt;

void* barber(void* arg) {
    int id = *(int*)arg;
    
    while (running) {
        printf("Barbero %d esperando clientes...\n", id);
        sem_p(&clientes);
        
        if (!running) break;
        
        sem_p(&mutex_sillas);
        sillas_ocupadas--;
        printf("Barbero %d va a atender a un cliente. Sillas ocupadas: %d/%d\n", 
               id, sillas_ocupadas, capacidad_sala);
        sem_v(&mutex_sillas);
        
        printf("Barbero %d cortando el pelo al cliente\n", id);
        usleep(opt.cut_time * 1000);
        
        printf("Barbero %d ha terminado de cortar el pelo\n", id);
        sem_v(&barberos);
    }
    
    return NULL;
}

void* cliente(void* arg) {
    int id = *(int*)arg;
    
    if (!running) return NULL;
    
    printf("Cliente %d llega a la barbería\n", id);
    
    sem_p(&mutex_sillas);
    if (sillas_ocupadas < capacidad_sala) {
        sillas_ocupadas++;
        printf("Cliente %d se sienta en la sala de espera. Sillas ocupadas: %d/%d\n", 
               id, sillas_ocupadas, capacidad_sala);
        sem_v(&mutex_sillas);
        
        sem_v(&clientes);
        sem_p(&barberos);
        
        printf("Cliente %d se va con el pelo cortado\n", id);
    } else {
        sem_v(&mutex_sillas);
        printf("Cliente %d se va porque no hay sillas disponibles\n", id);
    }
    
    return NULL;
}

void sigint_handler(int sig) {
    printf("\nRecibida señal de terminación. Cerrando la barbería...\n");
    running = false;
}

int main(int argc, char **argv) {
    pthread_t *hilos_barberos, *hilos_clientes;
    int *barbero_ids, *cliente_ids;
    
    opt.barbers = 5;
    opt.customers = 50;
    opt.cut_time = 3000;
    
    read_options(argc, argv, &opt);
    
    capacidad_sala = opt.barbers;
    
    signal(SIGINT, sigint_handler);
    
    sem_init(&clientes, 0);
    sem_init(&barberos, 0);
    sem_init(&mutex_sillas, 1);
    
    hilos_barberos = malloc(opt.barbers * sizeof(pthread_t));
    hilos_clientes = malloc(opt.customers * sizeof(pthread_t));
    barbero_ids = malloc(opt.barbers * sizeof(int));
    cliente_ids = malloc(opt.customers * sizeof(int));
    
    if (!hilos_barberos || !hilos_clientes || !barbero_ids || !cliente_ids) {
        fprintf(stderr, "Error: No se pudo reservar memoria\n");
        exit(1);
    }
    
    printf("Iniciando barbería con %d barberos y %d clientes\n", opt.barbers, opt.customers);
    printf("Tiempo de corte: %d ms\n", opt.cut_time);
    
    for (int i = 0; i < opt.barbers; i++) {
        barbero_ids[i] = i + 1;
        pthread_create(&hilos_barberos[i], NULL, barber, &barbero_ids[i]);
    }
    
    for (int i = 0; i < opt.customers; i++) {
        cliente_ids[i] = i + 1;
        pthread_create(&hilos_clientes[i], NULL, cliente, &cliente_ids[i]);
        usleep(10000); // 10ms entre llegadas de clientes
    }
    
    for (int i = 0; i < opt.customers; i++) {
        pthread_join(hilos_clientes[i], NULL);
    }
    
    running = false;
    
    for (int i = 0; i < opt.barbers; i++) {
        sem_v(&clientes);
    }
    
    for (int i = 0; i < opt.barbers; i++) {
        pthread_join(hilos_barberos[i], NULL);
    }
    
    free(hilos_barberos);
    free(hilos_clientes);
    free(barbero_ids);
    free(cliente_ids);
    
    sem_destroy(&clientes);
    sem_destroy(&barberos);
    sem_destroy(&mutex_sillas);
    
    printf("Barbería cerrada. Todos los clientes han sido atendidos.\n");
    
    return 0;
}
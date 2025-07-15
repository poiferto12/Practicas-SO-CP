//Autores:
//Christian Lema c.lema1@udc.es
//Eliana Reigada e.reigadal@udc.es


#define MAX_NAME 100
#define MAX_FILES 50
#define PATH_MAX 4096
#define TAMANO 2048

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/utsname.h>

// Estructuras
typedef struct tItemL {
    int indice;
    char *nombre;
} tItemL;

typedef struct tNodoL* tPosL;

struct tNodoL {
    tItemL comando;
    tPosL siguiente;
};

typedef tPosL tList;

typedef struct {
    int descriptor;
    char nombre[MAX_NAME];
    char modo[20];
} archivoAbierto;

typedef struct {
    tList listaComandos;
    int indiceComando;
    archivoAbierto archivosAbiertos[MAX_FILES];
    int numArchAbiertos;
} estadoTerminal;

// Funciones de manejo de la lista de comandos
void createEmptyList(tList *lista) {
    *lista = NULL;
}

bool isEmptyList(tList lista) {
    return lista == NULL;
}

tPosL first(tList lista) {
    return lista;
}

tPosL last(tList lista) {
    tPosL ultimoElem;
    for (ultimoElem = lista; ultimoElem->siguiente != NULL; ultimoElem = ultimoElem->siguiente);
    return ultimoElem;
}

void insertItem(tItemL item, tPosL posicion, tList *lista) {
    tPosL nuevoNodo = malloc(sizeof(struct tNodoL));

    if (nuevoNodo == NULL) {
        perror("Error: No se pudo asignar memoria");
        return;
    }

    nuevoNodo->comando = item;
    nuevoNodo->siguiente = NULL;

    if (*lista == NULL) {
        *lista = nuevoNodo;

    } else if (posicion == NULL) {
        tPosL ultimo = last(*lista);
        ultimo->siguiente = nuevoNodo;
    } else {
        nuevoNodo->siguiente = posicion->siguiente;
        posicion->siguiente = nuevoNodo;
    }
}

void freeList(tList *lista) {
    tPosL elemento = *lista;
    tPosL siguiente;

    while (elemento != NULL) {
        siguiente = elemento->siguiente;
        free(elemento->comando.nombre);
        free(elemento);
        elemento = siguiente;
    }
    *lista = NULL;
}

// Funciones de lista de memoria

typedef struct MemBlock {
    void *address;
    size_t size;
    time_t alloc_time;
    char type[10];
    key_t key;
    char filename[256];
    int fd;
    struct MemBlock *next;
} MemBlock;

MemBlock *head = NULL;

void InsertarNodo(MemBlock **head, void *address, size_t size, const char *type, key_t key, const char *filename, int fd) {
    MemBlock *new_node = (MemBlock *)malloc(sizeof(MemBlock));
    new_node->address = address;
    new_node->size = size;
    new_node->alloc_time = time(NULL);
    strcpy(new_node->type, type);
    new_node->key = key;
    if (filename) strcpy(new_node->filename, filename);
    new_node->fd = fd;
    new_node->next = *head;
    *head = new_node;
}

void EliminarNodo(MemBlock **head, void *address) {
    MemBlock *temp = *head, *prev = NULL;
    while (temp != NULL && temp->address != address) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return;
    if (prev == NULL) *head = temp->next;
    else prev->next = temp->next;
    free(temp);
}

void ImprimirLista(MemBlock *head) {
    MemBlock *temp = head;
    while (temp != NULL) {
        printf("Address: %p, Size: %zu, Type: %s, Time: %s", temp->address, temp->size, temp->type, ctime(&temp->alloc_time));
        if (strcmp(temp->type, "shared") == 0) printf(", Key: %d", temp->key);
        if (strcmp(temp->type, "mmap") == 0) printf(", File: %s, FD: %d", temp->filename, temp->fd);
        printf("\n");
        temp = temp->next;
    }
}

void freeAllMemory() {
    MemBlock *current = head;
    while (current != NULL) {
        MemBlock *next = current->next;
        if (strcmp(current->type, "malloc") == 0) {
            free(current->address);
        } else if (strcmp(current->type, "mmap") == 0) {
            munmap(current->address, current->size);
            close(current->fd);
        } else if (strcmp(current->type, "shared") == 0) {
            shmdt(current->address);
        }
        free(current);
        current = next;
    }
    head = NULL;
}

// Funciones de la shell
void imprimirPrompt();
void leerEntrada(char *entrada);
int procesarEntrada(char *entrada, char *trozos[], estadoTerminal *estadoTerminal);
int trocearCadena(char *cadena, char *trozos[]);
void authors(char *trozos[]);
void pid();
void ppid();
void cd(char *trozos[]);
void date(char *trozos[]);
void historic(char *trozos[], estadoTerminal *estadoTerminal);
void cmd_open(char *trozos[], estadoTerminal *estadoTerminal);
void Cmd_close(char *trozos[], estadoTerminal*estadoTerminal);
void Cmd_dup(char *trozos[], estadoTerminal *estadoTerminal);
void infosys();
void help(char *trozos[]);
void listarFicherosAbiertos(estadoTerminal *estadoTerminal);
void añadirAFicherosAbiertos(estadoTerminal *estadoTerminal, int descriptorArch, char *nombreArchivo, int modApert);
void obtenerModoApertura(int modApert, char *modo);

//Funciónes práctica 1
void makefile(char *trozos[]);
void makedir(char *trozos[]);
void listfile(char *trozos[]);
void cwd();
void listdir(char *trozos[]);
void reclist(char *trozos[]);
void revlist(char *trozos[]);
void erase(char *trozos[]);
void delrec(const char *path);
void listarRecursivamente(const char *rutaDir, int hidDir, int longDir, int linkDir, int accDir, int rev);


// Funciones practica 2
void allocate(char *trozos[]);
void deallocate(char *trozos[]);
void memfill(char *trozos[]);
void memdump(char *trozos[]);
void memory(char *trozos[]);
void Cmd_ReadFile(char *trozos[]);
void Recursiva(int n);
void Cmd_WriteFile(char *ar[]);
void Cmd_Read(char *ar[]);
void Cmd_Write(char *ar[]);
void do_AllocateMmap(char *arg[]);
void do_DeallocateDelkey(char *args[]);



// Función principal
int main() {
    char entrada[512];
    char *trozos[64];
    int continuar = 1;

    // Inicializa el estado de la shell
    estadoTerminal estadoTerminal;
    estadoTerminal.indiceComando = 1;
    estadoTerminal.numArchAbiertos = 0;
    createEmptyList(&estadoTerminal.listaComandos);

    //Procesa la entrada
    while (continuar) {
        imprimirPrompt();
        leerEntrada(entrada);
        if (procesarEntrada(entrada, trozos, &estadoTerminal) == 0) {
            continuar = 0;
        }
    }

    // Liberar memoria del historial de comandos
    freeList(&estadoTerminal.listaComandos);

    // Liberar toda la memoria asignada
    freeAllMemory();

    return 0;
}

void imprimirPrompt() {
    printf("$: ");
}

void leerEntrada(char *entrada) {
    if (fgets(entrada, 512, stdin) == NULL) {
        perror("Error al leer la entrada");
    }
}

int procesarEntrada(char *entrada, char *trozos[], estadoTerminal *estadoTerminal) {
    entrada[strcspn(entrada, "\n")] = 0; 
    int num_trozos = trocearCadena(entrada, trozos);

    if (num_trozos == 0) {
        return 1;
    }

    char nombreComando[64]; 
    nombreComando[0] = '\0'; 

    for (int i = 0; i < num_trozos; i++) {
        strcat(nombreComando, trozos[i]); 
        if (i < num_trozos - 1) {
            strcat(nombreComando, " "); 
        }
    }

    // Almacenar el comando en el historial
    tItemL nuevoComando;
    nuevoComando.indice = estadoTerminal->indiceComando++;
    nuevoComando.nombre = strdup(nombreComando);
    insertItem(nuevoComando, NULL, &estadoTerminal->listaComandos);

    // Procesar los comandos
    if (strcmp(trozos[0], "exit") == 0 || strcmp(trozos[0], "quit") == 0 || strcmp(trozos[0], "bye") == 0) {
        return 0;
    } else if (strcmp(trozos[0], "authors") == 0) {
        authors(trozos);
    } else if (strcmp(trozos[0], "pid") == 0) {
        pid();
    } else if (strcmp(trozos[0], "ppid") == 0) {
        ppid();
    } else if (strcmp(trozos[0], "cd") == 0) {
        cd(trozos);
    } else if (strcmp(trozos[0], "date") == 0) {
        date(trozos);
    } else if (strcmp(trozos[0], "historic") == 0) {
        historic(trozos, estadoTerminal);
    } else if (strcmp(trozos[0], "open") == 0) {
        cmd_open(trozos, estadoTerminal);
    } else if (strcmp(trozos[0], "close") == 0) {
        Cmd_close(trozos, estadoTerminal);
    } else if (strcmp(trozos[0], "dup") == 0) {
        Cmd_dup(trozos, estadoTerminal);
    } else if (strcmp(trozos[0], "infosys") == 0) {
        infosys();
    } else if (strcmp(trozos[0], "help") == 0) {
        help(trozos);
    } else if (strcmp(trozos[0], "makefile") == 0) {
        makefile(trozos);
    } else if (strcmp(trozos[0], "makedir") == 0) {
        makedir(trozos);
    } else if (strcmp(trozos[0], "listfile") == 0) {
        listfile(trozos);
    } else if (strcmp(trozos[0], "cwd") == 0) {
        cwd();
    } else if (strcmp(trozos[0], "listdir") == 0) {
        listdir(trozos);
    } else if (strcmp(trozos[0], "reclist") == 0) {
        reclist(trozos);
    } else if (strcmp(trozos[0], "revlist") == 0) {
        revlist(trozos);
    } else if (strcmp(trozos[0], "erase") == 0) {
        erase(trozos);
    } else if (strcmp(trozos[0], "delrec") == 0) {
        delrec(trozos[1]);
    } else if (strcmp(trozos[0], "allocate") == 0) {
        allocate(trozos);
    } else if (strcmp(trozos[0], "deallocate") == 0) {
        deallocate(trozos);
    } else if (strcmp(trozos[0], "memfill") == 0) {
        memfill(trozos);
    } else if (strcmp(trozos[0], "memdump") == 0) {
        memdump(trozos);
    } else if (strcmp(trozos[0], "memory") == 0) {
        memory(trozos);
    } else if (strcmp(trozos[0], "readfile") == 0) {
        Cmd_ReadFile(trozos);
    } else if (strcmp(trozos[0], "recurse") == 0) {
        Recursiva(atoi(trozos[1]));
    } else if (strcmp(trozos[0], "writefile") == 0) {
        Cmd_WriteFile(trozos);
    } else if (strcmp(trozos[0], "read") == 0) {
        Cmd_Read(trozos);
    } else if (strcmp(trozos[0], "write") == 0) {
        Cmd_Write(trozos);
    } else {
        perror("Comando no reconocido\n");
    }

    return 1;
}

int trocearCadena(char *cadena, char *trozos[]) {
    int i = 1;

    if ((trozos[0] = strtok(cadena, " \n\t")) == NULL)
        return 0;
    while ((trozos[i] = strtok(NULL, " \n\t")) != NULL)
        i++;
    return i;
}

void authors(char *trozos[]) {
    //Si se escribre authors a secas imprime logins y nombre de usuario
    if (trozos[1] == NULL) {
        printf("Eliana, e.reigadal@udc.es\nChristian, c.lema1@udc.es\n");
    } 
    //Si se escribe authors -l imprime solo logins
    else if (strcmp(trozos[1], "-l") == 0) {
        printf("e.reigadal@udc.es, c.lema1@udc.es\n");
    } 
    //Si se escribe authors -n imprime solo los nombres
    else if (strcmp(trozos[1], "-n") == 0) {
        printf("Eliana, Christian\n");
    }
}

void pid() {
    __pid_t pid = getpid();
    printf("PID:%d\n", pid);
}

void ppid() {
    __pid_t ppid = getppid();
    printf("PPID:%d\n", ppid);
}

void cd(char *trozos[]) {
    // Si se recibe cd a secas se imprime el directorio actual
    if (trozos[1] == NULL) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("El directorio actual es: %s\n", cwd);
        } else {
            perror("Error al obtener el directorio actual");
        }

    // Sino cambia el directorio a la dirección que le pases
    } else {
        if (chdir(trozos[1]) != 0) {
            perror("Error al cambiar de directorio");
        } else {
            printf("Se ha cambiado el directorio correctamente\n");
        }
    }
}

void date(char *trozos[]) {
    //Si se recibe date se imprime la hora y la fecha
    if (trozos[1] == NULL) {
        system("date +\"%d-%m-%Y\"");
        system("date +\"%H:%M:%S\"");

    //Si se recibe date -d se imprime solo la fecha
    } else if (strcmp(trozos[1], "-d") == 0) {
        system("date +\"%d-%m-%Y\"");

    //Si se recibe date -t se imprime solo la hora
    } else if (strcmp(trozos[1], "-t") == 0) {
        system("date +\"%H:%M:%S\"");
    }
}

void historic(char *trozos[], estadoTerminal *estadoTerminal) {
    tPosL nodo = estadoTerminal->listaComandos;
    // Si no se pasa ningún argumento, mostramos todo el historial
    if (trozos[1] == NULL) {
        while (nodo != NULL) {
            printf("%d: %s\n", nodo->comando.indice, nodo->comando.nombre);
            nodo = nodo->siguiente;
        }
    }
    // Si se pasa "historic -N"  mostramos los últimos N comandos
    else if (trozos[1][0] == '-') {

        // Convertir el numero "-N" en un número entero ignorando el guion
        int N = atoi(&trozos[1][1]);
        if (N <= 0) {  // Comprobar que N es positivo
            printf("El número de comandos debe ser positivo.\n");
            return;
        }

        // Contar el número de comandos en la lista
        int totalComandos = 0;
        tPosL aux = estadoTerminal->listaComandos;
        while (aux != NULL) {
            totalComandos++;
            aux = aux->siguiente;
        }

        // Si encontramos menos comandos de los que pedimos, ajustamos N
        if (N > totalComandos) {
            N = totalComandos;
        }

        // Cambiamos el puntero de la lista al primer comando que queremos mostrar
        int skip = totalComandos - N;
        for (int i = 0; i < skip; i++) {
            nodo = nodo->siguiente;
        }

        // Imprimimos los últimos N comandos 
        while (nodo != NULL) {
            printf("%d: %s\n", nodo->comando.indice, nodo->comando.nombre);
            nodo = nodo->siguiente;
        }
    }
    // Si se pasa "historic N" mostramos y ejecutamos comando numero N
    else {
        int N = atoi(trozos[1]);
        if (N <= 0) {  // Comprobar que N es positivo
            printf("El número de comandos debe ser positivo.\n");
            return;
        }

        // Buscar el comando en la lista
        tPosL aux = estadoTerminal->listaComandos;
        while (aux != NULL && aux->comando.indice != N) {
            aux = aux->siguiente;
        }

        if (aux == NULL) {
            printf("No se encontró el comando número %d.\n", N);
            return;
        }

        // Mostrar el comando
        printf("%d: %s\n", aux->comando.indice, aux->comando.nombre);

        // Evitar bucle infinito al ejecutar otro "historic N"
        if (strncmp(aux->comando.nombre, "historic", 8) != 0){
            // Procesar el comando encontrado
            char *trozosComando[32];
            int num_trozos = trocearCadena(aux->comando.nombre, trozosComando);
            if (num_trozos > 0) {
                procesarEntrada(aux->comando.nombre, trozosComando, estadoTerminal);
            }
        } else {
            printf("Ejecución de 'historic' evitada para prevenir bucles infinitos.\n");
        }
    }
}

void infosys() {
    struct utsname buffer;
    uname(&buffer); //Almacena la información en el buffer
    
    //Imprime los datos almacenados en el buffer
    printf("Sistema operativo: %s\n", buffer.sysname);
    printf("Red del host: %s\n", buffer.nodename);
    printf("Versión actual del kernel: %s\n", buffer.release);
    printf("Versión completa del sistema: %s\n", buffer.version);
    printf("Arquitectura hardware: %s\n", buffer.machine);
}

void help(char *trozos[]) {
   //Si se recibe help a secas, se imprimen el nombre de los comandos que puedes utilizar
    if (trozos[1]==NULL){
         printf("Comandos disponibles: authors, pid, ppid, cd, date, historic, open, close, dup, infosys, help\n");
    }
    //Si se recibe help seguido del nombre de un comando, se imprime información específica de ese comando
    else {
        if (strcmp(trozos[1],"authors") == 0){
            printf("authors [-n|-l]	Muestra los nombres y/o logins de los autores\n");
        }else if (strcmp(trozos[1],"pid") == 0){
            printf("pid [-p]	Muestra el pid del shell o de su proceso padre\n");
        }else if (strcmp(trozos[1],"ppid") == 0){
            printf("ppid 	Muestra el pid del proceso padre del shell\n");
        }else if (strcmp(trozos[1],"cd") == 0){
            printf("cd [dir]	Cambia (o muestra) el directorio actual del shell\n");
        }else if (strcmp(trozos[1],"date") == 0){
            printf("date [-d|-t]	Muestra la fecha y/o la hora actual\n");
        }else if (strcmp(trozos[1],"historic") == 0){
            printf("historic [-c|-N|N]	Muestra (o borra)el historico de comandos\n");
            printf("\t-N: muestra los N primeros\n");
            printf("\t-c: borra el historico\n");
            printf("N: repite el comando N\n");
        }else if (strcmp(trozos[1],"open") == 0){
            printf("open fich m1 m2...	Abre el fichero fich\n");
            printf("\ty lo anade a la lista de ficheros abiertos del shell\n");
            printf("\tm1, m2..es el modo de apertura (or bit a bit de los siguientes)\n");
            printf("\tcr: O_CREAT	ap: O_APPEND\n");
            printf("\tex: O_EXCL 	ro: O_RDONLY\n");
            printf("\trw: O_RDWR 	wo: O_WRONLY\n");
            printf("\ttr: O_TRUNC\n");
        }else if (strcmp(trozos[1],"close") == 0){
            printf("close [df]: Cierra el descriptor de archivo df y elimina el elemento correspondiente de la lista.\n");
        }else if (strcmp(trozos[1],"dup") == 0){
            printf("dup [df]: Duplica el descriptor de archivo df , creando la correspondiente nueva entrada en la lista de archivos.\n");
        }else if (strcmp(trozos[1],"infosys") == 0){
            printf("infosys : Imprime información sobre la máquina que ejecuta la shell\n");
        }else if (strcmp(trozos[1],"help") == 0){
            printf("help :Muestra ayuda sobre los comandos\n");
        }else if (strcmp(trozos[1],"quit") == 0){
            printf("quit, exit, bye: Terminan la shell.\n");
        }else if (strcmp(trozos[1],"exit") == 0){
            printf("quit, exit, bye: Terminan la shell.\n");
        }else if (strcmp(trozos[1],"bye") == 0){
            printf("quit, exit, bye: Terminan la shell.\n");
        }else if (strcmp(trozos[1],"makefile") == 0){
            printf("makefile [name]	Crea un fichero de nombre name\n");
        }else if (strcmp(trozos[1],"makedir") == 0){
            printf("makedir [name]	Crea un directorio de nombre name\n");
        }else if (strcmp(trozos[1],"listfile") == 0){
            printf("listfile [-long][-link][-acc] name1 name2 ..	lista ficheros;\n");
	        printf("\t-long: listado largo\n");
	        printf("\t-acc: acesstime\n");
	        printf("\t-link: si es enlace simbolico, el path contenido\n");
        }else if (strcmp(trozos[1],"cwd") == 0){
            printf("cwd 	Muestra el directorio actual del shell\n");
        }else if (strcmp(trozos[1],"listdir") == 0){
            printf("listdir [-hid][-long][-link][-acc] n1 n2 ..	lista contenidos de directorios\n");
            printf("\t-long: listado largo\n");
            printf("\t-hid: incluye los ficheros ocultos\n");
            printf("\t-acc: acesstime\n");
            printf("\t-link: si es enlace simbolico, el path contenido\n");
        }else if (strcmp(trozos[1],"reclist") == 0){
            printf("reclist[-hid][-long][-link][-acc] n1 n2 ..	lista recursivamente contenidos de directorios(subdirs despues)\n");
            printf("\t-hid: incluye los ficheros ocultos\n");
            printf("\t-long: listado largo\n");
            printf("\t-acc: acesstime\n");
            printf("\t-link: si es enlace simbolico, el path contenido\n");
        }else if (strcmp(trozos[1],"revlist") == 0){
            printf("revlist[-hid][-long][-link][-acc] n1 n2 ..	lista recursivamente contenidos de directorios(subdirs antes)\n");
            printf("\t-hid: incluye los ficheros ocultos\n");
            printf("\t-long: listado largo\n");
            printf("\t-acc: acesstime\n");
            printf("\t-link: si es enlace simbolico, el path contenido\n");
        }else if (strcmp(trozos[1],"erase") == 0){
            printf("erase [name1 name2 ..]	Borra ficheros o directorios vacios\n");
        }else if (strcmp(trozos[1],"delrec") == 0){
            printf("delrec [name1 name2 ..]	Borra ficheros o directorios no vacios recursivamente\n");
        }else if (strcmp(trozos[1], "malloc") == 0) {
            printf("malloc [-free] [tam]\tasigna un bloque memoria de tamano tam con malloc\n");
            printf("\t-free: desasigna un bloque de memoria de tamano tam asignado con malloc\n");
        }else if (strcmp(trozos[1], "allocate") == 0) {
            printf("allocate [-malloc|-shared|-createshared|-mmap]... Asigna un bloque de memoria\n");
            printf("\t-malloc tam: asigna un bloque malloc de tamano tam\n");
            printf("\t-createshared cl tam: asigna (creando) el bloque de memoria compartida de clave cl y tamano tam\n");
            printf("\t-shared cl: asigna el bloque de memoria compartida (ya existente) de clave cl\n");
            printf("\t-mmap fich perm: mapea el fichero fich, perm son los permisos\n");
        }else if (strcmp(trozos[1], "mmap") == 0) {
            printf("mmap [-free] fich prm\tmapea el fichero fich con permisos prm\n");
            printf("\t-free fich: desmapea el fichero fich\n");
        } else if (strcmp(trozos[1], "shared") == 0) {
            printf("shared [-free|-create|-delkey] cl [tam]\tasigna memoria compartida con clave cl en el programa\n");
            printf("\t-create cl tam: asigna (creando) el bloque de memoria compartida de clave cl y tamano tam\n");
            printf("\t-free cl: desmapea el bloque de memoria compartida de clave cl\n");
            printf("\t-delkey cl: elimina del sistema (sin desmapear) la clave de memoria cl\n");
        } else if (strcmp(trozos[1], "deallocate") == 0) {
            printf("deallocate [-malloc|-shared|-delkey|-mmap|addr].. Desasigna un bloque de memoria\n");
            printf("\t-malloc tam: desasigna el bloque malloc de tamano tam\n");
            printf("\t-shared cl: desasigna (desmapea) el bloque de memoria compartida de clave cl\n");
            printf("\t-delkey cl: elimina del sistema (sin desmapear) la clave de memoria cl\n");
            printf("\t-mmap fich: desmapea el fichero mapeado fich\n");
            printf("\taddr: desasigna el bloque de memoria en la direccion addr\n");
        } else if (strcmp(trozos[1], "delkey") == 0) {
            printf("delkey cl\telimina del sistema (sin desmapear) la clave de memoria cl\n");
        } else if (strcmp(trozos[1], "addr") == 0) {
            printf("addr no encontrado\n");
        } else if (strcmp(trozos[1], "memfill") == 0) {
            printf("memfill addr cont byte\tLlena la memoria a partir de addr con byte\n");
        } else if (strcmp(trozos[1], "memdump") == 0) {
            printf("memdump addr cont\tVuelca en pantallas los contenidos (cont bytes) de la posicion de memoria addr\n");
        } else if (strcmp(trozos[1], "memory") == 0) {
            printf("memory [-blocks|-funcs|-vars|-all|-pmap] ..\tMuestra muestra detalles de la memoria del proceso\n");
            printf("\t-blocks: los bloques de memoria asignados\n");
            printf("\t-funcs: las direcciones de las funciones\n");
            printf("\t-vars: las direcciones de las variables\n");
            printf("\t-all: todo\n");
            printf("\t-pmap: muestra la salida del comando pmap(o similar)\n");
        } else if (strcmp(trozos[1], "readfile") == 0) {
            printf("readfile fiche addr cont\tLee cont bytes desde fich a la direccion addr\n");
        } else if (strcmp(trozos[1], "writefile") == 0) {
            printf("writefile [-o] fiche addr cont\tEscribe cont bytes desde la direccion addr a fich (-o sobreescribe)\n");
        } else if (strcmp(trozos[1], "read") == 0) {
            printf("read df addr cont\tTransfiere cont bytes del fichero descrito por df a la dirección addr\n");
        } else if (strcmp(trozos[1], "write") == 0) {
            printf("write df addr cont\tTransfiere cont bytes desde la dirección addr al fichero descrito por df\n");
        } else if (strcmp(trozos[1], "recurse") == 0) {
            printf("recurse [n]\tInvoca a la funcion recursiva n veces\n");
        } else {
            printf("Comando no encontrado\n");
        }
    }
}

void obtenerModoApertura(int modApert, char *modo) {
    //Inicializa la cadena modo como una cadena vacía
    strcpy(modo, "");

    // Procesa los modos de apertura uno por uno
    if (modApert & O_CREAT) strcat(modo, "cr ");
    if (modApert & O_EXCL) strcat(modo, "ex ");
    if (modApert & O_RDONLY) strcat(modo, "ro ");
    if (modApert & O_WRONLY) strcat(modo, "wo ");
    if (modApert & O_RDWR) strcat(modo, "rw ");
    if (modApert & O_APPEND) strcat(modo, "ap ");
    if (modApert & O_TRUNC) strcat(modo, "tr ");

    // Elimina el último espacio
    if (strlen(modo) > 0) {
        modo[strlen(modo) - 1] = '\0';
    }
}

void listarFicherosAbiertos(estadoTerminal *estadoTerminal) {
    //Comprueba que no hay archivos abiertos y imprime un aviso en caso de que no los haya
    if (estadoTerminal->numArchAbiertos == 0) {
        printf("No hay archivos abiertos.\n");
        return;
    }
    //En caso de que si existan imprime la lista de archivos abiertos
    printf("Archivos abiertos:\n");
    for (int i = 0; i < estadoTerminal->numArchAbiertos; i++) {
        printf("Descriptor: %d | Archivo: %s | Modo: %s\n",
               estadoTerminal->archivosAbiertos[i].descriptor,
               estadoTerminal->archivosAbiertos[i].nombre,
               estadoTerminal->archivosAbiertos[i].modo);
    }
}

void añadirAFicherosAbiertos(estadoTerminal *estadoTerminal, int descriptorArch, char *nombreArchivo, int modApert) {
    //Comprueba si se ha alcanzado el límite de archivos abiertos
    if (estadoTerminal->numArchAbiertos >= MAX_FILES){
        printf("Se ha alcanzado el límite de archivos abiertos\n");
        return;
    }
    //Se inicializa una variable nueva donde se va a almacenar el nuevo archivo
    archivoAbierto nuevoArchivo;
    nuevoArchivo.descriptor = descriptorArch;
    strncpy(nuevoArchivo.nombre, nombreArchivo, MAX_NAME - 1);
    nuevoArchivo.nombre[MAX_NAME - 1] = '\0';

    // Convertir los modos de apertura en formato legible
    obtenerModoApertura(modApert, nuevoArchivo.modo);

    //Se añade el nuevo archivo a la lista 
    estadoTerminal->archivosAbiertos[estadoTerminal->numArchAbiertos] = nuevoArchivo;
    estadoTerminal->numArchAbiertos++;
}

char* NombreFicheroDescriptor(int descriptorArch, estadoTerminal *estadoTerminal) {
    //Recorre toda la lista de archivos abiertos hasta encontrar el descriptor
    for (int i = 0; i < estadoTerminal->numArchAbiertos; i++) {
        if (estadoTerminal->archivosAbiertos[i].descriptor == descriptorArch) {
            // Y retorna el nombre del archivo
            return estadoTerminal->archivosAbiertos[i].nombre; 
        }
    }
    // Si no se encuentra el descriptor, retorna NULL
    return NULL; 
}

void cmd_open(char *trozos[], estadoTerminal *estadoTerminal) {

    int i, descriptorArchivo, mode = 0;

    //Si recibe open a secas, imprime una lista de los ficheros abiertos
    if (trozos[1] == NULL) { 
        listarFicherosAbiertos(estadoTerminal);
        return;
    }

    //Sino, comprueba que modo ha recibido para abrir el archivo
    for (i = 2; trozos[i] != NULL; i++) {
        if (!strcmp(trozos[i], "cr")) mode |= O_CREAT;
        else if (!strcmp(trozos[i], "ex")) mode |= O_EXCL;
        else if (!strcmp(trozos[i], "ro")) mode |= O_RDONLY;
        else if (!strcmp(trozos[i], "wo")) mode |= O_WRONLY;
        else if (!strcmp(trozos[i], "rw")) mode |= O_RDWR;
        else if (!strcmp(trozos[i], "ap")) mode |= O_APPEND;
        else if (!strcmp(trozos[i], "tr")) mode |= O_TRUNC;
        else break;
    }

    //Comprueba que sea posible abrir el archivo segun el modo
    if ((descriptorArchivo = open(trozos[1], mode, 0777)) == -1) {
        perror("Imposible abrir fichero");
    } 
    
    //Y si es posible añade el archivo a la lista de ficheros abiertos
    else {
        añadirAFicherosAbiertos(estadoTerminal, descriptorArchivo, trozos[1], mode);
        printf("Añadida entrada a la tabla de ficheros abiertos: %s\n", trozos[1]);
    }
}

void Cmd_close(char *trozos[], estadoTerminal *estadoTerminal) {

    int descriptorArchivos;

    // Verifica si hay un argumento y que sea un número válido
    if (trozos[1] == NULL || (descriptorArchivos = atoi(trozos[1])) < 0) {
        listarFicherosAbiertos(estadoTerminal); 
        return;
    }

    // Verifica que el descriptor no sea uno de los estándares
    if (descriptorArchivos == 0 || descriptorArchivos == 1 || descriptorArchivos == 2) {
        perror("Error");
        printf("No se puede cerrar el descriptor %d (stdin/stdout/stderr).\n", descriptorArchivos);
        return;
    }

    // Intenta cerrar el descriptor
    if (close(descriptorArchivos) == -1) {
        perror("Imposible cerrar descriptor");
    } else {
        // Elimina el archivo de la lista
        for (int i = 0; i < estadoTerminal->numArchAbiertos; i++) {
            if (estadoTerminal->archivosAbiertos[i].descriptor == descriptorArchivos) {
                // Usar memmove para mover los elementos restantes hacia la izquierda
                memmove(&estadoTerminal->archivosAbiertos[i], 
                        &estadoTerminal->archivosAbiertos[i + 1], 
                        (estadoTerminal->numArchAbiertos - i - 1) * sizeof(archivoAbierto));
                estadoTerminal->numArchAbiertos--; // Reducir el número de archivos abiertos
                break;  // Salir del bucle después de cerrar el archivo
            }
        }
        printf("Descriptor cerrado: %d\n", descriptorArchivos);
    }
}

void Cmd_dup(char *trozos[], estadoTerminal *estadoTerminal) {
    int descriptor, duplicado;
    char nombre[MAX_NAME], *pnombre;

    // Comprueba que el argumento sea un número válido  
    if (trozos[1] == NULL || (descriptor = atoi(trozos[1])) < 0) {
        listarFicherosAbiertos(estadoTerminal);
        return;
    }

    // Verifica que el descriptor no sea uno de los estándares
    if (descriptor == 0 || descriptor == 1 || descriptor == 2) {
        perror("Error");
        printf("No se puede duplicar el descriptor %d (stdin/stdout/stderr).\n", descriptor);
        return;
    }

    //Duplica el descriptor
    duplicado = dup(descriptor);
    if (duplicado == -1) {
        perror("Imposible duplicar descriptor");
        return;
    }

    // Buscar el nombre del archivo asociado al descriptor original
    pnombre = NombreFicheroDescriptor(descriptor, estadoTerminal);
    if (pnombre == NULL) {
        printf("No se encontró el archivo asociado al descriptor %d\n", descriptor);
        close(duplicado);  // Cerrar el descriptor duplicado si no se encontró el original
        return;
    }

    //Construye el nombre del descriptor duplicado
    snprintf(nombre, MAX_NAME, "dup %d (%s)", descriptor, pnombre);

    //Se añade a la lista de ficheros abiertos
    añadirAFicherosAbiertos(estadoTerminal, duplicado,nombre, fcntl(duplicado,F_GETFL));
    printf("Descriptor duplicado: %d -> %d\n", descriptor, duplicado);
}

void makefile(char *trozos[]){
    //Crea un archivo con los permisos especificados
    int archivoAbierto = open(trozos[1], O_CREAT | O_EXCL |O_WRONLY, 0644); 
    if (archivoAbierto== -1) {
        perror("Error al crear el archivo");
        return;
    }
    printf("Archivo %s creado con éxito\n", trozos[1]);
}

void makedir(char *trozos[]){
    //Crea un directorio con los permisos especificados
    if (mkdir(trozos[1], 0755) == -1) {
        perror("Error al crear el directorio");
    } else {
        printf("Directorio %s creado\n", trozos[1]);
    }
}
char LetraTF(mode_t m)
{
    switch (m & __S_IFMT)
    { /*and bit a bit con los bits de formato,0170000 */
    case __S_IFSOCK:
        return 's'; /*socket */
    case __S_IFLNK:
        return 'l'; /*symbolic link*/
    case __S_IFREG:
        return '-'; /* fichero normal*/
    case __S_IFBLK:
        return 'b'; /*block device*/
    case __S_IFDIR:
        return 'd'; /*directorio */
    case __S_IFCHR:
        return 'c'; /*char device*/
    case __S_IFIFO:
        return 'p'; /*pipe*/
    default:
        return '?'; /*desconocido, no deberia aparecer*/
    }
}

const char *ConvierteModo2(mode_t m)
{
    static char permisos[12];
    strcpy(permisos, "---------- ");

    permisos[0] = LetraTF(m);
    if (m & S_IRUSR)
        permisos[1] = 'r'; /*propietario*/
    if (m & S_IWUSR)
        permisos[2] = 'w';
    if (m & S_IXUSR)
        permisos[3] = 'x';
    if (m & S_IRGRP)
        permisos[4] = 'r'; /*grupo*/
    if (m & S_IWGRP)
        permisos[5] = 'w';
    if (m & S_IXGRP)
        permisos[6] = 'x';
    if (m & S_IROTH)
        permisos[7] = 'r'; /*resto*/
    if (m & S_IWOTH)
        permisos[8] = 'w';
    if (m & S_IXOTH)
        permisos[9] = 'x';
    if (m & S_ISUID)
        permisos[3] = 's'; /*setuid, setgid y stickybit*/
    if (m & S_ISGID)
        permisos[6] = 's';
    if (m & __S_ISVTX)
        permisos[9] = 't';
    return permisos;
}

void cwd(){
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) !=NULL){
        printf("El directorio actual es: %s\n",cwd);
    }else{
        perror("Error al obtener el directorio actual");
    }
}

void listfile(char *trozos[]){
    int longFile= 0, linkFile= 0, accFile= 0;
    int i =1;

    // Opciones
    while (trozos[i] != NULL && trozos[i][0] == '-'){
        if (strcmp(trozos[i],"-long") == 0) {
            longFile = 1;
        }else if (strcmp(trozos[i], "-link") == 0){
            linkFile = 1;
        }else if (strcmp(trozos[i],"-acc") == 0){
            accFile = 1;
        }else{
            printf("Opción no reconocida: %s\n", trozos[i]);
            return;
        }
        i++;
    }

    // Bucle para acceder a cada archivo
    for (;trozos[i]!= NULL;i++){
        struct stat fileStat;
        struct passwd *pw;
        struct group *gr;
        char tiempoStr[20];
        char linkTarget[PATH_MAX];
        ssize_t linkLength;

        if (lstat(trozos[i],&fileStat) ==-1){
            perror("Error al obtener información del archivo");
            continue;
        }

        // Salida normal
        if(!longFile && !linkFile && !accFile){
            printf("%ld %s\n", fileStat.st_size,trozos[i]);
            continue;
        }

        // Salida de -long
        if (longFile){
            pw = getpwuid(fileStat.st_uid);
            gr =getgrgid(fileStat.st_gid);
            strftime(tiempoStr, sizeof(tiempoStr), "%Y/%m/%d-%H:%M",localtime(&fileStat.st_mtime));
            printf("%s %ld (%ld) %s %s %s %ld %s\n",
                   tiempoStr,
                   fileStat.st_nlink,
                   fileStat.st_ino,
                   pw ? pw->pw_name: "unknown",
                   gr ? gr->gr_name :"unknown",
                   ConvierteModo2(fileStat.st_mode),
                   fileStat.st_size,
                   trozos[i]);
        }

        // Salida de -link
        if (linkFile){
            if(S_ISLNK(fileStat.st_mode)){
                linkLength = readlink(trozos[i], linkTarget, sizeof(linkTarget) - 1);
                if (linkLength != -1){
                    linkTarget[linkLength] = '\0';
                    printf("%ld %s -> %s\n", fileStat.st_size, trozos[i], linkTarget);
                }else{
                    perror("Error al leer el enlace simbólico");
                }
            }else{
                printf("%ld %s\n", fileStat.st_size, trozos[i]);
            }
        }

        // Salida de -acc
        if (accFile){
            strftime(tiempoStr, sizeof(tiempoStr), "%Y/%m/%d-%H:%M",localtime(&fileStat.st_atime));
            printf("Último acceso: %s\n",tiempoStr);
        }
    }
}

void listarDirectorio(const char *nombreDir, int hidDir, int longDir, int linkDir, int accDir){
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    struct passwd *pw;
    struct group *gr;
    char tiempoStr[20];
    char linkTarget[PATH_MAX];
    ssize_t linkLen;
    int unicoLink = 0;

    // Abrir el directorio
    dir = opendir(nombreDir);
    if (dir == NULL) {
        perror("Error al abrir el directorio");
        return;
    }

    // Usar lstat si linkDir está activado, de lo contrario usar stat
    if (linkDir) {
        if (lstat(nombreDir, &fileStat) == -1){
            perror("Error al obtener información del archivo");
        }
    } else {
        if (stat(nombreDir, &fileStat) == -1){
            perror("Error al obtener información del archivo");
        }
    }

    while ((entry = readdir(dir)) != NULL){
        // Omitir archivos ocultos si no se especifica -hid
        if (!hidDir && entry->d_name[0] == '.'){
            continue;
        }

        // Construir la ruta completa del archivo
        char filePath[PATH_MAX];
        snprintf(filePath, sizeof(filePath), "%s/%s", nombreDir, entry->d_name);

        

        // Salida normal
        if (!longDir && !linkDir && !accDir) {
            printf("    %8ld  %s\n", fileStat.st_size, entry->d_name);
            continue;
        }

        // Salida de -link
            if (linkDir && !unicoLink){ {
                if (S_ISLNK(fileStat.st_mode)){
                    linkLen = readlink(nombreDir, linkTarget, sizeof(linkTarget) - 1);
                    if (linkLen != -1) {
                        linkTarget[linkLen] = '\0';
                        printf("    %8ld  %s -> %s\n", fileStat.st_size, nombreDir, linkTarget);
                        unicoLink = 1;
                    } else {
                        perror("Error al leer el enlace simbólico");
                    }
                } else {
                    printf("    %8ld  %s\n", fileStat.st_size, entry->d_name);
                }
            }
        }

        // Salida de -long
        if (longDir){
            pw = getpwuid(fileStat.st_uid);
            gr = getgrgid(fileStat.st_gid);
            strftime(tiempoStr, sizeof(tiempoStr), "%Y/%m/%d-%H:%M", localtime(&fileStat.st_mtime));
            printf("%s %ld (%ld) %s %s %s %ld %s\n",
                   tiempoStr,
                   fileStat.st_nlink,
                   fileStat.st_ino,
                   pw ? pw->pw_name : "desconocido",
                   gr ? gr->gr_name : "desconocido",
                   ConvierteModo2(fileStat.st_mode),
                   fileStat.st_size,
                   entry->d_name);
        }

        // Salida de -acc
        if (accDir) {
            strftime(tiempoStr, sizeof(tiempoStr), "%Y/%m/%d-%H:%M", localtime(&fileStat.st_atime));
            printf("Último acceso: %s %s\n", tiempoStr, entry->d_name);
        }
    }
    unicoLink = 0;

    closedir(dir);
}

void listdir(char *trozos[]){
    int longDir = 0, linkDir = 0, accDir = 0, hidDir = 0;
    int i = 1;

    // Opciones
    while (trozos[i] != NULL && trozos[i][0] == '-'){
        if(strcmp(trozos[i], "-long") == 0){
            longDir = 1;
        }else if(strcmp(trozos[i], "-link") == 0){
            linkDir = 1;
        }else if(strcmp(trozos[i], "-acc") == 0){
            accDir = 1;
        }else if(strcmp(trozos[i], "-hid") == 0){
            hidDir = 1;
        }else{
            printf("Opción no reconocida: %s\n", trozos[i]);
            return;
        }
        i++;
    }

    // Bucle para acceder a cada directorio
    for (; trozos[i] != NULL; i++){
        listarDirectorio(trozos[i], hidDir, longDir, linkDir, accDir);
    }
}

void reclist(char *trozos[]){
    int hidDir = 0, longDir = 0, linkDir = 0, accDir = 0;
    int i = 1;

    // Opciones
    while (trozos[i] != NULL && trozos[i][0] == '-'){
        if(strcmp(trozos[i], "-hid") == 0){
            hidDir = 1;
        }else if(strcmp(trozos[i], "-long") == 0){
            longDir = 1;
        }else if(strcmp(trozos[i], "-link") == 0){
            linkDir = 1;
        }else if(strcmp(trozos[i], "-acc") == 0){
            accDir = 1;
        }else{
            printf("Opción no reconocida: %s\n", trozos[i]);
            return;
        }
        i++;
    }

    // Bucle para acceder a cada directorio
    for (; trozos[i] != NULL; i++){
        listarRecursivamente(trozos[i], hidDir, longDir, linkDir, accDir, 0);
    }
}

void revlist(char *trozos[]){
    int hidDir = 0, longDir = 0, linkDir = 0, accDir = 0;
    int i = 1;

    // Opciones
    while (trozos[i] != NULL && trozos[i][0] == '-'){
        if(strcmp(trozos[i], "-hid") == 0) {
            hidDir = 1;
        }else if(strcmp(trozos[i], "-long") == 0){
            longDir = 1;
        }else if(strcmp(trozos[i], "-link") == 0){
            linkDir = 1;
        }else if(strcmp(trozos[i], "-acc") == 0){
            accDir = 1;
        }else{
            printf("Opción no reconocida: %s\n", trozos[i]);
            return;
        }
        i++;
    }

    // Bucle para acceder a cada directorio
    for (; trozos[i] != NULL; i++) {
        listarRecursivamente(trozos[i], hidDir, longDir, linkDir, accDir, 1);
    }
}

void listarRecursivamente(const char *rutaDir, int hidDir, int longDir, int linkDir, int accDir, int rev){
    DIR *dir;
    struct dirent *entrada;
    struct stat fileStat;
    char rutaFile[PATH_MAX];

    if ((dir = opendir(rutaDir)) == NULL){
        perror("Error al abrir el directorio");
        return;
    }

    if (rev){
        // Primero listar todos los subdirectorios
        while ((entrada = readdir(dir)) != NULL) {
            // Omitir archivos ocultos si no se especifica -hid
            if (!hidDir && entrada->d_name[0] == '.'){
                continue;
            }

            // Construir la ruta completa del archivo
            snprintf(rutaFile, sizeof(rutaFile), "%s/%s", rutaDir, entrada->d_name);

            if (stat(rutaFile, &fileStat) == -1){
                perror("Error al obtener información del archivo");
                continue;
            }

            // Recursivamente listar subdirectorios
            if (S_ISDIR(fileStat.st_mode) && strcmp(entrada->d_name, ".") != 0 && strcmp(entrada->d_name, "..") != 0){
                listarRecursivamente(rutaFile, hidDir, longDir, linkDir, accDir, rev);
            }
        }

        // Reiniciar el directorio para listar archivos
        rewinddir(dir);
    }

    // Imprimir el nombre del directorio actual
    printf("************%s\n", rutaDir);

    // Listar archivos en el directorio actual
    listarDirectorio(rutaDir, hidDir, longDir, linkDir, accDir);

    if(!rev){
        // Reiniciar el directorio para listar subdirectorios
        rewinddir(dir);

        // Luego listar todos los subdirectorios
        while ((entrada = readdir(dir)) != NULL) {
            // Omitir archivos ocultos si no se especifica -hid
            if (!hidDir && entrada->d_name[0] == '.'){
                continue;
            }

            // Construir la ruta completa del archivo
            snprintf(rutaFile, sizeof(rutaFile), "%s/%s", rutaDir, entrada->d_name);

            if (stat(rutaFile, &fileStat) == -1){
                perror("Error al obtener información del archivo");
                continue;
            }

            // Recursivamente listar subdirectorios
            if (S_ISDIR(fileStat.st_mode) && strcmp(entrada->d_name, ".") != 0 && strcmp(entrada->d_name, "..") != 0){
                listarRecursivamente(rutaFile, hidDir, longDir, linkDir, accDir, rev);
            }
        }
    }

    closedir(dir);
}

void erase(char *trozos[]){
    struct stat infArchivo;

    // Obtenemos la información sobre el archivo en cuestión
    if (lstat(trozos[1], &infArchivo) < 0) {
        printf("Error al obtener información del archivo '%s': %s\n", trozos[1], strerror(errno));
        return;
    }

    // Usamos ConvierteModo2 para mostrar los permisos actuales
    printf("Intentando eliminar '%s' con permisos: %s\n", trozos[1], ConvierteModo2(infArchivo.st_mode));

    // Determinamos el tipo de archivo usando LetraTF
    char tipo = LetraTF(infArchivo.st_mode);

    switch (tipo) {
        case '-': //Fichero normal
            if (unlink(trozos[1]) == 0){
                printf("Archivo regular '%s' eliminado correctamente.\n", trozos[1]);
            }else{
                printf("Error al eliminar el archivo regular '%s': %s\n", trozos[1], strerror(errno));
            }
            break;

        case 'd': // Directorio
            if(rmdir(trozos[1]) == 0){
                printf("Directorio '%s' eliminado correctamente.\n", trozos[1]);
            }else{
                printf("Error al eliminar el directorio '%s': %s\n", trozos[1], strerror(errno));
            }
            break;

        case 'l': //Enlace simbólico
            if (unlink(trozos[1]) == 0){
                printf("Enlace simbólico '%s' eliminado correctamente.\n", trozos[1]);
            }else{
                printf("Error al eliminar el enlace simbólico '%s': %s\n", trozos[1], strerror(errno));
            }
            break;

        case 'p': // FIFO
        case 's': //socket
        case 'b': // bloque
        case 'c':// caracteres

            if (unlink(trozos[1])== 0){
                printf("Archivo especial '%s' (tipo: %c) eliminado correctamente.\n", trozos[1],tipo);
            }else{
                printf("Error al eliminar el archivo especial '%s' (tipo: %c): %s\n",trozos[1], tipo,strerror(errno));
            }
            break;

        default:
            printf("Tipo de archivo no soportado para '%s'.\n",trozos[1]);
            break;
    }
}

void delrec(const char *path){
    struct stat pathStat;
    
    // Obtener la información sobre el archivo o directorio
    if (lstat(path, &pathStat) < 0){
        printf("Error al obtener información de '%s': %s\n", path, strerror(errno));
        return;
    }

    // Comprobamos si es un directorio
    if (S_ISDIR(pathStat.st_mode)){
        DIR *dir= opendir(path);
        if(!dir){
            printf("Error al abrir el directorio '%s': %s\n",path, strerror(errno));
            return;
        }

        struct dirent *entry;
        while ((entry =readdir(dir)) != NULL){
            // Saltar "." y ".."
            if (strcmp(entry->d_name, ".")== 0 ||strcmp(entry->d_name, "..") ==0) {
                continue;
            }

            // Construir la ruta completa del archivo o subdirectorio
            char fullPath[1024];
            snprintf(fullPath, sizeof(fullPath),"%s/%s",path, entry->d_name);

            // Llamada recursiva para eliminar el contenido del directorio
            delrec(fullPath);
        }
        closedir(dir);

        // Intentar eliminar el directorio después de haber eliminado su contenido
        if(rmdir(path)== 0){
            printf("Directorio '%s' eliminado correctamente.\n", path);
        }else {
            printf("Error al eliminar el directorio '%s': %s\n", path, strerror(errno));
        }

    }else {
        // Si no es un directorio: eliminamos el archivo o enlace simbólico
        if (unlink(path)== 0) {
            printf("Archivo '%s' eliminado correctamente.\n", path);
        }else {
            printf("Error al eliminar el archivo '%s': %s\n", path, strerror(errno));
        }
    }
}

// Funciones Practica 2
void Recursiva (int n)
{
  char automatico[TAMANO];
  static char estatico[TAMANO];

  printf ("parametro:%3d(%p) array %p, arr estatico %p\n",n,&n,automatico, estatico);

  if (n>0)
    Recursiva(n-1);
}

void *cadtop(char *str) {
    return (void *)strtoul(str, NULL, 16);
}

void LlenarMemoria (void *p, size_t cont, unsigned char byte)
{
  unsigned char *arr=(unsigned char *) p;
  size_t i;

  for (i=0; i<cont;i++)
        arr[i]=byte;
}

void * ObtenerMemoriaShmget (key_t clave, size_t tam)
{
    void * p;
    int aux,id,flags=0777;
    struct shmid_ds s;

    if (tam)     /*tam distito de 0 indica crear */
        flags=flags | IPC_CREAT | IPC_EXCL; /*cuando no es crear pasamos de tamano 0*/
    if (clave==IPC_PRIVATE)  /*no nos vale*/
        {errno=EINVAL; return NULL;}
    if ((id=shmget(clave, tam, flags))==-1)
        return (NULL);
    if ((p=shmat(id,NULL,0))==(void*) -1){
        aux=errno;
        if (tam)
             shmctl(id,IPC_RMID,NULL);
        errno=aux;
        return (NULL);
    }
    shmctl (id,IPC_STAT,&s); /* si no es crear, necesitamos el tamano, que es s.shm_segsz*/
    InsertarNodo(&head, p, s.shm_segsz, "shmget", id, NULL, 0);
    return (p);
}

void do_AllocateCreateshared (char *tr[])
{
   key_t cl;
   size_t tam;
   void *p;

   if (tr[0]==NULL || tr[1]==NULL) {
        ImprimirLista(head);
        return;
   }
  
   cl=(key_t)  strtoul(tr[0],NULL,10);
   tam=(size_t) strtoul(tr[1],NULL,10);
   if (tam==0) {
    printf ("No se asignan bloques de 0 bytes\n");
    return;
   }
   if ((p=ObtenerMemoriaShmget(cl,tam))!=NULL)
        printf ("Asignados %lu bytes en %p\n",(unsigned long) tam, p);
   else
        printf ("Imposible asignar memoria compartida clave %lu:%s\n",(unsigned long) cl,strerror(errno));
}

void do_AllocateShared(char *tr[]) {
    key_t cl;
    void *p;

    if (tr[0] == NULL) {
        ImprimirLista(head);
        return;
    }

    cl = (key_t)strtoul(tr[0], NULL, 10);

    if ((p = ObtenerMemoriaShmget(cl, 0)) != NULL)
        printf("Asignada memoria compartida de clave %lu en %p\n", (unsigned long)cl, p);
    else
        perror("Imposible asignar memoria compartida");
}

void * MapearFichero (char * fichero, int protection)
{
    int df, map=MAP_PRIVATE,modo=O_RDONLY;
    struct stat s;
    void *p;

    if (protection&PROT_WRITE)
          modo=O_RDWR;
    if (stat(fichero,&s)==-1 || (df=open(fichero, modo))==-1)
          return NULL;
    if ((p=mmap (NULL,s.st_size, protection,map,df,0))==MAP_FAILED)
           return NULL;
    InsertarNodo(&head, p, s.st_size, "mmap", df, fichero, df);
    return p;
}

void do_AllocateMmap(char *arg[])
{ 
     char *perm;
     void *p;
     int protection=0;
     
     if (arg[0]==NULL)
            {ImprimirLista(head); return;}
     if ((perm=arg[1])!=NULL && strlen(perm)<4) {
            if (strchr(perm,'r')!=NULL) protection|=PROT_READ;
            if (strchr(perm,'w')!=NULL) protection|=PROT_WRITE;
            if (strchr(perm,'x')!=NULL) protection|=PROT_EXEC;
     }
     if ((p=MapearFichero(arg[0],protection))==NULL)
             perror ("Imposible mapear fichero");
     else
             printf ("fichero %s mapeado en %p\n", arg[0], p);
}

void do_DeallocateDelkey (char *args[])
{
   key_t clave;
   int id;
   char *key=args[0];

   if (key==NULL || (clave=(key_t) strtoul(key,NULL,10))==IPC_PRIVATE){
        printf ("      delkey necesita clave_valida\n");
        return;
   }
   if ((id=shmget(clave,0,0666))==-1){
        perror ("shmget: imposible obtener memoria compartida");
        return;
   }
   if (shmctl(id,IPC_RMID,NULL)==-1)
        perror ("shmctl: imposible eliminar memoria compartida\n");
}

ssize_t LeerFichero(char *f, void *p, size_t cont) {
    struct stat s;
    ssize_t n;
    int df, aux;

    if (stat(f, &s) == -1 || (df = open(f, O_RDONLY)) == -1)
        return -1;
    if (cont == (size_t)-1)   /* si pasamos -1 como bytes a leer lo leemos entero */
        cont = s.st_size;
    if ((n = read(df, p, cont)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        return -1;
    }
    close(df);
    return n;
}

void Cmd_ReadFile(char *trozos[]) {
    void *p;
    size_t cont = (size_t)-1; /* si no pasamos tamano se lee entero */
    ssize_t n;
    if (trozos[1] == NULL || trozos[2] == NULL) {
        printf("faltan parametros\n");
        return;
    }
    p = (void *)strtoul(trozos[2], NULL, 16); /* convertimos de cadena a puntero */
    if (trozos[3] != NULL)
        cont = (size_t)atoll(trozos[3]);

    if ((n = LeerFichero(trozos[1], p, cont)) == -1)
        perror("Imposible leer fichero");
    else
        printf("leidos %lld bytes de %s en %p\n", (long long)n, trozos[1], p);
}

void Do_pmap (void) /*sin argumentos*/
 { pid_t pid;       /*hace el pmap (o equivalente) del proceso actual*/
   char elpid[32];
   char *argv[4]={"pmap",elpid,NULL};
   
   sprintf (elpid,"%d", (int) getpid());
   if ((pid=fork())==-1){
      perror ("Imposible crear proceso");
      return;
      }
   if (pid==0){
      if (execvp(argv[0],argv)==-1)
         perror("cannot execute pmap (linux, solaris)");
         
      argv[0]="procstat"; argv[1]="vm"; argv[2]=elpid; argv[3]=NULL;   
      if (execvp(argv[0],argv)==-1)/*No hay pmap, probamos procstat FreeBSD */
         perror("cannot execute procstat (FreeBSD)");
         
      argv[0]="procmap",argv[1]=elpid;argv[2]=NULL;    
            if (execvp(argv[0],argv)==-1)  /*probamos procmap OpenBSD*/
         perror("cannot execute procmap (OpenBSD)");
         
      argv[0]="vmmap"; argv[1]="-interleave"; argv[2]=elpid;argv[3]=NULL;
      if (execvp(argv[0],argv)==-1) /*probamos vmmap Mac-OS*/
         perror("cannot execute vmmap (Mac-OS)");      
      exit(1);
  }
  waitpid (pid,NULL,0);
}

void allocate_malloc(size_t size) {
    if (size == 0) {
        printf("No se puede asignar un bloque de 0 bytes\n");
        return;
    }

    void *block = malloc(size);
    if (!block) {
        perror("Error: malloc falló");
        return;
    }
    InsertarNodo(&head, block, size, "malloc", 0, NULL, -1);
    printf("Malloc: Asignado %zu bytes en %p\n", size, block);
}

void deallocate_malloc(void *address) {
    MemBlock *current = head;
    while (current) {
        if (current->address == address && strcmp(current->type, "malloc") == 0) {
            EliminarNodo(&head, address);
            printf("Malloc: Liberado bloque en %p\n", address);
            free(address);
            return;
        } else {
            current = current->next;
        }
    }
    printf("Error: Bloque no encontrado o no es de tipo malloc\n");
}

void deallocate_shared(void *ptr) {
    MemBlock *temp = head;
    while (temp != NULL && temp->address != ptr) temp = temp->next;
    if (temp == NULL) return;
    shmdt(ptr);
    EliminarNodo(&head, ptr);
}

void deallocate_delkey(key_t key) {
    int id = shmget(key, 0, 0666);
    if (id == -1) return;
    shmctl(id, IPC_RMID, NULL);
}

void deallocate_mmap(const char *fichero, MemBlock **head) {
    MemBlock *current = *head; // Puntero al nodo actual
    MemBlock *previous = NULL; // Puntero al nodo previo

    // Buscamos el nodo con el archivo correspondiente
    while (current != NULL && strcmp(current->filename, fichero) != 0) {
        previous = current;
        current = current->next;
    }

    // Si no se encontró el archivo
    if (current == NULL) {
        fprintf(stderr, "Fichero %s no está mapeado en memoria.\n", fichero);
        return;
    }

    // Desmapeamos la memoria
    if (munmap(current->address, current->size) == -1) {
        perror("Error al desmapear el fichero");
        return;
    }

    // Cerramos el descriptor de archivo
    if (current->fd != -1) {
        if (close(current->fd) == -1) {
            perror("Error al cerrar el archivo");
        }
    }

    printf("Archivo %s desmapeado de %p\n", current->filename, current->address);

    // Eliminamos el nodo de la lista
    if (previous == NULL) {
        *head = current->next; // El nodo a eliminar es el primero
    } else {
        previous->next = current->next; // Saltamos el nodo actual
    }

    free(current); // Liberamos la memoria del nodo
}


// Funciones de llenado y volcado de memoria
void memfill(char *trozos[]) {
    void *addr = (void *)strtoul(trozos[1], NULL, 16);
    size_t cont = (size_t)strtoul(trozos[2], NULL, 10);
    unsigned char ch = (unsigned char)strtoul(trozos[3], NULL, 16);
    printf("Llenando %zu bytes de memoria con el byte (%02x) a partir de la direccion %p\n", cont, ch, addr);
    LlenarMemoria(addr, cont, ch);
}

void memdump(char *trozos[]) {
    void *addr = (void *)strtoul(trozos[1], NULL, 16);
    size_t cont = (size_t)strtoul(trozos[2], NULL, 10);
    unsigned char *p = (unsigned char *)addr;
    for (size_t i = 0; i < cont; i++) {
        printf("%02x ", p[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (cont % 16 != 0) printf("\n");
}

// Funciones del programa
void func1() {}
void func2() {}
void func3() {}

// Funciones de información de memoria
void memory_funcs() {
    printf("Funciones programa      %p,    %p,    %p\n", (void*)&func1, (void*)&func2, (void*)&func3);
    printf("Funciones libreria      %p,    %p,    %p\n", (void*)&printf, (void*)&malloc, (void*)&free);
}

// Variables globales
int global_var1 = 1;
int global_var2 = 2;
int global_var3 = 3;

// Variables estáticas
static int static_var1 = 4;
static int static_var2 = 5;
static int static_var3 = 6;

void memory_vars() {
    // Variables locales
    int local_var1 = 7;
    int local_var2 = 8;
    int local_var3 = 9;

    // Variables globales no inicializadas
    static int static_uninit_var1;
    static int static_uninit_var2;
    static int static_uninit_var3;

    // Variables estáticas no inicializadas
    int global_uninit_var1;
    int global_uninit_var2;
    int global_uninit_var3;

    printf("Variables locales       %p,    %p,    %p\n", (void*)&local_var1, (void*)&local_var2, (void*)&local_var3);
    printf("Variables globales      %p,    %p,    %p\n", (void*)&global_var1, (void*)&global_var2, (void*)&global_var3);
    printf("Var (N.I.)globales      %p,    %p,    %p\n", (void*)&global_uninit_var1, (void*)&global_uninit_var2, (void*)&global_uninit_var3);
    printf("Variables staticas      %p,    %p,    %p\n", (void*)&static_var1, (void*)&static_var2, (void*)&static_var3);
    printf("Var (N.I.)staticas      %p,    %p,    %p\n", (void*)&static_uninit_var1, (void*)&static_uninit_var2, (void*)&static_uninit_var3);
}

void memory_blocks() {
    printf("******Lista de bloques asignados para el proceso %d\n", getpid());
    MemBlock *current = head;
    while (current != NULL) {
        printf("Address: %p, Size: %zu, Type: %s, Time: %s", 
               current->address, current->size, current->type, ctime(&current->alloc_time));
        if (strcmp(current->type, "shared") == 0) {
            printf(", Key: %d", current->key);
        }
        if (strcmp(current->type, "mmap") == 0) {
            printf(", File: %s, FD: %d", current->filename, current->fd);
        }
        printf("\n");
        current = current->next;
    }
}

void memory_all() {
    memory_funcs();
    memory_vars();
    memory_blocks();
}

void allocate(char *trozos[]) {
    if (trozos[1] == NULL) {
        printf("Faltan parámetros\n");
        return;
    }

    if (strcmp(trozos[1], "-malloc") == 0) {
        if (trozos[2] == NULL) {
            printf("Faltan parámetros para malloc\n");
            return;
        }
        allocate_malloc(atoi(trozos[2]));
    } else if (strcmp(trozos[1], "-mmap") == 0) {
        if (trozos[2] == NULL) {
            printf("Faltan parámetros para mmap\n");
            return;
        }
        do_AllocateMmap(&trozos[2]);
    } else if (strcmp(trozos[1], "-createshared") == 0) {
        if (trozos[2] == NULL || trozos[3] == NULL) {
            printf("Faltan parámetros para createshared\n");
            return;
        }
        do_AllocateCreateshared(&trozos[2]);
    } else if (strcmp(trozos[1], "-shared") == 0) {
        if (trozos[2] == NULL) {
            printf("Faltan parámetros para shared\n");
            return;
        }
        do_AllocateShared(&trozos[2]);
    } else {
        printf("Comando no reconocido\n");
    }
}

void deallocate_address(void *address) {
    MemBlock *current = head;
    while (current != NULL) {
        if (current->address == address) {
            if (strcmp(current->type, "malloc") == 0) {
                deallocate_malloc(address);
            } else if (strcmp(current->type, "shared") == 0) {
                deallocate_shared(address);
            }
            return;
        }
        current = current->next;
    }
    printf("Error: Bloque no encontrado\n");
}

void deallocate(char *trozos[]) {
    if (trozos[1] == NULL) {
        printf("Faltan parámetros\n");
        return;
    }

    if (strcmp(trozos[1], "-malloc") == 0 || strcmp(trozos[1], "-shared") == 0) {
        if (trozos[2] == NULL) {
            printf("Faltan parámetros para %s\n", trozos[1]);
            return;
        }
        void *address = (void *)strtoul(trozos[2], NULL, 16);
        deallocate_address(address);
    } else if (strcmp(trozos[1], "-delkey") == 0) {
        if (trozos[2] == NULL) {
            printf("Faltan parámetros para delkey\n");
            return;
        }
        key_t key = (key_t)strtoul(trozos[2], NULL, 10);
        deallocate_delkey(key);
    } else if (strcmp(trozos[1], "-mmap") == 0) {
        if (trozos[2] == NULL) {
            printf("Faltan parámetros para mmap\n");
            return;
        }
        deallocate_mmap(trozos[2], &head);
    } else {
        void *address = (void *)strtoul(trozos[1], NULL, 16);
        deallocate_address(address);
    }
}

void memory(char *trozos[]) {
    if (strcmp(trozos[1], "-funcs") == 0) {
        memory_funcs();
    } else if (strcmp(trozos[1], "-vars") == 0) {
        memory_vars();
    } else if (strcmp(trozos[1], "-blocks") == 0) {
        memory_blocks();
    } else if (strcmp(trozos[1], "-all") == 0) {
        memory_all();
    } else if (strcmp(trozos[1], "-pmap") == 0) {
        Do_pmap();
    }
}

ssize_t EscribirFichero(char *f, void *p, size_t cont, int overwrite) {
    ssize_t n;
    int df, aux, flags;

    flags = O_CREAT | O_WRONLY;
    if (overwrite)
        flags |= O_TRUNC;
    else
        flags |= O_EXCL;

    if ((df = open(f, flags, 0777)) == -1)
        return -1;

    if ((n = write(df, p, cont)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        return -1;
    }
    close(df);
    return n;
}

void Cmd_WriteFile(char *trozos[]) {
    void *p;
    size_t cont;
    ssize_t n;
    int overwrite = 0;

    if (trozos[1] == NULL || trozos[2] == NULL || trozos[3] == NULL) {
        printf("Faltan parametros\n");
        return;
    }

    if (strcmp(trozos[1], "-o") == 0) {
        overwrite = 1;
        p = (void *)strtoul(trozos[3], NULL, 16); // convertimos de cadena a puntero
        cont = (size_t)atoll(trozos[4]);
    } else {
        p = (void *)strtoul(trozos[2], NULL, 16); // convertimos de cadena a puntero
        cont = (size_t)atoll(trozos[3]);
    }

    if ((n = EscribirFichero(trozos[overwrite ? 2 : 1], p, cont, overwrite)) == -1)
        perror("Imposible escribir fichero");
    else
        printf("escritos %lld bytes en %s desde %p\n", (long long)n, trozos[overwrite ? 2 : 1], p);
}

void Cmd_Read(char *ar[]) {
    void *p;
    size_t cont;
    ssize_t n;
    int df;

    if (ar[1] == NULL || ar[2] == NULL || ar[3] == NULL) {
        printf("Parametros incorrectos\n");
        return;
    }

    df = atoi(ar[1]);
    p = (void *)strtoul(ar[2], NULL, 16);
    cont = (size_t)atoll(ar[3]);

    if ((n = read(df, p, cont)) == -1)
        perror("Imposible leer fichero");
    else
        printf("leidos %lld bytes de %d en %p\n", (long long)n, df, p);
}

void Cmd_Write(char *ar[]) {
    void *p;
    size_t cont;
    ssize_t n;
    int df;

    if (ar[1] == NULL || ar[2] == NULL || ar[3] == NULL) {
        printf("Parametros incorrectos\n");
        return;
    }

    df = atoi(ar[1]);
    p = (void *)strtoul(ar[2], NULL, 16);
    cont = (size_t)atoll(ar[3]);

    if ((n = write(df, p, cont)) == -1)
        perror("Imposible escribir fichero");
    else
        printf("escritos %lld bytes en %d desde %p\n", (long long)n, df, p);
}


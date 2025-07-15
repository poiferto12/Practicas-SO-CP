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
#include <sys/resource.h>
#include <signal.h>

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

//Lista de búsqueda de directorios 
typedef struct Node {
    char directory[256];       
    struct Node *next;         
} Node;


typedef struct {
    Node *head;                
} DirectoryList;





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
//Funciones de la lista de búsqueda de directorios

void initList(DirectoryList *list) {
    list->head = NULL;         
}

void addDirectory(DirectoryList *list, const char *directory) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        perror("Error al asignar memoria");
        return;
    }
    strncpy(newNode->directory, directory, sizeof(newNode->directory) - 1);
    newNode->directory[sizeof(newNode->directory) - 1] = '\0';
    newNode->next = list->head;  
    list->head = newNode;        
}

void removeDirectory(DirectoryList *list, const char *directory) {
    Node *current = list->head;
    Node *previous = NULL;

    while (current) {
        if (strcmp(current->directory, directory) == 0) {
            if (previous) {
                previous->next = current->next; 
            } else {
                list->head = current->next;   
            }
            free(current);
            printf("Directorio eliminado: %s\n", directory);
            return;
        }
        previous = current;
        current = current->next;
    }
    printf("Directorio no encontrado: %s\n", directory);
}

void clearList(DirectoryList *list) {
    Node *current = list->head;
    Node *nextNode;

    while (current) {
        nextNode = current->next; 
        free(current);            
        current = nextNode;       
    }
    list->head = NULL;            
}

void showDirectories(const DirectoryList *list) {
    Node *current = list->head;
    printf("Lista de búsqueda:\n");
    while (current) {
        printf("- %s\n", current->directory);
        current = current->next;
    }
}

void importPath(DirectoryList *list) {
    char *path = getenv("PATH");
    if (!path) {
        perror("No se pudo obtener el PATH del entorno");
        return;
    }

    char *dir = strtok(path, ":");
    while (dir) {
        addDirectory(list, dir);  
        dir = strtok(NULL, ":");
    }
}


char* SearchListFirst(DirectoryList *list) {
    if (list == NULL || list->head == NULL) {
        return NULL;  
    }
    return list->head->directory;  
}

char* SearchListNext(DirectoryList *list) {
    static Node *current = NULL;  

    if (list == NULL || list->head == NULL) {
        current = NULL;  
        return NULL;}

    if (current == NULL) {
        current = list->head;
        return current->directory; }

    if (current->next != NULL) {
        current = current->next;
        return current->directory;}

    current = NULL;
    return NULL;
}

char *Ejecutable(char *s, DirectoryList *list) {
    static char path[MAX_NAME];
    struct stat st;

    char *p;
    if (s==NULL || (p=SearchListFirst(list))==NULL)
    return s;
    if (s[0]=='/' || !strncmp (s,"./",2) || !strncmp (s,"../",3))
    return s; /*s is an absolute pathname*/
    strncpy (path, p, MAX_NAME-1);strncat (path,"/",MAX_NAME-1); strncat(path,s,MAX_NAME-1);
    if (lstat(path,&st)!=-1)
    return path;
    while ((p=SearchListNext(list))!=NULL){
    strncpy (path, p, MAX_NAME-1);strncat (path,"/",MAX_NAME-1); strncat(path,s,MAX_NAME-1);
    if (lstat(path,&st)!=-1)
    return path;
    }
    return s;
}


// Funciones de la lista de procesos

extern char **environ;

typedef struct Process {
    pid_t pid;
    int priority;
    time_t start_time;
    int signal_received;
    char status[20];
    char command[256];
    struct Process *next;
} Process;

typedef struct {
    Process *head;
} ProcessList;

ProcessList processList = {NULL};


void addProcess(ProcessList *list, pid_t pid, const char *command, int priority) {
    Process *newProcess = (Process *)malloc(sizeof(Process));
    if (!newProcess) {
        perror("Error al asignar memoria");
        return;
    }
    newProcess->pid = pid;
    newProcess->start_time = time(NULL);
    strcpy(newProcess->status, "ACTIVE");
    strncpy(newProcess->command, command, sizeof(newProcess->command) - 1);
    newProcess->priority = priority;
    newProcess->next = list->head;
    list->head = newProcess;
}

void updateProcessStatus(ProcessList *list) {
    Process *current = list->head;
    int status;
    while (current) {
        pid_t result = waitpid(current->pid, &status, WNOHANG);
        if (result == 0) {
            strcpy(current->status, "ACTIVE");
        } else if (result == -1) {
            perror("Error al obtener estado del proceso");
        } else {
            if (WIFEXITED(status)) {
                snprintf(current->status, sizeof(current->status), "FINISHED (%d)", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                snprintf(current->status, sizeof(current->status), "SIGNALED (%d)", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                strcpy(current->status, "STOPPED");
            }
        }
        current = current->next;
    }
}

void freeProcessList(ProcessList *list) {
    Process *current = list->head;
    while (current != NULL) {
        Process *temp = current;
        current = current->next;
        free(temp);
    }
    list->head = NULL;
}


// Funciones de la shell
void imprimirPrompt();
void leerEntrada(char *entrada);
int procesarEntrada(char *entrada, char *trozos[], estadoTerminal *estadoTerminal, DirectoryList *listaDirectorio, char *envp[]);
int trocearCadena(char *cadena, char *trozos[]);
void authors(char *trozos[]);
void pid();
void ppid();
void cd(char *trozos[]);
void date(char *trozos[]);
void historic(char *trozos[], estadoTerminal *estadoTerminal, DirectoryList *listaDirectorio, char *envp[]);
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
void listarDirectorio(const char *nombreDir, int hidDir, int longDir, int linkDir, int accDir);

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
void do_AllocateCreateshared(char *tr[]);
void do_AllocateShared(char *tr[]);
void *MapearFichero(char *fichero, int protection);
ssize_t LeerFichero(char *f, void *p, size_t cont);
void Do_pmap();
void allocate_malloc(size_t size);
void deallocate_malloc(void *address);
void deallocate_shared(void *ptr);
void deallocate_delkey(key_t key);
void deallocate_mmap(const char *fichero, MemBlock **head);
void memory_funcs();
void memory_vars();
void memory_blocks();
void memory_all();

//Funciones practica 3
void getuid_cmd();
void setuid_cmd(char *trozos[]);
void changevar(char *trozos[]);
void showvar(char *trozos[], char *envp[]);
void environ_function(int argc, char *argv[]);
void subsvar(int argc, char *argv[]);
void Cmd_fork (char *tr[]);
void search (DirectoryList *list, char *trozos[]);
int exec(char *cmd[], char **new_env);
int execpri(int priority, char *cmd[], char **new_env);
void fg(char *program, char *args[]) ;
void fgpri(int priority, char *program, char *args[]);
void back(char *program, char *args[]);
void backpri(int priority, char *program, char *args[]) ;
void listjobs(ProcessList *list);
void deljobs(const char *option);

// Función principal
int main(int argc, char *argv[], char *envp[]) {
    char entrada[512];
    char *trozos[64];
    
    int continuar = 1;
    
    // Inicializa el estado de la shell
    estadoTerminal estadoTerminal;
    estadoTerminal.indiceComando = 1;
    estadoTerminal.numArchAbiertos = 0;
    createEmptyList(&estadoTerminal.listaComandos);

    // Inicializa la lista de directorios
    DirectoryList listaDirectorios;
    initList(&listaDirectorios);

    importPath(&listaDirectorios);


    //Procesa la entrada
    while (continuar) {
        imprimirPrompt();
        leerEntrada(entrada);
        if (procesarEntrada(entrada, trozos, &estadoTerminal, &listaDirectorios, envp) == 0) {
            continuar = 0;
        }
    }

    // Liberar memoria del historial de comandos
    freeList(&estadoTerminal.listaComandos);

    // Liberar toda la memoria asignada
    freeAllMemory();

    //Liberar la lista de directorios
    clearList(&listaDirectorios);

    freeProcessList(&processList);

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

int procesarEntrada(char *entrada, char *trozos[], estadoTerminal *estadoTerminal, DirectoryList *listaDirectorios,char *envp[]) {
    entrada[strcspn(entrada, "\n")] = 0; 
    int num_trozos = trocearCadena(entrada, trozos);

    if (num_trozos == 0) {
        return 1;
    }

    char nombreComando[64] = ""; 

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
        historic(trozos, estadoTerminal, listaDirectorios, envp);
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
    } else if (strcmp(trozos[0], "search") == 0) {
        search(listaDirectorios, trozos);
    } else if (strcmp(trozos[0], "exec") == 0) {
        exec(&trozos[1], NULL);
    } else if (strcmp(trozos[0], "getuid") == 0) {
        getuid_cmd();
    } else if (strcmp(trozos[0], "setuid") == 0) {
        setuid_cmd(trozos);
    } else if (strcmp(trozos[0], "changevar") == 0) {
        changevar(trozos);
    } else if (strcmp(trozos[0], "showvar") == 0) {
        showvar(trozos, environ);
    } else if (strcmp(trozos[0], "environ") == 0) {
        environ_function(num_trozos, trozos);
    } else if (strcmp(trozos[0], "subsvar") == 0) {
        subsvar(num_trozos, trozos);
    } else if (strcmp(trozos[0], "fork") == 0) {
        Cmd_fork(trozos);
    } else if (strcmp(trozos[0], "listjobs") == 0) {
        listjobs(&processList);
    } else if (strcmp(trozos[0], "execpri") == 0) {
        execpri(atoi(trozos[1]), &trozos[2], NULL);
    } else if (strcmp(trozos[0], "fg") == 0) {
        fg(trozos[1], &trozos[1]);
    } else if (strcmp(trozos[0], "fgpri") == 0) {
        fgpri(atoi(trozos[1]), trozos[2], &trozos[2]);
    } else if (strcmp(trozos[0], "back") == 0) {
        back(trozos[1], &trozos[1]);
    } else if (strcmp(trozos[0], "backpri") == 0) {
        backpri(atoi(trozos[1]), trozos[2], &trozos[2]);
    } else if (strcmp(trozos[0], "listjobs") == 0) {
        listjobs(&processList);
    } else if (strcmp(trozos[0], "deljobs") == 0) {
        deljobs(trozos[1]);
    } else {
        printf("Comando no reconocido\n");
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

void historic(char *trozos[], estadoTerminal *estadoTerminal, DirectoryList *listaDirectorio, char *envp[]) {
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
                procesarEntrada(aux->comando.nombre, trozosComando, estadoTerminal, listaDirectorio, envp);
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
        } else if (strcmp(trozos[1], "showvar") == 0) {
            printf("showvar var\tMuestra el valor y las direcciones de la variable de entorno var\n");
        } else if (strcmp(trozos[1], "changevar") == 0) {
            printf("changevar [-a|-e|-p] var valor\tCambia el valor de una variable de entorno\n\t-a: accede por el tercer arg de main\n\t-e: accede mediante environ\n\t-p: accede mediante putenv\n");
        } else if (strcmp(trozos[1], "subsvar") == 0) {
            printf("subsvar [-a|-e] var1 var2 valor\tSustituye la variable de entorno var1 con var2=valor\n\t-a: accede por el tercer arg de main\n\t-e: accede mediante environ\n");
        } else if (strcmp(trozos[1], "environ") == 0) {
            printf("environ [-environ|-addr]\tMuestra el entorno del proceso\n\t-environ: accede usando environ (en lugar del tercer arg de main)\n\t-addr: muestra el valor y donde se almacenan environ y el 3er arg main\n");
        } else if (strcmp(trozos[1], "fork") == 0) {
            printf("fork\tEl shell hace fork y queda en espera a que su hijo termine\n");
        } else if (strcmp(trozos[1], "search") == 0) {
            printf("search [-add|-del|-clear|-path]..	Manipula o muestra la ruta de busqueda del shell (path)\n");
	        printf("-add dir: aniade dir a la ruta de busqueda(equiv +dir)\n");
	        printf("-del dir: elimina dir de la ruta de busqueda (equiv -dir)\n");
	        printf("-clear: vacia la ruta de busqueda\n");
	        printf("-path: importa el PATH en la ruta de busqueda\n");
        } else if (strcmp(trozos[1], "exec") == 0) {
            printf("exec VAR1 VAR2 ..prog args....[@pri]	Ejecuta, sin crear proceso,prog con argumentos en un entorno que contiene solo las variables VAR1, VAR2...\n");
        } else if (strcmp(trozos[1], "execpri") == 0) {
            printf("execpri prio prog args....	Ejecuta, sin crear proceso, prog con argumentos con la prioridad cambiada a prio\n");
        } else if (strcmp(trozos[1], "fg") == 0) {
            printf("fg prog args...	Crea un proceso que ejecuta en primer plano prog con argumentos\n");
        } else if (strcmp(trozos[1], "fgpri") == 0) {
            printf("fgpri prio prog args...	Crea un proceso que ejecuta en primer plano prog con argumentos  con la prioridad cambiada a prio\n");
        } else if (strcmp(trozos[1], "back") == 0) {
            printf("back prog args...	Crea un proceso que ejecuta en segundo plano prog con argumentos\n");
        } else if (strcmp(trozos[1], "backpri") == 0) {
            printf("backpri prio prog args...	Crea un proceso que ejecuta en segundo plano prog con argumentos con la prioridad cambiada a prio\n");
        } else if (strcmp(trozos[1], "listjobs") == 0) {
            printf("listjobs 	Lista los procesos en segundo plano\n");
        } else if (strcmp(trozos[1], "deljobs") == 0) {
            printf("deljobs [-term][-sig]	Elimina los procesos de la lista procesos en sp\n");
            printf("\t-term: termina los procesos\n");
            printf("\t-sig: termina los procesos con el señal sig\n");
        } else{
            printf("Comando no reconocido\n");
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

//Practica 3
void getuid_cmd() {
    uid_t real_uid = getuid();
    uid_t effective_uid = geteuid();
    struct passwd *real_pw = getpwuid(real_uid);
    struct passwd *effective_pw = getpwuid(effective_uid);

    printf("Real UID: %d (%s)\n", real_uid, real_pw->pw_name);
    printf("Effective UID: %d (%s)\n", effective_uid, effective_pw->pw_name);
}

void setuid_cmd(char *trozos[]) {
    if (trozos[1] == NULL) {
        printf("Faltan parámetros\n");
        return;
    }

    if (strcmp(trozos[1], "-l") == 0) {
        if (trozos[2] == NULL) {
            uid_t real_uid = getuid();
            uid_t effective_uid = geteuid();
            struct passwd *real_pw = getpwuid(real_uid);
            struct passwd *effective_pw = getpwuid(effective_uid);

            printf("Credencial real: %d (%s)\n", real_uid, real_pw->pw_name);
            printf("Credencial efectiva: %d (%s)\n", effective_uid, effective_pw->pw_name);
            return;
        }

        struct passwd *pwd = getpwnam(trozos[2]);
        if (pwd == NULL) {
            printf("Usuario no existente %s\n", trozos[2]);
            return;
        }

        if (setuid(pwd->pw_uid) == -1) {
            perror("Imposible cambiar credencial");
        } else {
            printf("UID cambiado a %d (%s)\n", pwd->pw_uid, pwd->pw_name);
        }
    } else {
        uid_t uid = atoi(trozos[1]);
        struct passwd *pwd = getpwuid(uid);
        if (pwd == NULL) {
            printf("Usuario no existente %d\n", uid);
            return;
        }

        if (setuid(uid) == -1) {
            perror("Imposible cambiar credencial");
        } else {
            printf("UID cambiado a %d (%s)\n", uid, pwd->pw_name);
        }
    }
}

void showvar(char *trozos[], char *envp[]) {
    if (trozos[1] == NULL) {
        printf("Faltan parámetros\n");
        return;
    }

    for (int i = 1; trozos[i] != NULL; i++) {
        char *var = trozos[i];
        char *value;
        int found = 0;

        // Acceso mediante el tercer argumento de main
        for (int j = 0; envp[j] != NULL; j++) {
            if (strncmp(envp[j], var, strlen(var)) == 0 && envp[j][strlen(var)] == '=') {
                value = envp[j] + strlen(var) + 1;
                printf("Con arg3 main %s=%s(%p) @%p\n", var, value, (void *)value, (void *)&envp[j]);
                found = 1;
                break;
            }
        }

        // Acceso mediante environ
        for (int j = 0; environ[j] != NULL; j++) {
            if (strncmp(environ[j], var, strlen(var)) == 0 && environ[j][strlen(var)] == '=') {
                value = environ[j] + strlen(var) + 1;
                printf("Con environ %s=%s(%p) @%p\n", var, value, (void *)value, (void *)&environ[j]);
                found = 1;
                break;
            }
        }

        // Acceso mediante getenv
        value = getenv(var);
        if (value != NULL) {
            printf("Con getenv %s=%s(%p)\n", var, value, (void *)value);
            found = 1;
        }

        if (!found) {
            printf("Variable de entorno %s no encontrada\n", var);
        }
    }
}

void changevar(char *trozos[]) {
    if (trozos[1] == NULL || trozos[2] == NULL || trozos[3] == NULL) {
        printf("Uso: changevar [-a|-e|-p] var valor\n");
        return;
    }

    char *option = trozos[1];
    char *var = trozos[2];
    char *value = trozos[3];

    if (strcmp(option, "-a") == 0) {
        printf("Simulando añadir %s=%s al tercer argumento de main\n", var, value);
    } else if (strcmp(option, "-e") == 0) {
        if (setenv(var, value, 1) != 0) {
            perror("Imposible cambiar variable");
        } else {
            printf("Variable de entorno %s establecida a %s\n", var, value);
        }
    } else if (strcmp(option, "-p") == 0) {
        char *env_string = malloc(strlen(var) + strlen(value) + 2);
        if (env_string == NULL) {
            perror("Error al asignar memoria");
            return;
        }
        sprintf(env_string, "%s=%s", var, value);
        if (putenv(env_string) != 0) {
            perror("Imposible cambiar variable");
        } else {
            printf("Variable de entorno %s establecida a %s usando putenv\n", var, value);
        }
    } else {
        printf("Opción no válida. Uso: changevar [-a|-e|-p] var valor\n");
    }
}

void environ_function(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "-environ") == 0) {
            for (int i = 0; environ[i] != NULL; i++) {
                printf("%p->environ[%d]=(%p) %s\n", &environ[i], i, environ[i], environ[i]);
            }
        } else if (strcmp(argv[1], "-addr") == 0) {
            printf("environ:   %p (almacenado en %p)\n", environ, (void*)&environ);
        } else {
            printf("Opción desconocida\n");
        }
    } else {
        printf("Uso: environ [-environ|-addr]\n");
    }
}

void subsvar(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso incorrecto. La sintaxis es: subsvar [-a|-e] var1 var2 valor\n");
        return;
    }

    char *opcion = argv[1];
    char *var1 = argv[2];
    char *var2 = argv[3];
    char *valor = argv[4];

    if (strcmp(opcion, "-a") == 0) {
        // Opción -a: cambiar la variable de entorno accediendo por el tercer argumento de main (envp)
        int found = 0;
        for (int i = 0; environ[i] != NULL; i++) {
            if (strncmp(environ[i], var1, strlen(var1)) == 0 && environ[i][strlen(var1)] == '=') {
                // Reemplazar el valor
                size_t var2_len = strlen(var2);
                size_t new_len = var2_len + strlen(valor) + 2; // +2 por '=' y '\0'
                char *new_env = malloc(new_len);
                if (new_env == NULL) {
                    perror("malloc");
                    return;
                }
                snprintf(new_env, new_len, "%s=%s", var2, valor);
                environ[i] = new_env;  // Cambiar la variable en environ
                printf("Variable %s sustituida por %s=%s usando -a\n", var1, var2, valor);
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("Variable %s no encontrada en environ. No se realizó la sustitución.\n", var1);
        }

    } else if (strcmp(opcion, "-e") == 0) {
        // Opción -e: cambiar la variable de entorno usando environ
        int found = 0;
        for (int i = 0; environ[i] != NULL; i++) {
            if (strncmp(environ[i], var1, strlen(var1)) == 0 && environ[i][strlen(var1)] == '=') {
                // Reemplazar el valor
                size_t var2_len = strlen(var2);
                size_t new_len = var2_len + strlen(valor) + 2; // +2 por '=' y '\0'
                char *new_env = malloc(new_len);
                if (new_env == NULL) {
                    perror("malloc");
                    return;
                }
                snprintf(new_env, new_len, "%s=%s", var2, valor);
                environ[i] = new_env;  // Cambiar la variable en environ
                printf("Variable %s sustituida por %s=%s usando -e\n", var1, var2, valor);
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("Variable %s no encontrada en environ. No se realizó la sustitución.\n", var1);
        }

    } else {
        printf("Opción no válida. Uso: subsvar [-a|-e] var1 var2 valor\n");
    }
}

void Cmd_fork (char *tr[])
{
	pid_t pid;
	
	if ((pid=fork())==0){
        freeProcessList(&processList);
		printf ("ejecutando proceso %d\n", getpid());
	}
	else if (pid!=-1)
		waitpid (pid,NULL,0);
}


void search (DirectoryList *list, char *trozos[]){
    if (trozos[1] == NULL) {
        printf("Uso: search [-add dir | -del dir | -clear | -path]\n");
        return;
    }

    if (strcmp(trozos[1], "-add") == 0) {
        if (trozos[2] == NULL) {
            printf("Error: Falta el argumento del directorio para -add.\n");
            return;
        }
        addDirectory(list, trozos[2]);
        printf("Directorio agregado: %s\n", trozos[2]);
    } else if (strcmp(trozos[1], "-del") == 0) {
        if (trozos[2] == NULL) {
            printf("Error: Falta el argumento del directorio para -del.\n");
            return;
        }
        removeDirectory(list, trozos[2]);
        printf("Directorio eliminado: %s\n", trozos[2]);
    } else if (strcmp(trozos[1], "-clear") == 0) {
        clearList(list);
        printf("Lista de búsqueda limpiada.\n");
    } else if (strcmp(trozos[1], "-path") == 0) {
        importPath(list);
        printf("Directorios del PATH importados a la lista de búsqueda.\n");
    } else {
        printf("Comando no válido. Uso: search [-add dir | -del dir | -clear | -path]\n");
    }
    showDirectories(list);
}

int exec(char *cmd[], char **new_env) {
    if (cmd[0] == NULL) {
        printf("exec: No program specified\n");
        return -1;
    }

    // Ejecuta el comando utilizando execve
    return execve(cmd[0], cmd, new_env);
}

int execpri(int priority, char *cmd[], char **new_env) {
    if (cmd[0] == NULL) {
        errno = EFAULT;
        return -1;
    }

    // Set process priority
    if (setpriority(PRIO_PROCESS, getpid(), priority) == -1) {
        perror("Error setting priority");
        return -1;
    }

    // Execute the program
    if (new_env == NULL) {
        return execv(cmd[0], cmd);
    } else {
        return execve(cmd[0], cmd, new_env);
    }
}

void fg(char *program, char *args[]) {
    pid_t pid = fork();

    if (pid < 0) {
        // Error al crear el proceso hijo
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Este es el proceso hijo
        printf("Ejecutando en primer plano: %s\n", program);

        // Intentamos ejecutar el programa con execvp
        if (execvp(program, args) == -1) {
            // Error al ejecutar el programa
            perror("Error al ejecutar el programa");
            exit(EXIT_FAILURE);
        }
    } else {
        // Este es el proceso padre
        int status;

        // Esperar a que el hijo termine
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error en waitpid");
        } else {
            if (WIFEXITED(status)) {
                printf("El proceso hijo terminó con código de salida %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("El proceso hijo fue terminado por la señal %d\n", WTERMSIG(status));
            }
        }
    }
}

void fgpri(int priority, char *program, char *args[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error in fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        printf("Running in foreground with priority %d: %s\n", priority, program);
        if (setpriority(PRIO_PROCESS, 0, priority) == -1) {
            perror("Error setting priority");
            exit(EXIT_FAILURE);
        }
        if (execvp(program, args) == -1) {
            perror("Error executing program");
            exit(EXIT_FAILURE);
        }
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error in waitpid");
        } else {
            if (WIFEXITED(status)) {
                printf("Child process exited with code %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child process terminated by signal %d\n", WTERMSIG(status));
            }
        }
    }
}

void back(char *program, char *args[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (execvp(program, args) == -1) {
            perror("Error al ejecutar el programa");
            exit(EXIT_FAILURE);
        }
    } else {
        // Añadir el proceso a la lista de procesos en segundo plano
        Process *new_process = malloc(sizeof(Process));
        new_process->pid = pid;
        new_process->start_time = time(NULL);  // Obtener hora actual
        new_process->signal_received = 0;     // Inicializar señal
        strcpy(new_process->status, "ACTIVE");    // Establecer estado inicial
        strcpy(new_process->command, program);  // Guardar el comando

        new_process->next = processList.head;
        processList.head = new_process;

        printf("El proceso hijo con PID %d se está ejecutando en segundo plano.\n", pid);
    }
}



void backpri(int priority, char *program, char *args[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        printf("Ejecutando en segundo plano con prioridad %d: %s\n", priority, program);
        if (setpriority(PRIO_PROCESS, 0, priority) == -1) {
            perror("Error al establecer la prioridad");
            exit(EXIT_FAILURE);
        }
        if (execvp(program, args) == -1) {
            perror("Error ejecutando el programa");
            exit(EXIT_FAILURE);
        }
    } else {
        Process *new_process = malloc(sizeof(Process));
        if (new_process == NULL) {
            perror("Error al asignar memoria para el nuevo proceso");
            exit(EXIT_FAILURE);
        }
        new_process->pid = pid;
        new_process->priority = priority; // Almacenar la prioridad
        new_process->start_time = time(NULL);
        new_process->signal_received = 0;
        strcpy(new_process->status, "ACTIVE");
        strcpy(new_process->command, program);
        new_process->next = processList.head;
        processList.head = new_process;
        
        printf("El proceso hijo con PID %d se está ejecutando en segundo plano.\n", pid);
    }
}


const char* format_time(time_t time_val) {
    static char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S", localtime(&time_val));
    return time_str;
}

typedef struct SEN {
    const char *name;
    int number;
} SEN;

static struct SEN sigstrnum[]={   
	{"HUP", SIGHUP},
	{"INT", SIGINT},
	{"QUIT", SIGQUIT},
	{"ILL", SIGILL}, 
	{"TRAP", SIGTRAP},
	{"ABRT", SIGABRT},
	{"IOT", SIGIOT},
	{"BUS", SIGBUS},
	{"FPE", SIGFPE},
	{"KILL", SIGKILL},
	{"USR1", SIGUSR1},
	{"SEGV", SIGSEGV},
	{"USR2", SIGUSR2}, 
	{"PIPE", SIGPIPE},
	{"ALRM", SIGALRM},
	{"TERM", SIGTERM},
	{"CHLD", SIGCHLD},
	{"CONT", SIGCONT},
	{"STOP", SIGSTOP},
	{"TSTP", SIGTSTP}, 
	{"TTIN", SIGTTIN},
	{"TTOU", SIGTTOU},
	{"URG", SIGURG},
	{"XCPU", SIGXCPU},
	{"XFSZ", SIGXFSZ},
	{"VTALRM", SIGVTALRM},
	{"PROF", SIGPROF},
	{"WINCH", SIGWINCH}, 
	{"IO", SIGIO},
	{"SYS", SIGSYS},

#ifdef SIGPOLL
	{"POLL", SIGPOLL},
#endif
#ifdef SIGPWR
	{"PWR", SIGPWR},
#endif
#ifdef SIGEMT
	{"EMT", SIGEMT},
#endif
#ifdef SIGINFO
	{"INFO", SIGINFO},
#endif
#ifdef SIGSTKFLT
	{"STKFLT", SIGSTKFLT},
#endif
#ifdef SIGCLD
	{"CLD", SIGCLD},
#endif
#ifdef SIGLOST
	{"LOST", SIGLOST},
#endif
#ifdef SIGCANCEL
	{"CANCEL", SIGCANCEL},
#endif
#ifdef SIGTHAW
	{"THAW", SIGTHAW},
#endif
#ifdef SIGFREEZE
	{"FREEZE", SIGFREEZE},
#endif
#ifdef SIGLWP
	{"LWP", SIGLWP},
#endif
#ifdef SIGWAITING
	{"WAITING", SIGWAITING},
#endif
 	{NULL,-1},
	};    /*fin array sigstrnum */


int ValorSenal(char * sen)  /*devuelve el numero de senial a partir del nombre*/ 
{ 
  int i;
  for (i=0; sigstrnum[i].name!=NULL; i++)
  	if (!strcmp(sen, sigstrnum[i].name))
		return sigstrnum[i].number;
  return -1;
}


char *NombreSenal(int sen)  /*devuelve el nombre senal a partir de la senal*/ 
{
    int i;
    for (i = 0; sigstrnum[i].name != NULL; i++) {
        if (sen == sigstrnum[i].number) {
            return (char *)sigstrnum[i].name;  // Lanzamos a char* para que no se pierda la constancia
        }
    }
    return ("SIGUNKNOWN");
}

// Función para listar procesos
void listjobs(ProcessList *list) {
    updateProcessStatus(&processList);
    Process *current = list->head;
    while (current) {
        // Esperar a que el proceso hijo termine y actualizar su estado
        int status;
        pid_t result = waitpid(current->pid, &status, WNOHANG);

        // Verificar el estado del proceso
        if (result > 0) {  // El proceso ha cambiado de estado
            if (WIFEXITED(status)) {
                strcpy(current->status, "TERMINADO");
                current->signal_received = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                strcpy(current->status, "SIGNALED");
                current->signal_received = WTERMSIG(status);
            } else if (WIFSTOPPED(status)) {
                strcpy(current->status, "STOPPED");
                current->signal_received = WSTOPSIG(status);
            }
        }

        // Obtener la hora de inicio del proceso en formato "YYYY/MM/DD HH:MM:SS"
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S", localtime(&current->start_time));

        // Imprimir el proceso en el formato deseado
        printf("%d      eliana p=%d %s %s (%d) %s\n",
               current->pid,                           // PID
               current->priority,                      // Prioridad
               format_time(current->start_time),       // Fecha y hora de inicio
               current->status,                        // Estado del proceso (ej. "TERMINADO")
               current->signal_received,               // Señal recibida o código de salida
               current->command);                      // Comando ejecutado

        current = current->next;
    }
}


void deljobs(const char *option) {
    Process *current = processList.head;
    Process *prev = NULL;

    // Recorre la lista de procesos
    while (current != NULL) {
        if ((strcmp(option, "-term") == 0 && strcmp(current->status, "TERMINADO") == 0) ||
            (strcmp(option, "-sig") == 0)) {
            // Eliminar un proceso terminado o señalizado
            printf("%d      %s p=%d %s %s (%d) %s\n", 
                   current->pid, 
                   "eliana",                               // Nombre de usuario
                   current->priority,                      // Prioridad
                   format_time(current->start_time),       // Fecha y hora de inicio
                   current->status,                        // Estado del proceso
                   current->signal_received,               // Código de salida o señal
                   current->command);                      // Comando ejecutado

            // Cambiar el estado a "TERMINADO" si es necesario
            if (strcmp(option, "-sig") == 0 || strcmp(option, "-term") == 0) {
                strcpy(current->status, "TERMINADO");

            }

            // Eliminar el proceso de la lista
            if (prev == NULL) {
                processList.head = current->next;  // Primer proceso
            } else {
                prev->next = current->next;  // Enlaza el proceso anterior con el siguiente
            }

            Process *temp = current;
            current = current->next;
            free(temp); 
             // Libera el proceso
        } 
        else {
            prev = current;
            current = current->next;
        }
    }
}







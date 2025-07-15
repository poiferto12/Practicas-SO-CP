#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_SIZE 4096
#define MAX_HISTORY 64
#define MAX_NAME 256
#define MAX_FILES 100

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
} OpenFile;

typedef struct {
    tList listaComandos;
    int indiceComando;
    OpenFile archivosAbiertos[MAX_FILES];
    int num_openFiles;
} ShellState;

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
    tPosL puntero;
    for (puntero = lista; puntero->siguiente != NULL; puntero = puntero->siguiente);
    return puntero;
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
    tPosL current = *lista;
    tPosL next;

    while (current != NULL) {
        next = current->siguiente;
        free(current->comando.nombre);
        free(current);
        current = next;
    }

    *lista = NULL;
}

// Prototipos de funciones
void imprimirPrompt();
void leerEntrada(char *entrada);
int procesarEntrada(char *entrada, char *trozos[], ShellState *shellState);
int trocearCadena(char *cadena, char *trozos[]);
void authors(char *trozos[]);
void pid();
void ppid();
void cd(char *trozos[]);
void date(char *trozos[]);
void historic(char *trozos[], ShellState *shellState);
void Cmd_open(char *trozos[], ShellState *shellState);
void Cmd_close(char *trozos[], ShellState *shellState);
void Cmd_dup(char *trozos[], ShellState *shellState);
void infosys();
void help(char *trozos[]);
void listarFicherosAbiertos(ShellState *shellState);
void añadirAFicherosAbiertos(ShellState *shellState, int df, char *nombreArchivo, int flags);
void obtenerModoApertura(int flags, char *modo);


// Función principal
int main() {
    char entrada[512];
    char *trozos[32];
    int continuar = 1;

    // Inicializa el estado de la shell
    ShellState shellState;
    shellState.indiceComando = 1;
    shellState.num_openFiles = 0;
    createEmptyList(&shellState.listaComandos);


    while (continuar) {
        imprimirPrompt();
        leerEntrada(entrada);
        if (procesarEntrada(entrada, trozos, &shellState) == 0) {
            continuar = 0;
        }
    }

    // Liberar memoria del historial de comandos
    freeList(&shellState.listaComandos);

    return 0;
}

// Implementación de las funciones

void imprimirPrompt() {
    printf("$: ");
}

void leerEntrada(char *entrada) {
    if (fgets(entrada, 512, stdin) == NULL) {
        perror("Error al leer la entrada");
    }
}

int procesarEntrada(char *entrada, char *trozos[], ShellState *shellState) {
    entrada[strcspn(entrada, "\n")] = 0; // Elimina el salto de línea
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
    nuevoComando.indice = shellState->indiceComando++;
    nuevoComando.nombre = strdup(nombreComando);
    insertItem(nuevoComando, NULL, &shellState->listaComandos);

    // Procesar los comandos básicos
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
        historic(trozos, shellState);
    } else if (strcmp(trozos[0], "open") == 0) {
        Cmd_open(trozos, shellState);
    } else if (strcmp(trozos[0], "close") == 0) {
        Cmd_close(trozos, shellState);
    } else if (strcmp(trozos[0], "dup") == 0) {
        Cmd_dup(trozos, shellState);
    } else if (strcmp(trozos[0], "infosys") == 0) {
        infosys();
    } else if (strcmp(trozos[0], "help") == 0) {
        help(trozos);
    } else {
        printf("Comando no reconocido\n");
    }

    return 1;
}

int trocearCadena(char *cadena, char *trozos[]) {
    int i = 0;
    trozos[i] = strtok(cadena, " \t");
    while (trozos[i] != NULL) {
        i++;
        trozos[i] = strtok(NULL, " \t");
    }
    return i;
}

void authors(char *trozos[]) {
    if (trozos[1] == NULL) {
        printf("Eliana, e.reigadal@udc.es\nChristian, c.lema1@udc.es\n");
    } else if (strcmp(trozos[1], "-l") == 0) {
        printf("e.reigadal@udc.es, c.lema1@udc.es\n");
    } else if (strcmp(trozos[1], "-n") == 0) {
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
    if (trozos[1] == NULL) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("El directorio actual es: %s\n", cwd);
        } else {
            perror("Error al obtener el directorio actual");
        }
    } else {
        if (chdir(trozos[1]) != 0) {
            perror("Error al cambiar de directorio");
        }
    }
}

void date(char *trozos[]) {
    if (trozos[1] == NULL) {
        system("date +\"%d-%m-%Y\"");
        system("date +\"%H:%M:%S\"");
    } else if (strcmp(trozos[1], "-d") == 0) {
        system("date +\"%d-%m-%Y\"");
    } else if (strcmp(trozos[1], "-t") == 0) {
        system("date +\"%H:%M:%S\"");
    }
}

void historic(char *trozos[], ShellState *shellState) {
    tPosL nodo = shellState->listaComandos;
    
    // Si no se pasa ningún argumento, mostramos todo el historial
    if (trozos[1] == NULL) {
        while (nodo != NULL) {
            printf("%d: %s\n", nodo->comando.indice, nodo->comando.nombre);
            nodo = nodo->siguiente;
        }
    }
    // Manejo de "historic -N" para mostrar los últimos N comandos
    else if (trozos[1][0] == '-') {
        // Convertir el argumento "-N" en un número entero (ignorando el primer carácter)
        int N = atoi(&trozos[1][1]);

        if (N <= 0) {  // Validar que N sea positivo
            printf("El número de comandos debe ser positivo.\n");
            return;
        }

        // Contar el número total de comandos en la lista
        int totalComandos = 0;
        tPosL temp = shellState->listaComandos;
        while (temp != NULL) {
            totalComandos++;
            temp = temp->siguiente;
        }

        // Si hay menos comandos de los que pedimos, ajustamos N
        if (N > totalComandos) {
            N = totalComandos;
        }

        // Movemos el puntero de la lista al primer comando que queremos mostrar
        int skip = totalComandos - N;
        for (int i = 0; i < skip; i++) {
            nodo = nodo->siguiente;
        }

        // Mostramos los últimos N comandos
        while (nodo != NULL) {
            printf("%d: %s\n", nodo->comando.indice, nodo->comando.nombre);
            nodo = nodo->siguiente;
        }
    }
    // Manejo de "historic N" para mostrar un comando específico
    else {
        int buscarIndice = atoi(trozos[1]);
        
        // Buscar el comando con el índice especificado
        while (nodo != NULL && nodo->comando.indice != buscarIndice) {
            nodo = nodo->siguiente;
        }

        // Si el comando existe, lo mostramos
        if (nodo != NULL) {
            printf("%d: %s\n", nodo->comando.indice, nodo->comando.nombre);
        } else {
            printf("Error: No se encontró el comando con el índice %d.\n", buscarIndice);
        }
    }
}

void infosys() {
    struct utsname buffer;
    uname(&buffer);
    
    printf("Sistema operativo: %s\n", buffer.sysname);
    printf("Red del host: %s\n", buffer.nodename);
    printf("Versión actual del kernel: %s\n", buffer.release);
    printf("Versión completa del sistema: %s\n", buffer.version);
    printf("Arquitectura hardware: %s\n", buffer.machine);
}

void help(char *trozos[]) {
   
    if (trozos[1]==NULL){
         printf("Comandos disponibles: authors, pid, ppid, cd, date, historic, open, close, dup, infosys, help\n");
    }
    else {
        if (strcmp(trozos[1],"authors") == 0){
            printf ("authors : imprime los nombres y los logins de los autores de la terminal\n");
            printf("authors -l: imprime solo los logins\n");
            printf("authors -n: imprime solo los nombres\n");
        }else if (strcmp(trozos[1],"pid") == 0){
            printf("pid : imprime el PID del proceso que ejecuta la shell\n");
        }else if (strcmp(trozos[1],"ppid") == 0){
            printf("ppid: imprime el PID del proceso padre de la shell\n");
        }else if (strcmp(trozos[1],"cd") == 0){
            printf("cd: imprime el directorio de trabajo actual\n");
            printf("cd [dir]: Cambia el directorio de trabajo actual de la shell a dir\n");
        }else if (strcmp(trozos[1],"date") == 0){
            printf("date: Imprime la fecha actual en el formato DD/MM/YYYY y la hora actual en el formato hh:mm\n");
            printf("date -d :imprime solo la fecha\n");
            printf("date -t solo la hora.\n");
        }else if (strcmp(trozos[1],"historic") == 0){
            printf("historic Imprime todos los comandos con su número de orden\n");
            printf("historic N: Repite el comando número N (del historial)\n");
            printf("historic -N: Imprime solo los últimos N comandos\n");
        }else if (strcmp(trozos[1],"open") == 0){
            printf("open: Abre un archivo y lo agrega (junto con el descriptor de archivo y el modo de apertura) a la lista de archivos abiertos por la shell\n");
            printf(" si pones open a secas y no especificas el modo, te lo abre en modo lectura\n");
            printf(" open cr:Crea el archivo si no existe\n");
            printf("open ap: Añade información al final del archivo\n");
            printf("open ex: Falla si el archivo ya existe\n");
            printf("open ro: Abre el archivo en modo de solo lectura\n");
            printf("open rw: Abre el archivo en modo de lectura y escritura\n");
            printf("open wo: Abre el archivo en modo de solo escritura\n");
            printf("open tr: Trunca el archivo a un tamaño de 0 si existe\n");
        }else if (strcmp(trozos[1],"close") == 0){
            printf("close [df]: Cierra el descriptor de archivo df y elimina el elemento correspondiente de la lista.\n");
        }else if (strcmp(trozos[1],"dup") == 0){
            printf("dup [df]: Duplica el descriptor de archivo df , creando la correspondiente nueva entrada en la lista de archivos.\n");
        }else if (strcmp(trozos[1],"infosys") == 0){
            printf("infosys : Imprime información sobre la máquina que ejecuta la shell\n");
        }else if (strcmp(trozos[1],"help") == 0){
            printf("help : muestra una lista de los comandos disponibles\n");
            printf("help cmd: da una breve ayuda sobre el uso del comando cmd\n");
        }else if (strcmp(trozos[1],"quit") == 0){
            printf("quit, exit, bye: Terminan la shell.\n");
        }else if (strcmp(trozos[1],"exit") == 0){
            printf("quit, exit, bye: Terminan la shell.\n");
        }else if (strcmp(trozos[1],"bye") == 0){
            printf("quit, exit, bye: Terminan la shell.\n");
        }
    }
}

void obtenerModoApertura(int flags, char *modo) {
    strcpy(modo, "");

    // Procesar los flags uno por uno
    if (flags & O_CREAT) strcat(modo, "cr ");
    if (flags & O_EXCL) strcat(modo, "ex ");
    if (flags & O_RDONLY) strcat(modo, "ro ");
    if (flags & O_WRONLY) strcat(modo, "wo ");
    if (flags & O_RDWR) strcat(modo, "rw ");
    if (flags & O_APPEND) strcat(modo, "ap ");
    if (flags & O_TRUNC) strcat(modo, "tr ");

    // Eliminar el último espacio en blanco
    if (strlen(modo) > 0) {
        modo[strlen(modo) - 1] = '\0';
    }
}

void listarFicherosAbiertos(ShellState *shellState) {
    if (shellState->num_openFiles == 0) {
        printf("No hay archivos abiertos.\n");
        return;
    }

    printf("Archivos abiertos:\n");
    for (int i = 0; i < shellState->num_openFiles; i++) {
        printf("Descriptor: %d | Archivo: %s | Modo: %s\n",
               shellState->archivosAbiertos[i].descriptor,
               shellState->archivosAbiertos[i].nombre,
               shellState->archivosAbiertos[i].modo);
    }
}

void añadirAFicherosAbiertos(ShellState *shellState, int df, char *nombreArchivo, int flags) {
    if (shellState->num_openFiles >= MAX_FILES) {
        printf("Se ha alcanzado el límite de archivos abiertos\n");
        return;
    }

    OpenFile nuevoArchivo;
    nuevoArchivo.descriptor = df;
    strncpy(nuevoArchivo.nombre, nombreArchivo, MAX_NAME - 1);
    nuevoArchivo.nombre[MAX_NAME - 1] = '\0';

    // Convertir los flags en modo legible
    obtenerModoApertura(flags, nuevoArchivo.modo);

    shellState->archivosAbiertos[shellState->num_openFiles] = nuevoArchivo;
    shellState->num_openFiles++;
}

char* NombreFicheroDescriptor(int df, ShellState *shellState) {
    for (int i = 0; i < shellState->num_openFiles; i++) {
        if (shellState->archivosAbiertos[i].descriptor == df) {
            return shellState->archivosAbiertos[i].nombre; // Retorna el nombre del archivo
        }
    }
    return NULL; // Si no se encuentra el descriptor, retorna NULL
}

void Cmd_open(char *tr[], ShellState *shellState) {
    int i, df, mode = 0;

    if (tr[1] == NULL) { /* no hay parámetro */
        listarFicherosAbiertos(shellState);
        return;
    }

    if (shellState->num_openFiles >= MAX_FILES) {
        printf("Se ha alcanzado el límite de archivos abiertos\n");
        return;
    }

    for (i = 2; tr[i] != NULL; i++) {
        if (!strcmp(tr[i], "cr")) mode |= O_CREAT;
        else if (!strcmp(tr[i], "ex")) mode |= O_EXCL;
        else if (!strcmp(tr[i], "ro")) mode |= O_RDONLY;
        else if (!strcmp(tr[i], "wo")) mode |= O_WRONLY;
        else if (!strcmp(tr[i], "rw")) mode |= O_RDWR;
        else if (!strcmp(tr[i], "ap")) mode |= O_APPEND;
        else if (!strcmp(tr[i], "tr")) mode |= O_TRUNC;
        else break;
    }

    if ((df = open(tr[1], mode, 0777)) == -1) {
        perror("Imposible abrir fichero");
    } else {
        añadirAFicherosAbiertos(shellState, df, tr[1], mode);
        printf("Añadida entrada a la tabla de ficheros abiertos: %s\n", tr[1]);
    }
}

void Cmd_close(char *tr[], ShellState *shellState) {
    int df;

    // Verifica si hay un argumento y que sea un número válido
    if (tr[1] == NULL || (df = atoi(tr[1])) < 0) {
        listarFicherosAbiertos(shellState); // Llamar a la función para listar archivos abiertos
        return;
    }

    // Verifica que el descriptor no sea uno de los estándares
    if (df == 0 || df == 1 || df == 2) {
        printf("Error: No se puede cerrar el descriptor %d (stdin/stdout/stderr).\n", df);
        return;
    }

    // Intenta cerrar el descriptor
    if (close(df) == -1) {
        perror("Imposible cerrar descriptor");
    } else {
        // Eliminar el archivo de la lista
        for (int i = 0; i < shellState->num_openFiles; i++) {
            if (shellState->archivosAbiertos[i].descriptor == df) {
                // Usar memmove para mover los elementos restantes hacia la izquierda
                memmove(&shellState->archivosAbiertos[i], 
                        &shellState->archivosAbiertos[i + 1], 
                        (shellState->num_openFiles - i - 1) * sizeof(OpenFile));
                shellState->num_openFiles--; // Reducir el número de archivos abiertos
                break;  // Salir del bucle después de cerrar el archivo
            }
        }
        printf("Descriptor cerrado: %d\n", df);
    }
}

void Cmd_dup(char *tr[], ShellState *shellState) {
    int df, duplicado;
    char aux[MAX_NAME], *p;

    // Verifica que el argumento sea un número válido y está en tr[1]
    if (tr[1] == NULL || (df = atoi(tr[1])) < 0) {
        listarFicherosAbiertos(shellState);
        return;
    }

    // Verifica que el descriptor no sea uno de los estándares
    if (df == 0 || df == 1 || df == 2) {
        printf("Error: No se puede duplicar el descriptor %d (stdin/stdout/stderr).\n", df);
        return;
    }

    duplicado = dup(df);
    if (duplicado == -1) {
        perror("Imposible duplicar descriptor");
        return;
    }

    // Buscar el nombre del archivo asociado al descriptor original
    p = NombreFicheroDescriptor(df, shellState);
    if (p == NULL) {
        printf("No se encontró el archivo asociado al descriptor %d\n", df);
        close(duplicado);  // Cerrar el descriptor duplicado si no se encontró el original
        return;
    }

    snprintf(aux, MAX_NAME, "dup %d (%s)", df, p);
    añadirAFicherosAbiertos(shellState, duplicado, aux, fcntl(duplicado, F_GETFL));
    printf("Descriptor duplicado: %d -> %d\n", df, duplicado);
}
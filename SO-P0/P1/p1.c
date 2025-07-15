//Autores:
//Christian Lema c.lema1@udc.es
//Eliana Reigada e.reigadal@udc.es

#define MAX_SIZE 512
#define MAX_HISTORY 100
#define MAX_NAME 100
#define MAX_FILES 50
#define PATH_MAX 4096
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>


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

// Función principal
int main() {
    char entrada[512];
    char *trozos[32];
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
    // Elimina el salto de línea
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
    } else {
        perror("Comando no reconocido\n");
    }

    return 1;
}

int trocearCadena(char *cadena, char *trozos[]) {
    //Parte la cadena en varios trozos (el comando principal y sus argumentos)
    int i = 0;
    trozos[i] = strtok(cadena, " \t");
    while (trozos[i] != NULL) {
        i++;
        trozos[i] = strtok(NULL, " \t");
    }
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
         printf("Comandos disponibles: authors, pid, ppid, cd, date, historic, open, close, dup, infosys, help, makefile, makedir, listfile, cwd, listdir, reclist, revlist, erase, delrec\n");
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
    if (mkdir(trozos[1],0755)== -1){
        perror("Error al crear el directorio");
        return;
    }
    printf("Directorio %s creado con éxito\n",trozos[1]);
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

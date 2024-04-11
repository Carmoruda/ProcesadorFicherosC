#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include "./error_messages.h"
#include "./show_information.h"

/// @brief Gestiona la lógica de comprobar patrones
/// @return 0 si todo ha ido bien, -1 si ha habido un error
int checkPatternsProcess(pthread_mutex_t mutexLogFile, char *log_file, char *consolidated_file);

struct Operacion {
    char IdOperacion[20];
    char FECHA_INICIO[20];
    char FECHA_FIN[20];
    char IdUsuario[20];
    char IdTipoOperacion[20];
    int NoOperacion;
    float Importe;
    char Estado[20];
    int Sucursal;
};

void *pattern1(void *arg);
void *pattern2();
void *pattern3();
void *pattern4();
void *pattern5();

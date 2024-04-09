#include "../include/check_patterns.h"
#include "../include/program_data.h"

pthread_mutex_t mutexPatterns; // Mutex para el control de patrones
pthread_mutex_t mutexLog;      // Mutex para el acceso al log
pthread_mutex_t mutexPatterns;

int checkPatternsProcess(pthread_mutex_t mutexLogFile, char *log_file, char *consolidated_file)
{
    mutexLog = mutexLogFile;
    pthread_t th_pattern1, th_pattern2, th_pattern3, th_pattern4, th_pattern5;

    // Inicializar el mutex
    if (pthread_mutex_init(&mutexPatterns, NULL) != 0)
    {
        printLogScreen(mutexLog, log_file, PATTERN_MUTEX_ERROR, PATTERN_MUTEX_ERROR);
        return -1;
    }

    while (1)
    {
        th_pattern1 = pthread_create(&th_pattern1, NULL, pattern1, NULL);
        th_pattern2 = pthread_create(&th_pattern2, NULL, pattern2, NULL);
        th_pattern3 = pthread_create(&th_pattern3, NULL, pattern3, NULL);
        th_pattern4 = pthread_create(&th_pattern4, NULL, pattern4, NULL);
        th_pattern5 = pthread_create(&th_pattern5, NULL, pattern5, NULL);

        sleep(10);
    }

    return 0;
}

// --- Pattern 1 ---

void *pattern1()
{
    // Lógica de comprobación de patrón 1
}

/// --- Pattern 2 ---

void *pattern2()
{
    // Lógica de comprobación de patrón 2
}

/// --- Pattern 3 ---

void *pattern3()
{
    // Lógica de comprobación de patrón 3
}

/// --- Pattern 4 ---

void *pattern4()
{
    // Lógica de comprobación de patrón 4
}

void *pattern5()
{
    // Lógica de comprobación de patrón 5
}

int comparar_registros(const void *a, const void *b) {
    const struct Operacion *registro1 = (const struct Operacion *)a;
    const struct Operacion *registro2 = (const struct Operacion *)b;
    // Primero comparamos por IdUsuario
    return registro1->IdUsuario != registro2->IdUsuario ? registro1->IdUsuario - registro2->IdUsuario : strcmp(registro1->FECHA_INICIO, registro2->FECHA_INICIO);
}

void readConsolidatedFile(void *arg) {
    sucursal_file *ficheroCSV = (char *)arg;

    // Abrir el archivo en modo lectura y escritura
    FILE *archivo = fopen(ficheroCSV, "r+");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        pthread_exit(NULL);
    }


    // Leer los registros del archivo y almacenarlos en una matriz
    struct Operacion registros[MAX_RECORDS];
    int num_registros = 0;
    char linea[MAX_LINE_LENGTH];
    while (fgets(linea, sizeof(linea), archivo) != NULL && num_registros < MAX_RECORDS) {
        sscanf(linea, "%d;%[^;];%[^;];%d;%d;%d;%f;%s",
               &registros[num_registros].IdOperacion,
               registros[num_registros].FECHA_INICIO,
               registros[num_registros].FECHA_FIN,
               &registros[num_registros].IdUsuario,
               &registros[num_registros].IdTipoOperacion,
               &registros[num_registros].NoOperacion,
               &registros[num_registros].Importe,
               registros[num_registros].Estado);
        num_registros++;
    }
    fclose(ficheroCSV);
    qsort(registros, num_registros, sizeof(struct Operacion), comparar_registros);
}

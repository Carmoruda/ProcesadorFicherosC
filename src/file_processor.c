#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/inotify.h>
#include "../include/show_information.h"
#include "../include/error_messages.h"

#define CONFIG_PATH "./fp.conf"                   // Ruta del archivo de configuración
#define EVENT_SIZE (sizeof(struct inotify_event)) // Tamaño de los eventos
#define BUFFER_LENGTH (1024 * (EVENT_SIZE + 16))  // Tamaño del buffer para eventos
#define MAX_LINE_LENGTH 100
#define MAX_RECORDS 1000

pthread_cond_t cond;           // Variable de condición de los hilos
pthread_mutex_t mutex;         // Mutex para la exclusión mutua
pthread_mutex_t mutexLogFile;  // Mutex para el escritura en el archivo de log
pthread_mutex_t mutexPatterns; // Mutex para el control de patrones
sem_t sem_thread_creation;     // Semáforo para controlar la creación de hilos
pthread_mutex_t mutexLogFile;  // Mutex para el escritura en el archivo de log

DIR *folder; // Directorio de archivos de las sucursales

/// @brief Estructura que contiene la información de los archivos de las sucursales
typedef struct sucursal_file
{
    char file_name[100]; // Nombre del fichero
    int sucursal_number; // Número de la sucursal
    int num_operations;  // Número de operaciones realizadas
} sucursal_file;

struct Operacion
{
    int IdOperacion;
    char FECHA_INICIO[20];
    char FECHA_FIN[20];
    int IdUsuario;
    int IdTipoOperacion;
    int NoOperacion;
    float Importe;
    char Estado[20];
};

/// @brief Estructura que contiene la información del archivo de configuración
struct config_file
{
    char path_files[100];     // Directorio de archivos
    char inventory_file[100]; // Fichero consolidado
    char log_file[100];       // Fichero de log
    int num_processes;        // Número de procesos
    int simulate_sleep_max;   // Tiempo máximo de simulación
    int simulate_sleep_min;   // Tiempo mínimo de simulación
} config_file;

/// @brief Leer la información del archivo de configuración
/// @param pf_config Archivo de configuración
void readConfigFile(FILE *pf_config);

/// @brief Lee un nuevo archivo del directorio
void *reader();

/// @brief Crea un nuevo archivo de una sucursal
/// @param file_name Nombre del archivo
/// @param sucursal_number Número de la sucursal
/// @return Puntero al archivo de la sucursal
sucursal_file *newFile(char *file_name, int sucursal_number);

/// @brief Procesa los ficheros de las sucursales y los escribe en los ficheros
/// de log y consolidado
/// @param file Archivo de la sucursal a procesar
void processFiles(sucursal_file *file);

/// @brief Verifica la llegada de nuevos archivos al directorio común
void *verifyNewFile();

/// @brief Gestiona la lógica de comprobar patrones
/// @return 0 si todo ha ido bien, -1 si ha habido un error
int checkPatternsProcess();

/// @brief Gestiona la lógica de procesamiento de los archivos
/// @return 0 si todo ha ido bien, -1 si ha habido un error
int processFilesProcess();

void *pattern1();
int superaLimiteOperaciones(struct Operacion *registros, int inicio, int fin);
int comparar_registros(const void *a, const void *b);

void *pattern2();
void *pattern3();
void *pattern4();
void *pattern5();

int main()
{
    // Leer archivo de configuración
    FILE *file = fopen(CONFIG_PATH, "r");
    readConfigFile(file);

    // Proceso comprobar patrones
    pid_t proceso_patrones;

    proceso_patrones = fork();
    if (proceso_patrones != 0) // Proceso padre -> Proceso de procesar ficheros
    {
        processFilesProcess();
    }
    else if (proceso_patrones == 0) // Proceso hijo -> Proceso de comprobar patrones
    {
        checkPatternsProcess();
    }

    return 0;
}

void readConfigFile(FILE *pf_config)
{
    int contador = 0;
    char line[256];

    while (fgets(line, sizeof(line), pf_config)) // Leer línea por línea el archivo de configuración.
    {
        char *token = strtok(line, "="); // Separar la línea por el signo de igual.

        while (token != NULL) // Mientras queden palabras en la linea.
        {

            token[strcspn(token, "\n")] = '\0'; // Elimina el salto de línea.

            switch (contador)
            {
            case 1: // PATH_FILES
                strcpy(config_file.path_files, token);
                break;
            case 3: // INVENTORY_FILE
                strcpy(config_file.inventory_file, token);
                break;
            case 5: // LOG_FILE
                strcpy(config_file.log_file, token);
                break;
            case 7: // NUM_PROCESSES
                config_file.num_processes = atoi(token);
                break;
            case 9: // SIMULATE_SLEEP_MAX
                config_file.simulate_sleep_max = atoi(token);
                break;
            case 11: // SIMULATE_SLEEP_MIN
                config_file.simulate_sleep_min = atoi(token);
                break;
            default:
                break;
            }

            contador++;
            token = strtok(NULL, " "); // Siguiente palabra.
        }
    }
}

sucursal_file *newFile(char *file_name, int sucursal_number)
{
    sucursal_file *nueva_sucursal = (sucursal_file *)malloc(sizeof(sucursal_file)); // Reservamos memoria para el nuevo fichero

    strcpy(nueva_sucursal->file_name, file_name);      // Copiamos el nombre del fichero
    nueva_sucursal->sucursal_number = sucursal_number; // Copiamos el número de la sucursal
    nueva_sucursal->num_operations = 0;                // Inicializamos el número de operaciones

    // Avisamos a los hilos que deben comprar si hay un nuevo archivo
    pthread_cond_signal(&cond);

    return nueva_sucursal;
}

void *reader(void *file)
{
    // Se establece simulación de espera
    srand(time(NULL));
    sleep(rand() % config_file.simulate_sleep_max + config_file.simulate_sleep_min);

    processFiles((sucursal_file *)file); // Procesar el archivo

    pthread_exit(NULL); // Salir del hilo
}

void processFiles(sucursal_file *file)
{
    char line[256];                                  // Línea del fichero
    time_t time_date = time(NULL);                   // Dato de tiempo
    char control;                                    // Control de lectura
    char *logString = malloc(600 * sizeof(char));    // Mensaje a escribir en el log
    char *screenString = malloc(600 * sizeof(char)); // Mensaje a mostrar por pantalla

    char dataPath[100];
    char newDataPath[100] = "../processed/";
    strcpy(dataPath, config_file.path_files);
    strcat(dataPath, "/");

    FILE *sucursal_file = fopen(strcat(dataPath, file->file_name), "r"); // Archivo sucursal a procesar
    FILE *consolidated_file = fopen(config_file.inventory_file, "a");    // Archivo consolidado

    if (sucursal_file == NULL)
    {
        sprintf(logString, SUCURSAL_ERROR, file->file_name);
        printLogScreen(mutexLogFile, config_file.log_file, logString, logString);
        return;
    }

    if (consolidated_file == NULL)
    {
        printLogScreen(mutexLogFile, config_file.log_file, CONSOLIDADO_OPEN_ERROR, CONSOLIDADO_OPEN_ERROR);
        return;
    }

    // Formato fichero sucursal    -> ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO
    // Formato fichero consolidado -> ID_SUCURSAL;ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO
    // Formato fichero log         -> DD/MM/AAAA:::HH:MM:SS:::INICIO:::FIN:::NOMBRE_FICHERO:::NUMOPERACIONESCONSOLIDADAS

    // Bucle que leerá el fichero hasta que no haya más información.
    pthread_mutex_lock(&mutex); // Bloquear el mutex

    struct tm current_time = *localtime(&time_date); // Fecha y hora actual

    while (fgets(line, sizeof(line), sucursal_file))
    {
        fprintf(consolidated_file, "%d;%s", file->sucursal_number, line); // Escribir en el fichero consolidado
        file->num_operations++;                                           // Incrementar el número de operaciones
    }

    sprintf(logString, "%d/%d/%d:::%d:%d:%d:::%s:::%d", current_time.tm_mday,
            current_time.tm_mon + 1, current_time.tm_year + 1900,
            current_time.tm_hour, current_time.tm_min, current_time.tm_sec,
            file->file_name,
            file->num_operations);
    sprintf(screenString, "Fichero procesado: %s, con %d operaciones consolidadas", file->file_name, file->num_operations);

    printLogScreen(mutexLogFile, config_file.log_file, logString, screenString); // Imprimir en el log

    fclose(sucursal_file);
    fclose(consolidated_file);

    rename(dataPath, strcat(newDataPath, file->file_name)); // Mover el archivo a la carpeta de procesados
    pthread_mutex_unlock(&mutex);                           // Desbloquear el mutex
}

void *verifyNewFile()
{
    char *logString; // Mensaje a escribir en el log
    int fileDescriptor, watchDescriptor;
    char buffer[BUFFER_LENGTH];

    // Se inicializa el descriptor del inotify
    fileDescriptor = inotify_init();

    if (fileDescriptor < 0)
    { // Se comprueba que se inicialice el descriptor
        printLogScreen(mutexLogFile, config_file.log_file, INOTIFY_DESCRIPTOR_ERROR, INOTIFY_DESCRIPTOR_ERROR);
        exit(EXIT_FAILURE); // Si no se inincializa, avisa y finaliza el proceso con error
    }

    // Se establece el directorio a monitorear
    watchDescriptor = inotify_add_watch(fileDescriptor, config_file.path_files, IN_CREATE);

    if (watchDescriptor < 0)
    { // Se comprueba que se inicialice el watcher
        printLogScreen(mutexLogFile, config_file.log_file, INOTIFY_WATCHER_ERROR, INOTIFY_WATCHER_ERROR);
        exit(EXIT_FAILURE); // Si no se inincializa, avisa y finaliza el proceso con error
    }

    // Se queda a la espera de eventos
    while (1)
    {
        int length, i = 0;
        time_t time_date = time(NULL); // Dato de tiempo

        // Se leen los bytes del evento
        length = read(fileDescriptor, buffer, BUFFER_LENGTH);

        if (length < 0)
        { // Se comprueba que se inicialice correctamente
            printLogScreen(mutexLogFile, config_file.log_file, INOTIFY_LENGTH_ERROR, INOTIFY_LENGTH_ERROR);
            exit(EXIT_FAILURE); // Si no se inincializa, avisa y finaliza el proceso con error
        }

        // Se procesan los eventos
        while (i < length)
        {
            char *newNotificationScreen = "\n\n\n"
                                          "###               ###   ############   ###                           ###\n"
                                          "### ###           ###   ############    ###                         ###\n"
                                          "###   ###         ###   ###              ###                       ###\n"
                                          "###     ###       ###   ############      ###                     ###\n"
                                          "###       ###     ###   ############       ###        ###        ###\n"
                                          "###         ###   ###   ###                 ###     ### ###     ###\n"
                                          "###           ### ###   ############         ### ###       ### ###\n"
                                          "###               ###   ############          ###            ###\n"
                                          "\n\n\n";

            char newNotificationLog[600];
            struct tm current_time = *localtime(&time_date); // Fecha y hora actual
            sprintf(newNotificationLog, "%d/%d/%d:::%d:%d:%d:::NUEVO FICHERO EN EL DIRECTORIO", current_time.tm_mday,
                    current_time.tm_mon + 1, current_time.tm_year + 1900,
                    current_time.tm_hour, current_time.tm_min, current_time.tm_sec);

            struct inotify_event *event = (struct inotify_event *)&buffer[i]; // Se interpreta los datos del buffer como eventos

            if (event->mask & IN_CREATE) // Se comprueba si se ha creado un nuevo archivo
            {
                printLogScreen(mutexLogFile, config_file.log_file, newNotificationLog, newNotificationScreen); // Imprimir en el log
                closedir(folder);                                                                              // Cerrar el directorio
                folder = opendir(config_file.path_files);                                                      // Abrir el directorio de nuevo
            }

            i += EVENT_SIZE + event->len; // Se actualiza el tamaño
        }
    }
}

// --- PROCESS FUNCTIONS ---

int checkPatternsProcess()
{
    pthread_t th_pattern1, th_pattern2, th_pattern3, th_pattern4, th_pattern5;

    // Inicializar el mutex
    if (pthread_mutex_init(&mutexPatterns, NULL) != 0)
    {
        printLogScreen(mutexLogFile, config_file.log_file, PATTERN_MUTEX_ERROR, PATTERN_MUTEX_ERROR);
        return -1;
    }

    while (1)
    {
        th_pattern1 = pthread_create(&th_pattern1, NULL, pattern1, config_file.inventory_file);
        th_pattern2 = pthread_create(&th_pattern2, NULL, pattern2, config_file.inventory_file);
        th_pattern3 = pthread_create(&th_pattern3, NULL, pattern3, config_file.inventory_file);
        th_pattern4 = pthread_create(&th_pattern4, NULL, pattern4, config_file.inventory_file);
        th_pattern5 = pthread_create(&th_pattern5, NULL, pattern5, config_file.inventory_file);

        sleep(10);
    }

    return 0;
}

int processFilesProcess()
{
    sucursal_file *nueva_sucursal;

    // Path a los archivos
    char dataPath[100];
    strcpy(dataPath, config_file.path_files);
    strcat(dataPath, "/");
    struct dirent *directorio = malloc(sizeof(struct dirent));

    // Inicializamos las variables de condición y mutex
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexLogFile, NULL);

    // Inicializar hilos
    pthread_t th1, th2, th3, th4;

    printf("Path files: %s\n", config_file.path_files);

    // Se inicializa un hilo encargado de comprobar la llegada de nuevos archivos
    pthread_t newFileThread;

    int controler;
    controler = pthread_create(&newFileThread, NULL, verifyNewFile, NULL);

    if (controler < 0)
    {
        printLogScreen(mutexLogFile, config_file.log_file, VERIFIER_THREAD_ERROR, VERIFIER_THREAD_ERROR);
    }

    // Se inicializa un semáforo para sincronizar el procesado de archivos
    sem_init(&sem_thread_creation, 0, config_file.num_processes);

    folder = opendir(config_file.path_files);

    if (folder == NULL)
    {
        char *logString = malloc(600 * sizeof(char));
        sprintf(logString, FOLDER_OPEN_ERROR, folder);
        printLogScreen(mutexLogFile, config_file.log_file, logString, logString);
        return -1;
    }

    while (1)
    {
        while ((directorio = readdir(folder)) != NULL)
        {
            if (directorio->d_type == DT_REG) // Comprobar que sea un archivo
            {
                switch (directorio->d_name[4])
                {
                case '1':
                    nueva_sucursal = newFile(directorio->d_name, 1); // Añadimos un archivo de la sucursal 1 a la lista
                    sem_wait(&sem_thread_creation);
                    pthread_create(&th1, NULL, reader, nueva_sucursal); // Crear hilo 1
                    sem_post(&sem_thread_creation);
                    break;
                case '2':
                    nueva_sucursal = newFile(directorio->d_name, 2); // Añadimos un archivo de la sucursal 2 a la lista
                    sem_wait(&sem_thread_creation);
                    pthread_create(&th2, NULL, reader, nueva_sucursal); // Crear hilo 2
                    sem_post(&sem_thread_creation);
                    break;
                case '3':
                    nueva_sucursal = newFile(directorio->d_name, 3); // Añadimos un archivo de la sucursal 3 a la lista
                    sem_wait(&sem_thread_creation);
                    pthread_create(&th3, NULL, reader, nueva_sucursal); // Crear hilo 3
                    sem_post(&sem_thread_creation);
                    break;
                case '4':
                    nueva_sucursal = newFile(directorio->d_name, 4); // Añadimos un archivo de la sucursal 4 a la lista
                    sem_wait(&sem_thread_creation);
                    pthread_create(&th4, NULL, reader, nueva_sucursal); // Crear hilo 4
                    sem_post(&sem_thread_creation);
                    break;
                default:
                    break;
                }

                strcpy(dataPath, config_file.path_files);
                strcat(dataPath, "/");
            }
        }

        sleep(1);
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

void *pattern3()
{
    // Lógica de comprobación de patrón 3
}

void *pattern4()
{
    // Lógica de comprobación de patrón 4
}

void *pattern5()
{
    // Lógica de comprobación de patrón 5
}

#include "../include/check_patterns.h"
#include "../include/error_messages.h"
#include "../include/program_data.h"
#include "../include/show_information.h"
#include <sys/inotify.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <termios.h>

pthread_cond_t cond;          // Variable de condición de los hilos
pthread_mutex_t mutex;        // Mutex para la exclusión mutua
pthread_mutex_t mutexLogFile; // Mutex para el escritura en el archivo de log
sem_t sem_thread_creation;    // Semáforo para controlar la creación de hilos

typedef struct sucursal_info{
  char sucursal_number;
  char line[300];
  int flag;
}sucursal_info;

typedef struct {
  size_t mcSize;
  size_t usedSize;
  int filesCount;
  sucursal_info files[];
}shared_memory;

/// @brief Estructura que contiene la información de los archivos de las
/// sucursales
typedef struct sucursal_file
{
    char file_path[100];  // Ruta del fichero
    char file_name[100];  // Nombre del fichero
    char sucursal_number; // Número de la sucursal
    int num_operations;   // Número de operaciones realizadas
} sucursal_file;

/// @brief Estructura que contiene la información del directorio de una
/// sucursal
typedef struct sucursal_dir
{
    DIR *folder;           // Directorio de la sucursal
    char folder_name[100]; // Nombre del directorio
} sucursal_dir;

/// @brief Estructura que contiene la información del archivo de configuración
struct config_file
{
    char path_files[100];     // Directorio padre de archivos
    char suc_dir[100];        // Directorios de archivos de las sucursales
    char inventory_file[100]; // Fichero consolidado
    char log_file[100];       // Fichero de log
    int num_processes;        // Número de procesos
    int simulate_sleep_max;   // Tiempo máximo de simulación
    int simulate_sleep_min;   // Tiempo mínimo de simulación
    int size_fp;              // Tamaño máximo de la memoria compartida
} config_file;

/// @brief Leer la información del archivo de configuración
/// @param pf_config Archivo de configuración
void readConfigFile(FILE *pf_config);

/// @brief Lee un nuevo archivo del directorio
void *reader();

/// @brief Crea un nuevo archivo de una sucursal
/// @param file_path Ruta del archivo
/// @param file_name Nombre del archivo
/// @param sucursal_number Número de la sucursal
/// @return Puntero al archivo de la sucursal
sucursal_file *newFile(char *file_path, char *file_name, char sucursal_number);

/// @brief Procesa los ficheros de las sucursales y los escribe en los ficheros
/// de log y consolidado
/// @param file Archivo de la sucursal a procesar
void processFiles(sucursal_file *file, shared_memory *sharedMemory_ptr);

/// @brief Verifica la llegada de nuevos archivos al directorio común
/// @param folder_struct Directorio del que queremos verificar la llegada de nuevos archivos
void *verifyNewFile(void *folder_struct);

/// @brief Controla la logica detrás del procesado de directorios de sucursales
int processFilesProcess();

/// @brief Procesa los ficheros de un directorio concreto
/// @param folder_struct Directorio que queremos procesar
void *processSucursalDirectory(void *folder_struct);

/// @brief Muestra el menú de inicio del programa
/// @param error_flag Flag de error en la opción del usuario
/// @return Opción elegida
int Menu(int error_flag);

/// @brief Da comienzo a la auditoria
void StartAudit();

/// @brief Crea la memoria compartida
/// @param size Tamaño de la memoria compartida
/// @param idSharedMemory ID de la memoria compartida
/// @param sharedMemory_ptr Puntero a la memoria compartida
/// @return 0 si éxito, -1 error
int CreateSharedMemory(size_t size, int *idSharedMemory, shared_memory **sharedMemory_ptr);

/// @brief Actualiza el tamaño de la memoria compartida
/// @param idSharedMemory ID de la memoria compartida
/// @param newSize Nuevo tamaño de la memoria compartida
/// @param sharedMemory_ptr Puntero a la memoria compartida
/// @return 0 si éxito, -1 error
int ResizeSharedMemory(int *idSharedMemory, size_t newSize, shared_memory **sharedMemory_ptr);

/// @brief Añade información a la memoria compartida
/// @param idSharedMemory ID de la memoria compartida
/// @param sharedMemory_ptr Puntero a la memoria compartida
/// @param sucInfo Fichero a añadir a la memoria comapartida
void AddDataSharedMemory(int *idSharedMemory, shared_memory **sharedMemory_ptr, sucursal_info sucInfo);

/// @brief Copia la información de la memoria compartida en el archivo csv
/// @param sharedMemory_ptr Puntero a la memoria compartida
/// @param ConsolidatedPath Localización del csv consolidado 
void ConsolidateMemory(shared_memory *sharedMemory_ptr, const char *ConsolidatedPath);

/// @brief Llama a ConsolidateMemory al hacer Ctrl + C
/// @param signal Señal SIGINT
void CloseTriggered(int signal);

//Variables memoria compartida
int IDSharedMemory;
shared_memory *SharedMemory_ptr;

int main()
{
    fflush(stdout);
    // Leer archivo de configuración
    FILE *file = fopen(CONFIG_PATH, "r");
    readConfigFile(file);
    fclose(file);

    char *args[] = {"./create_structure.sh", NULL};
    bool isProgramRunning = true;

    signal(SIGINT, CloseTriggered);

    if(CreateSharedMemory(config_file.size_fp, &IDSharedMemory, &SharedMemory_ptr) == -1){
        printf("Error al crear la memoria virtual.");
        return EXIT_FAILURE;
    }

    do
    {
        switch (Menu(0))
        {
        case 1: // Ejecutar script creación estructura directorios
            pid_t pid = fork();
            if (pid == -1)
            {
                perror(FORK_ERROR);
                return 1;
            }
            else if (pid == 0)
            {
                execvp(args[0], args);
                perror(EXECVP_ERROR);
                return 1;
            }
            else
            {
                int status;
                waitpid(pid, &status, 0);
            }
            break;

        case 2: // Ejecutar programa
            StartAudit();
            break;

        case 3:
            printf("Saliendo del programa...");
            isProgramRunning = false;
            break;
        default:
            printf("Error en la interpretación de la opción elejida. Saliendo...");
            isProgramRunning = false;
            break;
        }
    } while (isProgramRunning);

    return 0;
}

void StartAudit()
{
    // Proceso comprobar patrones
    pid_t proceso_patrones;

    proceso_patrones = fork();
    if (proceso_patrones != 0) // Proceso padre -> Proceso de procesar ficheros
    {
        processFilesProcess();
    }

    if (proceso_patrones == 0) // Proceso hijo -> Proceso de comprobar patrones
    {
        printf("SOY EL HIJO\n");
        checkPatternsProcess(mutexLogFile, config_file.log_file, config_file.inventory_file);
    }
}

void CloseTriggered(int signal){
    printf("\nConsolidando memoria antes de salir...\n");
    printf("Líneas en MC: %d", SharedMemory_ptr->filesCount);
    ConsolidateMemory(SharedMemory_ptr, config_file.inventory_file);
    printf("Ficheros consolidados correctamente en %s.\n", config_file.inventory_file);
    exit(0);
}

int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int CreateSharedMemory(size_t size, int *idSharedMemory, shared_memory **sharedMemory_ptr){
  //Se crea la key
  __key_t smkey = ftok("../output/fich_consolidado.csv", 7);
  if(smkey == -1){
    printf("Error al generar key MC.");
    return -1;
  }
  //Se crea la zona de memoria compartida
  *idSharedMemory = shmget(smkey, size, IPC_CREAT | 0666);
  if(*idSharedMemory == -1){
    printf("Error al crear la zona de memoria compartida.");
    return -1;
  }
  //Se asigna la memoria compartida
  *sharedMemory_ptr = (shared_memory *)shmat(*idSharedMemory, NULL, 0);
  if(*sharedMemory_ptr == (void *)-1){
    printf("Error asignando MC.");
    return -1;
  }
  //Se incializa la memoria de la MC
  (*sharedMemory_ptr)->mcSize = size;
  (*sharedMemory_ptr)->filesCount = 0;
  (*sharedMemory_ptr)->usedSize = 0;

    return 0;
}

int ResizeSharedMemory(int *idSharedMemory, size_t newSize, shared_memory **sharedMemory_ptr) {
    // Guardar el tamaño usado anteriormente y el número de archivos
    size_t prevUsedSize = (*sharedMemory_ptr)->usedSize;
    int filesCount = (*sharedMemory_ptr)->filesCount;
    
    // Guardar un puntero a la memoria compartida actual
    shared_memory *oldMemory_ptr = *sharedMemory_ptr;

    // Crear una nueva zona de memoria compartida
    int newSharedMemoryID = shmget(IPC_PRIVATE, newSize, IPC_CREAT | 0666);
    if (newSharedMemoryID == -1) {
        printf("Error al crear la zona de memoria compartida ResizeSharedMemory.\n");
        return -1;
    }

    // Asociar la nueva memoria compartida
    shared_memory *newMemory_ptr = (shared_memory *)shmat(newSharedMemoryID, NULL, 0);
    if (newMemory_ptr == (void *)-1) {
        printf("Error asignando MC ResizeSharedMemory.\n");
        return -1;
    }

    // Inicializar la nueva memoria compartida
    newMemory_ptr->mcSize = newSize;
    newMemory_ptr->usedSize = prevUsedSize;
    newMemory_ptr->filesCount = filesCount;

    // Calcular el tamaño de los datos a copiar
    size_t dataSize = prevUsedSize - sizeof(shared_memory) + sizeof(sucursal_info) * filesCount;

    // Copiar los datos desde la memoria antigua a la nueva
    memcpy(newMemory_ptr->files, oldMemory_ptr->files, dataSize);

    // Desasociar la memoria antigua
    if (shmdt(oldMemory_ptr) == -1) {
        printf("Error ResizeMemory al desasociar la memoria antigua.\n");
        return -1;
    }

    // Eliminar la memoria compartida antigua
    if (shmctl(*idSharedMemory, IPC_RMID, NULL) == -1) {
        printf("Error eliminando la memoria compartida antigua.\n");
        return -1;
    }

    // Actualizar el identificador de memoria compartida y el puntero
    *idSharedMemory = newSharedMemoryID;
    *sharedMemory_ptr = newMemory_ptr;

    return 0;
}

void AddDataSharedMemory(int *idSharedMemory, shared_memory **sharedMemory_ptr, sucursal_info sucInfo) {
    if ((*sharedMemory_ptr)->usedSize + sizeof(sucInfo) > (*sharedMemory_ptr)->mcSize) {
        size_t newSize = (*sharedMemory_ptr)->mcSize + sizeof(sucInfo);
        if (ResizeSharedMemory(idSharedMemory, newSize, sharedMemory_ptr) == -1) {
            printf("Error redimensionando la memoria compartida.\n");
            return;
        }
    }
    
    (*sharedMemory_ptr)->files[(*sharedMemory_ptr)->filesCount] = sucInfo;
    (*sharedMemory_ptr)->usedSize += sizeof(sucInfo);
    (*sharedMemory_ptr)->filesCount++;

    if ((*sharedMemory_ptr)->filesCount == 3)
    {
        ConsolidateMemory(*sharedMemory_ptr, config_file.inventory_file);
        printf("Ficheros consolidados en %s", config_file.inventory_file);
    }
    
}

void ConsolidateMemory(shared_memory *sharedMemory_ptr, const char *ConsolidatedPath) {
    FILE *consolidated_ptr = fopen(ConsolidatedPath, "a");

    if (consolidated_ptr == NULL) {
        perror("Error al abrir el archivo consolidado");
        return;
    }

    //pthread_mutex_lock(&mutex); // Bloquear el mutex

    for (size_t i = 0; i < sharedMemory_ptr->filesCount; i++) {
        sucursal_info sucInfo = sharedMemory_ptr->files[i];
        fprintf(consolidated_ptr, "%c;%s;%d\n", sucInfo.sucursal_number, sucInfo.line, sucInfo.flag);
    }

    //pthread_mutex_unlock(&mutex); // Desbloquear el mutex

    fclose(consolidated_ptr);
}

int Menu(int error_flag)
{
    int opcion;
    printf("UFV AUDITA\n\n");

    if (error_flag == 1)
    {
        printf("Seleccione una opción válida.\n");
    }

    printf("1. Crear estructura de ficheros\n2. Iniciar\n3. Salir\n\n=> ");
    scanf("%d", &opcion);
    return opcion > 3 || opcion < 1 ? Menu(1) : opcion;
}

void readConfigFile(FILE *pf_config)
{
    int contador = 0; // Contador de línea en la que estamos del fichero
    char line[256];   // Línea leída del fichero

    while (fgets(line, sizeof(line), pf_config)) // Leer línea por línea el archivo de configuración
    {
        char *token = strtok(line, "="); // Separar la línea por el signo de igual

        while (token != NULL) // Mientras queden palabras en la linea
        {

            token[strcspn(token, "\n")] = '\0'; // Elimina el salto de línea

            switch (contador)
            {
            case 1: // FATHER_DIR
                strcpy(config_file.path_files, token);
                break;
            case 3: // SUC_DIR
                strcpy(config_file.suc_dir, token);
                config_file.suc_dir[strcspn(config_file.suc_dir, "{")] = '\0';
                break;
            case 5: // INVENTORY_FILE
                strcpy(config_file.inventory_file, token);
                break;
            case 7: // LOG_FILE
                strcpy(config_file.log_file, token);
                break;
            case 9: // NUM_PROCESSES
                config_file.num_processes = atoi(token);
                break;
            case 11: // SIMULATE_SLEEP_MAX
                config_file.simulate_sleep_max = atoi(token);
                break;
            case 13: // SIMULATE_SLEEP_MIN
                config_file.simulate_sleep_min = atoi(token);
                break;
            case 15: // SIZE_FP
                config_file.size_fp = atoi(token);
                break;
            default:
                break;
            }

            contador++;                // Incrementar el contador de linea
            token = strtok(NULL, " "); // Siguiente palabra
        }
    }
}

sucursal_file *newFile(char *file_path, char *file_name, char sucursal_number)
{
    sucursal_file *nueva_sucursal = (sucursal_file *)malloc(sizeof(sucursal_file)); // Reservamos memoria para el nuevo fichero

    strcpy(nueva_sucursal->file_path, file_path);      // Copiamos la ruta del fichero
    strcpy(nueva_sucursal->file_name, file_name);      // Copiamos el nombre del fichero
    nueva_sucursal->sucursal_number = sucursal_number; // Copiamos el número de la sucursal
    nueva_sucursal->num_operations = 0;                // Inicializamos el número de operaciones

    pthread_cond_signal(&cond); // Avisamos a los hilos que deben comprobar si hay un nuevo archivo

    return nueva_sucursal;
}

void *reader(void *file)
{
    // Se establece simulación de espera
    srand(time(NULL));
    sleep(rand() % config_file.simulate_sleep_max + config_file.simulate_sleep_min);

    processFiles((sucursal_file *)file, SharedMemory_ptr); // Procesar el archivo

    free(file); // Liberar memoria

    pthread_exit(NULL); // Salir del hilo
}

void processFiles(sucursal_file *file, shared_memory *sharedMemory_ptr)
{
    char line[256];                                  // Línea del fichero
    time_t time_date = time(NULL);                   // Dato de tiempo
    char control;                                    // Control de lectura
    char *logString = malloc(600 * sizeof(char));    // Mensaje a escribir en el log
    char *screenString = malloc(600 * sizeof(char)); // Mensaje a mostrar por pantalla

    char newDataPath[100] = "../files_data/processed/"; // Ruta de la carpeta de ficheros procesados

    FILE *sucursal_file = fopen(file->file_path, "r");                // Archivo sucursal a procesar
    FILE *consolidated_file = fopen(config_file.inventory_file, "a"); // Archivo consolidado

    if (sucursal_file == NULL) // Error al abrir el archivo de la sucursal
    {
        sprintf(logString, SUCURSAL_ERROR, file->file_path);
        printLogScreen(mutexLogFile, config_file.log_file, logString, logString);
        return;
    }

    if (consolidated_file == NULL) // Error al abrir el archivo consolidado
    {
        printLogScreen(mutexLogFile, config_file.log_file, CONSOLIDADO_OPEN_ERROR, CONSOLIDADO_OPEN_ERROR);
        return;
    }

    // Formato fichero sucursal    -> ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO
    // Formato fichero consolidado -> ID_SUCURSAL;ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO
    // Formato fichero log         -> DD/MM/AAAA:::HH:MM:SS:::INICIO:::FIN:::NOMBRE_FICHERO:::NUMOPERACIONESCONSOLIDADAS

    pthread_mutex_lock(&mutex); // Bloquear el mutex

    struct tm current_time = *localtime(&time_date); // Fecha y hora actual
    int flag = 0;                                    // Flag para los patrones de actividades irregulares

    // Bucle que leerá el fichero de la sucursal hasta que no haya más información y la escribirá en el fichero consolidado

    // ****A METER A FICHERO CONSOLIDADO. COMENTADO PARA METERLO EN MC
    /*while (fgets(line, sizeof(line), sucursal_file))
    {
        line[strcspn(line, "\n")] = '\0';                                            // Elimina el salto de línea
        fprintf(consolidated_file, "%c;%s;%d\n", file->sucursal_number, line, flag); // Escribir en el fichero consolidado
        file->num_operations++;                                                      // Incrementar el número de operaciones
    }*/
    //Se agrega la línea de los ficheros a la memoria virtual
    while (fgets(line, sizeof(line), sucursal_file)) {
        line[strcspn(line, "\n")] = '\0'; // Elimina el salto de línea
        sucursal_info sucInfo = {file->sucursal_number, "", flag};
        strncpy(sucInfo.line, line, sizeof(sucInfo.line));
        AddDataSharedMemory(&IDSharedMemory,&sharedMemory_ptr, sucInfo);
        file->num_operations++;
    }

    // String que imprimiremos en el log
    sprintf(logString, "%d/%d/%d:::%d:%d:%d:::%s:::%d", current_time.tm_mday,
            current_time.tm_mon + 1, current_time.tm_year + 1900,
            current_time.tm_hour, current_time.tm_min, current_time.tm_sec,
            file->file_path, file->num_operations);

    // String que imprimiremos por pantalla
    sprintf(screenString,
            "Fichero procesado: %s, con %d operaciones consolidadas",
            file->file_path, file->num_operations);

    // Escribimos en el log y mostramos por pantalla
    printLogScreen(mutexLogFile, config_file.log_file, logString, screenString);

    fclose(sucursal_file);     // Cerar el archivo de la sucursal
    fclose(consolidated_file); // Cerrar el archivo consolidado

    rename(file->file_path, strcat(newDataPath, file->file_name)); // Mover el archivo a la carpeta de procesados
    pthread_mutex_unlock(&mutex);                                  // Desbloquear el mutex

    free(logString);
    free(screenString);
}

void *verifyNewFile(void *folder_struct)
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
    watchDescriptor = inotify_add_watch(fileDescriptor, ((sucursal_dir *)folder_struct)->folder_name, IN_CREATE);

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
            char *newNotificationScreen =
                "\n\n\n"
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

            sprintf(newNotificationLog,
                    "%d/%d/%d:::%d:%d:%d:::NUEVO FICHERO EN EL DIRECTORIO",
                    current_time.tm_mday, current_time.tm_mon + 1,
                    current_time.tm_year + 1900, current_time.tm_hour,
                    current_time.tm_min, current_time.tm_sec);

            struct inotify_event *event = (struct inotify_event *)&buffer[i]; // Se interpreta los datos del buffer como eventos

            if (event->mask & IN_CREATE) // Se comprueba si se ha creado un nuevo archivo
            {
                printLogScreen(mutexLogFile, config_file.log_file, newNotificationLog, newNotificationScreen);   // Imprimir en el log
                closedir(((sucursal_dir *)folder_struct)->folder);                                               // Cerrar el directorio
                ((sucursal_dir *)folder_struct)->folder = opendir(((sucursal_dir *)folder_struct)->folder_name); // Abrir el directorio de nuevo
            }

            i += EVENT_SIZE + event->len; // Se actualiza el tamaño
        }
    }
}

// --- PROCESS FUNCTIONS ---

int processFilesProcess()
{
    // Directorios de las sucursales
    char *suc_1_dir_name = malloc(200 * sizeof(char));
    char *suc_2_dir_name = malloc(200 * sizeof(char));
    char *suc_3_dir_name = malloc(200 * sizeof(char));
    char *suc_4_dir_name = malloc(200 * sizeof(char));

    sucursal_dir *suc_1_dir = (sucursal_dir *)malloc(sizeof(sucursal_dir)); // Reservamos memoria para la sucursal 1
    sucursal_dir *suc_2_dir = (sucursal_dir *)malloc(sizeof(sucursal_dir)); // Reservamos memoria para la sucursal 2
    sucursal_dir *suc_3_dir = (sucursal_dir *)malloc(sizeof(sucursal_dir)); // Reservamos memoria para la sucursal 3
    sucursal_dir *suc_4_dir = (sucursal_dir *)malloc(sizeof(sucursal_dir)); // Reservamos memoria para la sucursal 4

    sprintf(suc_1_dir_name, "%s/%s1/", config_file.path_files, config_file.suc_dir); // Ruta del directorio de la sucursal 1
    sprintf(suc_2_dir_name, "%s/%s2/", config_file.path_files, config_file.suc_dir); // Ruta del directorio de la sucursal 1
    sprintf(suc_3_dir_name, "%s/%s3/", config_file.path_files, config_file.suc_dir); // Ruta del directorio de la sucursal 1
    sprintf(suc_4_dir_name, "%s/%s4/", config_file.path_files, config_file.suc_dir); // Ruta del directorio de la sucursal 1

    strcpy(suc_1_dir->folder_name, suc_1_dir_name); // Almacenamos en la estructura del directorio de la sucursal 1 la ruta
    strcpy(suc_2_dir->folder_name, suc_2_dir_name); // Almacenamos en la estructura del directorio de la sucursal 1 la ruta
    strcpy(suc_3_dir->folder_name, suc_3_dir_name); // Almacenamos en la estructura del directorio de la sucursal 1 la ruta
    strcpy(suc_4_dir->folder_name, suc_4_dir_name); // Almacenamos en la estructura del directorio de la sucursal 1 la ruta

    free(suc_1_dir_name);
    free(suc_2_dir_name);
    free(suc_3_dir_name);
    free(suc_4_dir_name);

    // Strings imprimir
    char *logString = malloc(600 * sizeof(char));    // Mensaje a mostrar en el log
    char *screenString = malloc(600 * sizeof(char)); // Mensaje a mostrar por pantalla

    // Inicializamos las variables de condición y mutex
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexLogFile, NULL);

    // Inicializar hilos
    pthread_t th1, th2, th3, th4;

    // Mostrar path ficheros
    time_t time_date = time(NULL);                   // Dato de tiempo
    struct tm current_time = *localtime(&time_date); // Fecha y hora actual

    // String que imprimiremos en el log
    sprintf(logString,
            "%d/%d/%d:::%d:%d:%d:::PROCESAMIENTO ARCHIVOS INICIADO:::DIRECTORIO %s",
            current_time.tm_mday, current_time.tm_mon + 1,
            current_time.tm_year + 1900, current_time.tm_hour, current_time.tm_min,
            current_time.tm_sec, config_file.path_files);

    // String que imprimiremos por pantalla
    sprintf(screenString,
            "Procesamiento de archivos iniciado en el directorio: %s",
            config_file.path_files);

    // Escribimos en el log y mostramos por pantalla
    printLogScreen(mutexLogFile, config_file.log_file, logString, screenString);

    // Se inicializa un hilo encargado de comprobar la llegada de nuevos archivos
    pthread_t newFileThread_SUC1, newFileThread_SUC2, newFileThread_SUC3, newFileThread_SUC4;

    int controler_SUC1, controler_SUC2, controler_SUC3, controler_SUC4;

    controler_SUC1 = pthread_create(&newFileThread_SUC1, NULL, verifyNewFile, suc_1_dir);
    controler_SUC2 = pthread_create(&newFileThread_SUC2, NULL, verifyNewFile, suc_2_dir);
    controler_SUC3 = pthread_create(&newFileThread_SUC3, NULL, verifyNewFile, suc_3_dir);
    controler_SUC4 = pthread_create(&newFileThread_SUC4, NULL, verifyNewFile, suc_4_dir);

    if (controler_SUC1 < 0 || controler_SUC2 < 0 || controler_SUC3 < 0 || controler_SUC4 < 0)
    {
        printLogScreen(mutexLogFile, config_file.log_file, VERIFIER_THREAD_ERROR, VERIFIER_THREAD_ERROR);
    }

    // Se inicializa un semáforo para sincronizar el procesado de archivos
    sem_init(&sem_thread_creation, 0, config_file.num_processes);

    pthread_create(&th1, NULL, processSucursalDirectory, suc_1_dir); // Crear hilo sucursal 1
    pthread_create(&th2, NULL, processSucursalDirectory, suc_2_dir); // Crear hilo sucursal 2
    pthread_create(&th3, NULL, processSucursalDirectory, suc_3_dir); // Crear hilo sucursal 3
    pthread_create(&th4, NULL, processSucursalDirectory, suc_4_dir); // Crear hilo sucursal 4

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    pthread_join(th3, NULL);
    pthread_join(th4, NULL);

    pthread_join(newFileThread_SUC1, NULL);
    pthread_join(newFileThread_SUC2, NULL);
    pthread_join(newFileThread_SUC3, NULL);
    pthread_join(newFileThread_SUC4, NULL);

    free(suc_1_dir);
    free(suc_2_dir);
    free(suc_3_dir);
    free(suc_4_dir);

    free(logString);
    free(screenString);

    return 0;
}

void *processSucursalDirectory(void *folder_struct)
{
    // Inicializar hilo
    pthread_t th;

    // Strings imprimir
    char *logString = malloc(600 * sizeof(char));    // Mensaje a mostrar en el log
    char *screenString = malloc(600 * sizeof(char)); // Mensaje a mostrar por pantalla

    struct dirent *directorio;     // Estructura de directorio
    sucursal_file *nueva_sucursal; // Reservamos memoria para el nuevo fichero

    char filePath[100]; // Ruta del fichero

    ((sucursal_dir *)folder_struct)->folder = opendir(((sucursal_dir *)folder_struct)->folder_name);

    if (((sucursal_dir *)folder_struct)->folder == NULL) // Error al abrir el directorio
    {
        sprintf(logString, FOLDER_OPEN_ERROR, ((sucursal_dir *)folder_struct)->folder);
        printLogScreen(mutexLogFile, config_file.log_file, logString, logString);
        free(logString);
        free(screenString);
        return NULL;
    }

    while (1)
    {
        while ((directorio = readdir(((sucursal_dir *)folder_struct)->folder)) != NULL) // Leer el directorio
        {

            if (directorio->d_type == DT_REG) // Comprobar que sea un archivo
            {
                sprintf(filePath, "%s%s", ((sucursal_dir *)folder_struct)->folder_name, directorio->d_name); // Ruta al archivo
                nueva_sucursal = newFile(filePath, directorio->d_name, filePath[19]);                        // Añadimos un archivo de la sucursal 1 a la lista
                sem_wait(&sem_thread_creation);
                pthread_create(&th, NULL, reader, nueva_sucursal); // Creamos el hilo para procesar la sucursal
                sem_post(&sem_thread_creation);
                pthread_join(th, NULL);
                break;
            }
        }
        int ch = getch();
        if (ch == ' ') {
            printf("Barra espaciadora presionada. Consolidando memoria...\n");
            CloseTriggered(0);
        }
    }

    //sleep(1);
    closedir(((sucursal_dir *)folder_struct)->folder);
    free(logString);
    free(screenString);
    return NULL;
}

/*  CHECKPATTERNS FUNCTIONS BOTTOM */

pthread_mutex_t mutexPatterns; // Mutex para el control de patrones
pthread_mutex_t mutexLog;      // Mutex para el acceso al log
pthread_mutex_t mutexPatterns;

struct Operacion registros[MAX_RECORDS];
struct Operacion reg_patrones[MAX_RECORDS];

int num_registros;

/// @brief Compara dos operaciones para la funcion qsort, que ordena el fichero
/// csv en orden de usuario.
/// @param a Primera operación.
/// @param b Segunda operación.
/// @return Devuelve un 1 si las usuarios coinciden, y un 0 en caso contrario.
int comparar_registros(const void *a, const void *b);

int comparar_por_fecha_inicio(const void *a, const void *b);
/// @brief Compara dos fechas de inicio de dos operaciones para verificar si se
/// cumple la condicion del patron1.
/// @param fecha1 Primera fecha
/// @param fecha2 Segunda fecha.
/// @return Devuelve un 1 si las fechas estan en la misma hora, y un 0 en caso
/// contrario.
int enLaMismaHora(char *fecha1, char *fecha2);

/// @brief Compara dos fechas de inicio de dos operaciones para verificar si se
/// cumple la condicion del mismo día.
/// @param fecha1 Primera fecha
/// @param fecha2 Segunda fecha.
/// @return Devuelve un 1 si las fechas estan en el mismo día, y un 0 en caso
/// contrario.
int enElMismoDía(char *fecha1, char *fecha2);

/// @brief Vuelca todas las operaciones procesadas del fichero consolidado en un
/// vector de estructuras, para poder trabajar sobre el posteriormente.
/// @return Devuelve el número de operaciones que hay en el consolidado.
int readConsolidatedFile();

int checkPatternsProcess(pthread_mutex_t mutexLogFile, char *log_file, char *consolidated_file)
{
        fflush(stdin);
    fflush(stdout);
    mutexLog = mutexLogFile;
    pthread_t th_pattern1, th_pattern2, th_pattern3, th_pattern4, th_pattern5;

    // Inicializar el mutex
    if (pthread_mutex_init(&mutexPatterns, NULL) != 0)
    {
        printLogScreen(mutexLog, log_file, PATTERN_MUTEX_ERROR, PATTERN_MUTEX_ERROR);
        return -1;
    }

    setbuf(stdout, NULL); // Se desactiva el buffer de salida para el flujo actual

    while (1)
    {
        num_registros = readConsolidatedFile();

        pthread_create(&th_pattern1, NULL, pattern1, NULL);
        pthread_create(&th_pattern2, NULL, pattern2, NULL);
        //pthread_create(&th_pattern3, NULL, pattern3, NULL);
        //pthread_create(&th_pattern4, NULL, pattern4, NULL);
        pthread_create(&th_pattern5, NULL, pattern5, NULL);
    }

    pthread_join(th_pattern1, NULL);
    pthread_join(th_pattern2, NULL);
   //pthread_join(th_pattern3, NULL);
    //pthread_join(th_pattern4, NULL);
    pthread_join(th_pattern5, NULL);

    return 0;
}

// --- Pattern 1 ---

void *pattern1(void *arg)
{
    int contadorOperaciones = 0;
    char ultimoUsuario[100];
    strcpy(ultimoUsuario, registros[0].IdUsuario);
    char ultimoTiempo[100];
    strcpy(ultimoTiempo, registros[0].FECHA_INICIO);
    bool cumpleCondicion = false;

    for (int i = 1; i < num_registros; i++)
    {
        // Verificar si es la misma persona y si la operación está dentro del rango
        // de una hora, y si el flag está a 0
        if (strcmp(registros[i].IdUsuario, ultimoUsuario) == 0 &&
            enLaMismaHora(registros[i].FECHA_INICIO, ultimoTiempo) == 1 && registros[i].flag == 0)
        {
            reg_patrones[contadorOperaciones] = registros[i];
            contadorOperaciones++;
            // Si el usuario realiza 5 o más operaciones dentro de una hora, hacer
            // algo
            if (contadorOperaciones > 2)
            {
                cumpleCondicion = true;

            }
        }
        else 
        {
            if (cumpleCondicion == true)
            {

                printf("Datos de la operacion que provoca el patron 1:\n");
                for (int i = 0; i < contadorOperaciones; i++)
                {
                    printf("Sucursal: "
                           "%d,IdOperacion: %s, FECHA_INICIO: %s, FECHA_FIN: %s, "
                           "IdUsuario: %s, IdTipoOperacion: %s, NoOperacion: %d, "
                           "Importe: %.2f, Estado: %s\n",
                           registros[i].Sucursal, registros[i].IdOperacion,
                           registros[i].FECHA_INICIO, registros[i].FECHA_FIN,
                           registros[i].IdUsuario, registros[i].IdTipoOperacion,
                           registros[i].NoOperacion, registros[i].Importe,
                           registros[i].Estado);
                    // Cambio el flag a 1 para evitar que se repita el patron incesablemente.
                    registros[i].flag = 1;
                }

                printf("FIN DEL PATRON\n\n\n\n");
                cumpleCondicion = false;
            }

            // Reiniciar el contador de operaciones para un nuevo usuario o una nueva
            // hora
            contadorOperaciones = 0;
        }

        // Actualizar el usuario y el tiempo para la próxima iteración
        strcpy(ultimoUsuario, registros[i].IdUsuario);
        strcpy(ultimoTiempo, registros[i].FECHA_INICIO);
    }
    pthread_exit(NULL);
}

/// --- Pattern 2 ---

void *pattern2(void *arg)
{
    int contadorOperaciones = 0;
    char ultimoUsuario[100];
    strcpy(ultimoUsuario, registros[0].IdUsuario);
    char ultimoTiempo[100];
    strcpy(ultimoTiempo, registros[0].FECHA_INICIO);
    bool cumpleCondicion = false;

    for (int i = 1; i < num_registros; i++)
    {
        // Verificar si es la misma persona y si la operación está dentro del rango
        // de un día
        if (strcmp(registros[i].IdUsuario, ultimoUsuario) == 0 &&
            enElMismoDía(registros[i].FECHA_INICIO, ultimoTiempo) == 1 &&
            registros[i].Importe < 0 &&
            registros[i].flag == 0)
        {
            
            reg_patrones[contadorOperaciones] = registros[i];
            contadorOperaciones++;
            // Si el usuario realiza 3 o más operaciones de retiro en un mismo día.
            if (contadorOperaciones >= 3)
            {
                cumpleCondicion = true;
            }
        }
        else
        {
            if (cumpleCondicion == true)
            {
                printf("Entra");
                printf("Datos de la operacion que provoca el patron 2:\n");
                for (int i = 0; i < contadorOperaciones; i++)
                {
                    printf("Sucursal: "
                           "%d,IdOperacion: %s, FECHA_INICIO: %s, FECHA_FIN: %s, "
                           "IdUsuario: %s, IdTipoOperacion: %s, NoOperacion: %d, "
                           "Importe: %.2f, Estado: %s\n",
                           registros[i].Sucursal, registros[i].IdOperacion,
                           registros[i].FECHA_INICIO, registros[i].FECHA_FIN,
                           registros[i].IdUsuario, registros[i].IdTipoOperacion,
                           registros[i].NoOperacion, registros[i].Importe,
                           registros[i].Estado);
                    registros[i].flag = 1;
                }
                printf("FIN PATRON 2\n\n\n\n");
                cumpleCondicion = false;
            }

            // Reiniciar el contador de operaciones para un nuevo usuario o una nueva
            // hora
            contadorOperaciones = 0;
        }

        // Actualizar el usuario y el tiempo para la próxima iteración
        strcpy(ultimoUsuario, registros[i].IdUsuario);
        strcpy(ultimoTiempo, registros[i].FECHA_INICIO);
    }

    pthread_exit(NULL);
}

/// --- Pattern 3 ---
/*
void *pattern3(void *arg)
{
    /*int contadorOperaciones = 0;
    char ultimoUsuario[100];
    strcpy(ultimoUsuario, registros[0].IdUsuario);
    char ultimoTiempo[100];
    strcpy(ultimoTiempo, registros[0].FECHA_INICIO);
    bool cumpleCondicion = false;

    for (int i = 1; i < num_registros; i++)
    {
        // Verificar si es la misma persona y si la operación está dentro del rango
        // de una hora
        if (strcmp(registros[i].IdUsuario, ultimoUsuario) &&
            enElMismoDía(registros[i].FECHA_INICIO, ultimoTiempo) == 1 &&
            strcmp(registros[i].Estado, "Error") &&
            registros[i].flag == 0)
        {
            reg_patrones[contadorOperaciones] = registros[i];
            contadorOperaciones++;
            // Si el usuario realiza 3 o más operaciones de retiro en un mismo día.
            if (contadorOperaciones >= 3)
            {
                cumpleCondicion = true;
            }
        }
        else
        {
            if (cumpleCondicion == true)
            {
                printf("Datos de la operacion que provoca el patron 3:\n");
                for (int i = 0; i < contadorOperaciones; i++)
                {
                    printf("Sucursal: "
                           "%d,IdOperacion: %s, FECHA_INICIO: %s, FECHA_FIN: %s, "
                           "IdUsuario: %s, IdTipoOperacion: %s, NoOperacion: %d, "
                           "Importe: %.2f, Estado: %s\n",
                           registros[i].Sucursal, registros[i].IdOperacion,
                           registros[i].FECHA_INICIO, registros[i].FECHA_FIN,
                           registros[i].IdUsuario, registros[i].IdTipoOperacion,
                           registros[i].NoOperacion, registros[i].Importe,
                           registros[i].Estado);
                    // Cambio el flag a 1, para no revisar esta operacion más, porque ya cumplió un patron
                    registros[i].flag = 1;
                }
                cumpleCondicion = false;
            }

            // Reiniciar el contador de operaciones para un nuevo usuario o una nueva
            // hora
            contadorOperaciones = 0;
        }

        // Actualizar el usuario y el tiempo para la próxima iteración
        strcpy(ultimoUsuario, registros[i].IdUsuario);
        strcpy(ultimoTiempo, registros[i].FECHA_INICIO);
    }

    pthread_exit(NULL);
}

/// --- Pattern 4 ---

void *pattern4(void *arg)
{
    struct Operacion *Usuarios = NULL; // Vector de estructuras dinámico
    int tamanoInicial = 100;           // Tamaño inicial del vector (puedes ajustarlo según tus necesidades)
    Usuarios = (struct Operacion *)malloc(tamanoInicial * sizeof(struct Operacion));

    int num_usuarios = 0;
    int contadorOperaciones = 0;
    char ultimoUsuario[20];
    strcpy(ultimoUsuario, registros[0].IdUsuario);
    char ultimoTiempo[20];
    strcpy(ultimoTiempo, registros[0].FECHA_INICIO);
    char tipo;

    for (int i = 1; i < num_registros; i++)
    {
        // Verificar si es la misma persona y si la operación está dentro del rango
        // de un dia
        if (strcmp(registros[i].IdUsuario, ultimoUsuario) &&
            enElMismoDía(registros[i].FECHA_INICIO, ultimoTiempo) == 1 && registros[i].flag == 0)
        {

            strcpy(Usuarios[num_usuarios].IdOperacion, registros[i - 1].IdOperacion);
            strcpy(Usuarios[num_usuarios].FECHA_INICIO, registros[i - 1].FECHA_INICIO);
            strcpy(Usuarios[num_usuarios].FECHA_FIN, registros[i - 1].FECHA_FIN);
            strcpy(Usuarios[num_usuarios].IdUsuario, registros[i - 1].IdUsuario);
            strcpy(Usuarios[num_usuarios].IdTipoOperacion, registros[i - 1].IdTipoOperacion);
            Usuarios[num_usuarios].NoOperacion = registros[i - 1].NoOperacion;
            Usuarios[num_usuarios].Importe = registros[i - 1].Importe;
            strcpy(Usuarios[num_usuarios].Estado, registros[i - 1].Estado);
            Usuarios[num_usuarios].Sucursal = registros[i - 1].Sucursal;
            Usuarios[num_usuarios].DineroIngr = registros[i - 1].DineroIngr;
            Usuarios[num_usuarios].DineroRet = registros[i - 1].DineroRet;
            Usuarios[num_usuarios].flag = registros[i - 1].flag;
            num_usuarios++;
        }
        else
        {                                          // Hemos pasado al siguiente usuario, ya que el vector Registros esta ordenado por usuario, e ignora aquellas operaaciones con flag a 1
                                                   // por lo que toca comprobar si el anterior usuario ha hecho o no los 4 tipos de operaciones
            int cumpleCondicion[4] = {0, 0, 0, 0}; // Vector que me dirá si se han realizado una o más operaciones de cada tipo.
            int Cumple = 1;                        // Variable de control para comprobar que todas se cumplen.
            for (int j = 0; j < num_usuarios; j++)
            {
                tipo = Usuarios[j].IdTipoOperacion[7]; // Tipo guarda el tipo de operación, basándonos en la posicion numero 7 que es la diferencial.
                switch (tipo)
                {
                case '1':
                    cumpleCondicion[0] = 1;
                case '2':
                    cumpleCondicion[1] = 1;
                case '3':
                    cumpleCondicion[2] = 1;
                case '4':
                    cumpleCondicion[4] = 1;
                }
            }
            // Si tras haber analizado todas las operaciones de este usuario hay algun tipo de operacion que no haya sido realizada, Cumple será 0.
            for (int k = 0; k < 4; k++)
            {
                if (cumpleCondicion[k] != 1)
                {
                    Cumple = 0;
                }
            }
            // Si la condicion si se cumple, entonces muestro las operaciones del usuario.
            if (Cumple = 1)
            {
                printf("Datos de la operacion que provoca el patron 4:\n");
                for (int j = 0; j < num_usuarios; j++)
                {
                    printf("Sucursal: "
                           "%d,IdOperacion: %s, FECHA_INICIO: %s, FECHA_FIN: %s, "
                           "IdUsuario: %s, IdTipoOperacion: %s, NoOperacion: %d, "
                           "Importe: %.2f, Estado: %s\n",
                           Usuarios[j].Sucursal, registros[i].IdOperacion,
                           Usuarios[j].FECHA_INICIO, registros[i].FECHA_FIN,
                           Usuarios[j].IdUsuario, registros[i].IdTipoOperacion,
                           Usuarios[j].NoOperacion, registros[i].Importe,
                           Usuarios[j].Estado);
                }
                // Ahora, sabiendo que se cumplió el patron, tengo que cambiar todos los flags de este usuario a 1, para no volver a revisar este patron posteriormente.
                for (int r = 0; r < num_registros; r++)
                {
                    if (strcmp(registros[r].IdUsuario, ultimoUsuario))
                    {
                        registros[r].flag = 1;
                    }
                }
            }
            // Libero y vacio el espacio de memoria, para volver a usarlo con el nuevo usuario siguiente.
            free(Usuarios);   // Liberar la memoria asignada al vector de estructuras Usuarios
            Usuarios = NULL;  // Establecer el puntero a NULL para indicar que el vector está vacío
            num_usuarios = 0; // Restablecer el contador de usuarios
            Usuarios = (struct Operacion *)malloc(tamanoInicial * sizeof(struct Operacion));
        }

        strcpy(ultimoUsuario, registros[i].IdUsuario);
        strcpy(ultimoTiempo, registros[i].FECHA_INICIO);
    }

    pthread_exit(NULL);
}
*/
/// --- Pattern 5 ---

void *pattern5(void *arg)
{
    int contadorOperaciones = 0;
    char ultimoUsuario[100];
    strcpy(ultimoUsuario, registros[0].IdUsuario);
    char ultimoTiempo[100];
    strcpy(ultimoTiempo, registros[0].FECHA_INICIO);

    for (int i = 1; i < num_registros; i++)
    {
        // Verificar si es la misma persona y si la operación está dentro del rango
        // de una hora
        if (strcmp(registros[i].IdUsuario, ultimoUsuario) == 0 &&
            enElMismoDía(registros[i].FECHA_INICIO, ultimoTiempo) == 1 && registros[i].flag == 0)
        {
            if (registros[i].Importe > 0)
            {
                registros[i].DineroIngr += registros[i].Importe;
            }
            else if (registros[i].Importe < 0)
            {
                registros[i].DineroRet += abs(registros[i].Importe);
            }
        }
        else if(strcmp(registros[i].IdUsuario, ultimoUsuario) == 1)
        {
            if (registros[i - 1].DineroRet > registros[i - 1].DineroIngr)
            {
                printf("El usuario %s ha retirado más dinero del que ha ingresado.",
                registros[i - 1].IdUsuario);
                registros[i].flag = 1;
            }
        }

        // Actualizar el usuario y el tiempo para la próxima iteración
        strcpy(ultimoUsuario, registros[i].IdUsuario);
        strcpy(ultimoTiempo, registros[i].FECHA_INICIO);
    }
    // Mostrar los registros ordenados por pantalla

    pthread_exit(NULL);
}

int readConsolidatedFile()
{
    int num_registros = 0;
    char archive[200] = "../output/fich_consolidado.csv";

    // Abrir el archivo en modo lectura y escritura
    FILE *archivo = fopen(archive, "r+");
    if (archivo == NULL)
    {
        perror("Error al abrir el archivo");
        pthread_exit(NULL);
    }

    // Bloquear el mutex antes de acceder al archivo
    // pthread_mutex_lock(&mutexPatterns);

    // Leer los registros del archivo y almacenarlos en una matriz
    char linea[MAX_LINE_LENGTH];
    while (fgets(linea, sizeof(linea), archivo) != NULL)
    {
        // Formato fichero consolidado -> ID_SUCURSAL;ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO

        sscanf(linea, "%d;%[^;];%[^;];%[^;];%[^;];%[^;];%d;%f;%[^;];%d",
               &registros[num_registros].Sucursal,
               registros[num_registros].IdOperacion,
               registros[num_registros].FECHA_INICIO,
               registros[num_registros].FECHA_FIN,
               registros[num_registros].IdUsuario,
               registros[num_registros].IdTipoOperacion,
               &registros[num_registros].NoOperacion,
               &registros[num_registros].Importe, registros[num_registros].Estado,
               registros[num_registros].flag);
        registros[num_registros].DineroIngr = 0;
        registros[num_registros].DineroRet = 0;
        num_registros++;
    }

    // Cerrar el archivo
    fclose(archivo);

    // Desbloquear el mutex después de acceder al archivo
    // pthread_mutex_unlock(&mutexPatterns);

    if (num_registros > 1)
    {
        // Ordenar el vector por fecha de inicio y usuario
        qsort(registros, num_registros, sizeof(struct Operacion), comparar_registros);
    }

    return num_registros;
}
// Función de apoyo a la función de qsort, para ordenar por usuarios.
int comparar_registros(const void *a, const void *b)
{
    const struct Operacion *registro1 = (const struct Operacion *)a;
    const struct Operacion *registro2 = (const struct Operacion *)b;

    return strcmp(registro1->IdUsuario, registro2->IdUsuario);
}


// Función para verificar si se superan las 5 operaciones por hora
int enElMismoDía(char *fecha1, char *fecha2)
{
    int dia1, mes1, anio1, hora1, minuto1;
    int dia2, mes2, anio2, hora2, minuto2;

    // Parseamos las fechas
    sscanf(fecha1, "%2d/%2d/%4d%2d:%2d", &dia1, &mes1, &anio1, &hora1, &minuto1);
    sscanf(fecha2, "%2d/%2d/%4d%2d:%2d", &dia2, &mes2, &anio2, &hora2, &minuto2);

    // Comparamos día, mes y año
    if (dia1 == dia2 && mes1 == mes2 && anio1 == anio2)
    {
        return 1; // Están en el mismo día
    }
    else
    {
        return 0; // No están en el mismo día
    }
}

// Función para verificar si se superan las 5 operaciones por hora
int enLaMismaHora(char *fecha1, char *fecha2)
{
    int dia1, mes1, anio1, hora1, minuto1;
    int dia2, mes2, anio2, hora2, minuto2;

    // Parseamos las fechas
    sscanf(fecha1, "%2d/%2d/%4d%2d:%2d", &dia1, &mes1, &anio1, &hora1, &minuto1);
    sscanf(fecha2, "%2d/%2d/%4d%2d:%2d", &dia2, &mes2, &anio2, &hora2, &minuto2);

    // Comparamos día, mes, año y hora
    if (dia1 == dia2 && mes1 == mes2 && anio1 == anio2 && hora1 == hora2)
    {
        return 1; // Están en la misma hora
    }
    else
    {

        return 0; // No están en la misma hora
    }
}
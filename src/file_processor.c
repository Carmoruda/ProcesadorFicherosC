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
#include "../include/program_data.h"


pthread_cond_t cond;          // Variable de condición de los hilos
pthread_mutex_t mutex;        // Mutex para la exclusión mutua
pthread_mutex_t mutexLogFile; // Mutex para el escritura en el archivo de log
sem_t sem_thread_creation;    // Semáforo para controlar la creación de hilos
pthread_mutex_t mutexLogFile; // Mutex para el escritura en el archivo de log
pthread_mutex_t mutexPatrones;

DIR *folder; // Directorio de archivos de las sucursales

/// @brief Estructura que contiene la información de los archivos de las sucursales
typedef struct sucursal_file
{
    char file_name[100]; // Nombre del fichero
    int sucursal_number; // Número de la sucursal
    int num_operations;  // Número de operaciones realizadas
} sucursal_file;

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

///@brief Estructura para representar cada registro del archivo CSV
struct Operacion {
    int IdOperacion;
    char FECHA_INICIO[20];
    char FECHA_FIN[20];
    int IdUsuario;
    int IdTipoOperacion;
    int NoOperacion;
    float Importe;
    char Estado[20];
};
/// @brief Leer la información del archivo de configuración
/// @param pf_config Archivo de configuración
void readConfigFile(FILE *pf_config);

/// @brief Leer la información del archivo de configuración
/// @param registros Vector de estructuras con todas las operaciones extraidas del .csv
/// @param inicio Variables para controlar que se cumplan las condiciones
/// @param fin Variables para controlar que se cumplan las condiciones
int superaLimiteOperaciones(struct Operacion *registros, int inicio, int fin);

/// @brief Detectar si se produce el patron numero 1 en el fichero consolidado
/// @param arg Archivo de proceso consolidado
void patron1(void *arg);

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

/// @brief Verifca la llegada de nuevos archivos al directorio común
void *verifyNewFile();

int main()
{
    sucursal_file *nueva_sucursal;
    int pid_patrones;
    // Path a los archivos
    char dataPath[100];
    strcpy(dataPath, config_file.path_files);
    strcat(dataPath, "/");
    struct dirent *directorio = malloc(sizeof(struct dirent));

    // Inicializamos las variables de condición y mutex
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexLogFile, NULL);

    // Leer archivo de configuración
    FILE *file = fopen(CONFIG_PATH, "r");
    readConfigFile(file);

    // Inicializar hilos
    pthread_t th1, th2, th3, th4, p1, p2, p3;

    printf("Path files: %s\n", config_file.path_files);

    // Se inicializa un hilo encargado de comprobar la llegada de nuevos archivos
    pthread_t newFileThread;
    int controler;
    controler = pthread_create(&newFileThread, NULL, verifyNewFile, NULL);

    if (controler < 0)
    {
        printf("Error al crear hilo verifier");
    }

    // Se inicializa un semáforo para sincronizar el procesado de archivos
    sem_init(&sem_thread_creation, 0, config_file.num_processes);

    folder = opendir(config_file.path_files);

    if (folder == NULL)
    {
        printf("Error al abrir el directorio.");
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
                pid_patrones = fork();
                if (pid_patrones) // Este es el proceso padre
                {
                    printf("PADRE: Soy el proceso padre y mi pid sigue siendo: %d\n", getpid());
                    printf("PADRE: Mi hijo tiene el pid: %d\n", pid_patrones);

                    pthread_create(&p1, NULL, patron1, (void *)nueva_sucursal);

                    pthread_join(p1, NULL);
                    // Destruir el mutex
                    pthread_mutex_destroy(&mutexPatrones);
                }
                else // Proceso hijo
                {
                    printf("HIJO: Soy el proceso hijo y mi pid es: %d\n", getpid());
                    printf("HIJO: mi padre tiene el pid: %d\n", getppid());
                }

                strcpy(dataPath, config_file.path_files);
                strcat(dataPath, "/");
            }
        }

        sleep(1);
    }

    return 0;
}


// Función para verificar si se superan las 5 operaciones por hora
int superaLimiteOperaciones(struct Operacion *registros, int inicio, int fin) {
    int contadorOperaciones = 1;
    for (int i = inicio + 1; i < fin; i++) {
        // Calcular la diferencia de tiempo entre operaciones adyacentes
        struct tm tm1, tm2;
        strptime(registros[i - 1].FECHA_INICIO, "%d/%m/%Y %H:%M", &tm1);
        strptime(registros[i].FECHA_INICIO, "%d/%m/%Y %H:%M", &tm2);
        time_t tiempo1 = mktime(&tm1);
        time_t tiempo2 = mktime(&tm2);
        double diferenciaTiempo = difftime(tiempo2, tiempo1);

        // Si la diferencia de tiempo es menor o igual a una hora, incrementar el contador de operaciones
        if (diferenciaTiempo <= 3600) { // 3600 segundos = 1 hora
            contadorOperaciones++;
            // Si el contador supera 5, retornar verdadero
            if (contadorOperaciones > 5) {
                return 1;
            }
        } else {
            // Si la diferencia de tiempo es mayor a una hora, reiniciar el contador
            contadorOperaciones = 1;
        }
    }
    return 0;
}
void patron1(void *arg) {
    sucursal_file *ficheroCSV = (char *)arg;

    // Abrir el archivo en modo lectura y escritura
    FILE *archivo = fopen(ficheroCSV, "r+");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        pthread_exit(NULL);
    }

    // Bloquear el mutex antes de acceder al archivo
    pthread_mutex_lock(&mutex);

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


    int i = 0;
    while (i < num_registros) {
        int idUsuarioActual = registros[i].IdUsuario;
        int j = i + 1;
        while (j < num_registros && registros[j].IdUsuario == idUsuarioActual) {
            // Verificar si se superan las 5 operaciones por hora
            if (superaLimiteOperaciones(registros, i, j)) {
                // Mostrar las operaciones realizadas por el usuario
                printf("Usuario %d ha realizado más de 5 operaciones en menos de 1 hora:\n", idUsuarioActual);
                for (int k = i; k < j; k++) {
                    printf("IdOperacion: %d, FECHA_INICIO: %s, FECHA_FIN: %s, IdUsuario: %d, IdTipoOperacion: %d, NoOperacion: %d, Importe: %.2f, Estado: %s\n",
                           registros[k].IdOperacion,
                           registros[k].FECHA_INICIO,
                           registros[k].FECHA_FIN,
                           registros[k].IdUsuario,
                           registros[k].IdTipoOperacion,
                           registros[k].NoOperacion,
                           registros[k].Importe,
                           registros[k].Estado);
                }
                break; // Salir del bucle interno
            }
            j++;
        }
        i = j;
    }
    // Mostrar los registros ordenados por pantalla
    for (int i = 0; i < num_registros; i++) {
        printf("IdOperacion: %d, FECHA_INICIO: %s, FECHA_FIN: %s, IdUsuario: %d, IdTipoOperacion: %d, NoOperacion: %d, Importe: %.2f, Estado: %s\n",
               registros[i].IdOperacion,
               registros[i].FECHA_INICIO,
               registros[i].FECHA_FIN,
               registros[i].IdUsuario,
               registros[i].IdTipoOperacion,
               registros[i].NoOperacion,
               registros[i].Importe,
               registros[i].Estado);
    }

    // Desbloquear el mutex después de acceder al archivo
    pthread_mutex_unlock(&mutex);

    // Cerrar el archivo
    fclose(archivo);

    pthread_exit(NULL);
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
    char line[256];                // Línea del fichero
    time_t time_date = time(NULL); // Dato de tiempo
    char control;                  // Control de lectura
    char logString[600];           // Mensaje a escribir en el log
    char screenString[600];        // Mensaje a mostrar por pantalla

    char dataPath[100];
    char newDataPath[100] = "../processed/";
    strcpy(dataPath, config_file.path_files);
    strcat(dataPath, "/");

    FILE *sucursal_file = fopen(strcat(dataPath, file->file_name), "r"); // Archivo sucursal a procesar
    FILE *consolidated_file = fopen(config_file.inventory_file, "a");    // Archivo consolidado

    if (sucursal_file == NULL)
    {
        printf("Error al abrir el archivo de la sucursal.\n");
        return;
    }

    if (consolidated_file == NULL)
    {
        printf("Error al abrir el archivo consolidado.\n");
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
    int fileDescriptor, watchDescriptor;
    char buffer[BUFFER_LENGTH];

    // Se inicializa el descriptor del inotify
    fileDescriptor = inotify_init();

    if (fileDescriptor < 0)
    {   // Se comprueba que se inicialice el descriptor
        printf("Error initializing inotify descriptor");
        exit(EXIT_FAILURE); // Si no se inincializa, avisa y finaliza el proceso con error
    }

    // Se establece el directorio a monitorear
    watchDescriptor = inotify_add_watch(fileDescriptor, config_file.path_files, IN_CREATE);

    if (watchDescriptor < 0)
    {   // Se comprueba que se inicialice el watcher
        printf("Error initializing inotify watcher");
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
        {   // Se comprueba que se inicialice correctamente
            printf("Error initializing inotify length");
            exit(EXIT_FAILURE); // Si no se inincializa, avisa y finaliza el proceso con error
        }

        // Se procesan los eventos
        while (i < length)
        {
            char newNotificationScreen[600] = "\n\n\n"
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

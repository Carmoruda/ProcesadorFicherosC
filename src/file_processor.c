#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#define CONFIG_PATH "./fp.conf" // Ruta del archivo de configuración

pthread_cond_t cond;   // Variable de condición de los hilos
pthread_mutex_t mutex; // Mutex para la exclusión mutua

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

/// @brief Procesa los ficheros de las sucursales y los escribe en los ficheros de log y consolidado
/// @param file Archivo de la sucursal a procesar
void processFiles(sucursal_file *file);

int main()
{
    sucursal_file *nueva_sucursal;

    char dataPath[100];
    strcpy(dataPath, config_file.path_files);
    strcat(dataPath, "/");

    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);

    // Leer archivo de configuración
    FILE *file = fopen(CONFIG_PATH, "r");
    readConfigFile(file);
    pthread_t th1, th2, th3, th4;

    printf("Path files: %s\n", config_file.path_files);

    struct dirent *directorio;
    DIR *folder;

    folder = opendir(config_file.path_files);

    if (folder == NULL)
    {
        printf("Error al abrir el directorio.");
        return -1;
    }

    while (1)
    {
        while (directorio = readdir(folder))
        {
            switch (directorio->d_name[4])
            {
            case '1':
                nueva_sucursal = newFile(directorio->d_name, 1);    // Añadimos un archivo de la sucursal 1 a la lista
                pthread_create(&th1, NULL, reader, nueva_sucursal); // Crear hilo 1
                break;
            case '2':
                nueva_sucursal = newFile(directorio->d_name, 2);    // Añadimos un archivo de la sucursal 2 a la lista
                pthread_create(&th2, NULL, reader, nueva_sucursal); // Crear hilo 2
                break;
            case '3':
                nueva_sucursal = newFile(directorio->d_name, 3);    // Añadimos un archivo de la sucursal 3 a la lista
                pthread_create(&th3, NULL, reader, nueva_sucursal); // Crear hilo 3
                break;
            case '4':
                nueva_sucursal = newFile(directorio->d_name, 4);    // Añadimos un archivo de la sucursal 4 a la lista
                pthread_create(&th4, NULL, reader, nueva_sucursal); // Crear hilo 4
                break;
            default:
                break;
            }

            strcat(dataPath, directorio->d_name); // Concatenar el nombre del archivo al path
            // remove(dataPath);                     // Eliminar el archivo

            strcpy(dataPath, config_file.path_files);
            strcat(dataPath, "/");
            sleep(1);
        }
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
    pthread_mutex_lock(&mutex); // Bloquear el mutex

    processFiles((sucursal_file *)file); // Procesar el archivo
    printf("Archivo leído: %s\n", ((sucursal_file *)file)->file_name);

    pthread_mutex_unlock(&mutex); // Desbloquear el mutex

    pthread_exit(NULL); // Salir del hilo
}

void processFiles(sucursal_file *file)
{
    char line[256];                // Línea del fichero
    time_t time_date = time(NULL); // Dato de tiempo
    char control;                  // Control de lectura

    char dataPath[100];
    strcpy(dataPath, config_file.path_files);
    strcat(dataPath, "/");

    FILE *sucursal_file = fopen(strcat(dataPath, file->file_name), "r"); // Archivo sucursal a procesar
    FILE *log_file = fopen(config_file.log_file, "a");                   // Archivo de log
    FILE *consolidated_file = fopen(config_file.inventory_file, "a");    // Archivo consolidado

    if (file == NULL)
    {
        printf("Error al abrir el archivo de la sucursal.\n");
        return;
    }

    if (log_file == NULL)
    {
        printf("Error al abrir el archivo de log.\n");
        return;
    }

    if (consolidated_file == NULL)
    {
        printf("Error al abrir el archivo consolidado.\n");
        return;
    }

    // Formato fichero sucursal     -> ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO
    // Formato fichero consolidado  -> ID_SUCURSAL;ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO
    // Formato fichero log          -> DD/MM/AAAA:::HH:MM:SS:::INICIO:::FIN:::NOMBRE_FICHERO:::NUMOPERACIONESCONSOLIDADAS

    // Bucle que leerá el fichero hasta que no haya más información.
    while (fgets(line, sizeof(line), sucursal_file))
    {
        fprintf(consolidated_file, "%d;%s", file->sucursal_number, line); // Escribir en el fichero consolidado
        file->num_operations++;                                           // Incrementar el número de operaciones
    }

    struct tm current_time = *localtime(&time_date);                                                                                                                                                                                         // Fecha y hora actual
    fprintf(log_file, "%d/%d/%d:::%d:%d:%d:::%s:::%d\n", current_time.tm_mday, current_time.tm_mon + 1, current_time.tm_year + 1900, current_time.tm_hour, current_time.tm_min, current_time.tm_sec, file->file_name, file->num_operations); // Escribir en el archivo de log

    fclose(sucursal_file);
    fclose(log_file);
    fclose(consolidated_file);
}

void notifyNewFile(){
    printf("###               ###   ############   ###                           ###\n");
    printf("### ###           ###   ############    ###                         ###\n");
    printf("###   ###         ###   ###              ###                       ###\n");
    printf("###     ###       ###   ############      ###                     ###\n");
    printf("###       ###     ###   ############       ###        ###        ###\n");
    printf("###         ###   ###   ###                 ###     ### ###     ###\n");
    printf("###           ### ###   ############         ### ###       ### ###\n");
    printf("###               ###   ############          ###            ###\n");
}

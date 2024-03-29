#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define DATA_PATH "../data/"
#define CONFIG_PATH "../fp.conf" // Ruta del archivo de configuración

pthread_cond_t cond;   // Variable de condición de los hilos
pthread_mutex_t mutex; // Mutex para la exclusión mutua

typedef struct sucursal_file
{
    char file_name[100]; // Nombre del fichero
    int sucursal_number; // Número de la sucursal
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

sucursal_file archivos[100];
int contador;

/// @brief Leer la información del archivo de configuración
/// @param pf_config Archivo de configuración
void readConfigFile(FILE *pf_config);

/// @brief Lee un nuevo archivo del directorio
void *reader();

/// @brief Crea un nuevo archivo
/// @param file_name Nombre del archivo
/// @param sucursal_number Número de la sucursal
void newFile(char *file_name, int sucursal_number);

int main()
{
    contador = 0;
    char dataPath[100];
    strcpy(dataPath, DATA_PATH);
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
                newFile(directorio->d_name, 1);           // Añadimos un archivo de la sucursal 1 a la lista
                pthread_create(&th1, NULL, reader, NULL); // Crear hilo 1
                break;
            case '2':
                newFile(directorio->d_name, 2);           // Añadimos un archivo de la sucursal 2 a la lista
                pthread_create(&th2, NULL, reader, NULL); // Crear hilo 2
                break;
            case '3':
                newFile(directorio->d_name, 3);           // Añadimos un archivo de la sucursal 3 a la lista
                pthread_create(&th3, NULL, reader, NULL); // Crear hilo 3
                break;
            case '4':
                newFile(directorio->d_name, 4);           // Añadimos un archivo de la sucursal 4 a la lista
                pthread_create(&th4, NULL, reader, NULL); // Crear hilo 4
                break;
            default:
                break;
            }

            strcat(dataPath, directorio->d_name); // Concatenar el nombre del archivo al path
            remove(dataPath);                     // Eliminar el archivo

            strcpy(dataPath, DATA_PATH);
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

void newFile(char *file_name, int sucursal_number)
{
    sucursal_file *nueva_sucursal = (sucursal_file *)malloc(sizeof(sucursal_file)); // Reservamos memoria para el nuevo fichero

    strcpy(nueva_sucursal->file_name, file_name);      // Copiamos el nombre del fichero
    nueva_sucursal->sucursal_number = sucursal_number; // Copiamos el número de la sucursal

    // Bloqueamos el mutex para evitar problemas de concurrencia
    pthread_mutex_lock(&mutex);

    // Agregamos el archivo a la lista de archivos
    archivos[contador] = *nueva_sucursal;
    contador++;

    // Desbloqueamos el mutex
    pthread_mutex_unlock(&mutex);

    // Avisamos a los hilos que deben comprar si hay un nuevo archivo
    pthread_cond_signal(&cond);
}

void *reader()
{
    pthread_mutex_lock(&mutex); // Bloquear el mutex

    while (contador == 0) // Mientras no haya archivos
    {
        pthread_cond_wait(&cond, &mutex); // Esperar a que haya archivos
    }

    // Hilo del Patron 1

    printf("Archivo leído: %s\n", archivos[contador - 1].file_name);

    pthread_mutex_unlock(&mutex); // Desbloquear el mutex

    pthread_exit(NULL); // Salir del hilo
}

void patron1(void *arg)
{
}

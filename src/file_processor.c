#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define CONFIG_PATH "../fp.conf" // Ruta del archivo de configuración

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
/// @return 0 si se ejecuta correctamente
int readConfigFile(FILE *pf_config);

void *hilo1(void *arg);
void *hilo2(void *arg);
void *hilo3(void *arg);
void *hilo4(void *arg);

int main()
{
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

    while (directorio = readdir(folder))
    {
        switch (directorio->d_name[4])
        {
        case '1':
            pthread_create(&th1, NULL, hilo1, NULL);
            break;
        case '2':
            pthread_create(&th2, NULL, hilo2, NULL);
            break;
        case '3':
            pthread_create(&th3, NULL, hilo3, NULL);
            break;
        case '4':
            pthread_create(&th4, NULL, hilo4, NULL);
            break;
        default:
            break;
        }
    }

    printf("Hilos creados\n");
    return 0;
}
void *hilo1(void *arg)
{
    printf("Hilo 1\n");
}
void *hilo2(void *arg)
{
    printf("Hilo 2\n");
}
void *hilo3(void *arg)
{
    printf("Hilo 3\n");
}
void *hilo4(void *arg)
{
    printf("Hilo 4\n");
}
int readConfigFile(FILE *pf_config)
{
    int contador = 0;
    char line[256];

    while (fgets(line, sizeof(line), pf_config))
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
    return contador;
}

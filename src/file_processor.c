#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_PATH "../fp.conf" // Ruta del archivo de configuración

/// @brief Estructura que contiene la información del archivo de configuración
struct config_file
{
    char path_files[100];
    char inventory_file[100];
    char log_file[100];
    int num_processes;
    int simulate_sleep_max;
    int simulate_sleep_min;
} config_file;

/// @brief Leer la información del archivo de configuración
/// @param pf_config Archivo de configuración
/// @return 0 si se ejecuta correctamente
int readConfigFile(FILE *pf_config);

int main()
{
    // Leer archivo de configuración
    FILE *file = fopen(CONFIG_PATH, "r");
    readConfigFile(file);

    return 0;
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
}

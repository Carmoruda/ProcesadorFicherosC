#include "../include/check_patterns.h"
#include "../include/program_data.h"
#include <stdio.h>
pthread_mutex_t mutexPatterns; // Mutex para el control de patrones
pthread_mutex_t mutexLog;      // Mutex para el acceso al log
pthread_mutex_t mutexPatterns;

/// @brief Compara dos operaciones para la funcion qsort, que ordena el fichero csv en orden de usuario.
/// @param a Primera operación.
/// @param b Segunda operación.
/// @return Devuelve un 1 si las usuarios coinciden, y un 0 en caso contrario.
int comparar_registros(const void *a, const void *b);

/// @brief Compara dos fechas de inicio de dos operaciones para verificar si se cumple la condicion del patron1.
/// @param fecha1 Primera fecha
/// @param fecha2 Segunda fecha.
/// @return Devuelve un 1 si las fechas estan en la misma hora, y un 0 en caso contrario.
int enLaMismaHora(char *fecha1, char *fecha2);



int checkPatternsProcess(pthread_mutex_t mutexLogFile, char *log_file, char *consolidated_file)
{
    printf("AD");
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
        int a;
        a = pthread_create(&th_pattern1, NULL, pattern1, NULL);
        printf("%d", a);
        th_pattern2 = pthread_create(&th_pattern2, NULL, pattern2, NULL);
        th_pattern3 = pthread_create(&th_pattern3, NULL, pattern3, NULL);
        th_pattern4 = pthread_create(&th_pattern4, NULL, pattern4, NULL);
        th_pattern5 = pthread_create(&th_pattern5, NULL, pattern5, NULL);

    }

        pthread_join(th_pattern1, NULL);
        pthread_join(th_pattern2, NULL);
        pthread_join(th_pattern3, NULL);
        pthread_join(th_pattern4, NULL);
        pthread_join(th_pattern5, NULL);
    return 0;
}

// --- Pattern 1 ---

void *pattern1(void *arg)
{
    printf("HOLA");
    char *ficheroCSV = "../output/fich_consolidado.csv";

    // Abrir el archivo en modo lectura y escritura
    FILE *archivo = fopen(ficheroCSV, "r+");
    if (archivo == NULL)
    {
        perror("Error al abrir el archivo");
        pthread_exit(NULL);
    }

    // Bloquear el mutex antes de acceder al archivo
    //pthread_mutex_lock(&mutexPatterns);

    // Leer los registros del archivo y almacenarlos en una matriz
    /*struct Operacion registros[MAX_RECORDS];
    int num_registros = 0;
    char linea[MAX_LINE_LENGTH];
    while (fgets(linea, sizeof(linea), archivo) != NULL && num_registros < MAX_RECORDS)
    {
        sscanf(linea, "%d;%[^;];%[^;];%[^;];%[^;];%[^;];%d;%f;%[^;]",
               &registros[num_registros].Sucursal,
               registros[num_registros].IdOperacion,
               registros[num_registros].FECHA_INICIO,
               registros[num_registros].FECHA_FIN,
               registros[num_registros].IdUsuario,
               registros[num_registros].IdTipoOperacion,
               &registros[num_registros].NoOperacion,
               &registros[num_registros].Importe,
               registros[num_registros].Estado);
        num_registros++;

    }
    // Cerrar el archivo
    fclose(archivo);
    qsort(registros, num_registros, sizeof(struct Operacion), comparar_registros);

    int contadorOperaciones = 0;
    char* ultimoUsuario;
    char* ultimoTiempo;

    for (int i = 0; i < num_registros; i++)
    {
        // Verificar si es la misma persona y si la operación está dentro del rango de una hora
        if (strcmp(registros[i].IdUsuario,ultimoUsuario) && enLaMismaHora(registros[i].FECHA_INICIO, ultimoTiempo) == 1)
        {
            contadorOperaciones++;
            // Si el usuario realiza 5 o más operaciones dentro de una hora, hacer algo
            if (contadorOperaciones >= 5)
            {
                // Aquí puedes imprimir un mensaje, tomar alguna acción, etc.
                printf("Usuario %d realizó 5 o más operaciones dentro de una hora.\n", registros[i].IdUsuario);
            }
        }
        else
        {
            // Reiniciar el contador de operaciones para un nuevo usuario o una nueva hora
            contadorOperaciones = 1;
        }

        // Actualizar el usuario y el tiempo para la próxima iteración
        strcpy(ultimoUsuario, registros[i].IdUsuario);
        strcpy(ultimoTiempo, registros[i].FECHA_INICIO);
    }
    // Mostrar los registros ordenados por pantalla
    for (int i = 0; i < num_registros; i++)
    {
        printf("Sucursal: %d,IdOperacion: %s, FECHA_INICIO: %s, FECHA_FIN: %s, IdUsuario: %s, IdTipoOperacion: %s, NoOperacion: %d, Importe: %.2f, Estado: %s\n",
                            registros[i].Sucursal,
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
    //pthread_mutex_unlock(&mutexPatterns);
*/
    pthread_exit(NULL);
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

// Función de apoyo a la función de qsort, para ordenar por usuarios.
int comparar_registros(const void *a, const void *b)
{
    const struct Operacion *registro1 = (const struct Operacion *)a;
    const struct Operacion *registro2 = (const struct Operacion *)b;

    return strcmp(registro1->IdUsuario, registro2->IdUsuario);
}

// Función para verificar si se superan las 5 operaciones por hora
int enLaMismaHora(char *fecha1, char *fecha2)
{
    time_t f1, f2, diferencia;
    f1 = (long)fecha1;
    f2 = (long)fecha2;
    unsigned long long int horas = diferencia / 60 / 60;
    diferencia -= 60 * 60 * horas;
    unsigned long long int minutos = diferencia / 60;
    diferencia -= 60 * minutos;

    if (diferencia <= 3600)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

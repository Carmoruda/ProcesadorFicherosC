#include "../include/check_patterns.h"
#include "../include/program_data.h"
#include <stdio.h>

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
        pthread_create(&th_pattern3, NULL, pattern3, NULL);
        pthread_create(&th_pattern4, NULL, pattern4, NULL);
        pthread_create(&th_pattern5, NULL, pattern5, NULL);
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
        if (strcmp(registros[i].IdUsuario, ultimoUsuario) &&
            enLaMismaHora(registros[i].FECHA_INICIO, ultimoTiempo) == 1 && registros[i].flag == 0)
        {
            reg_patrones[contadorOperaciones] = registros[i];
            contadorOperaciones++;

            // Si el usuario realiza 5 o más operaciones dentro de una hora, hacer
            // algo
            if (contadorOperaciones >= 5)
            {
                cumpleCondicion = true;

            }
            printf("EEEE%s\n\n", registros[i].IdUsuario);
        }
        else if(strcmp(registros[i].IdUsuario, ultimoUsuario) == 0)
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
        if (strcmp(registros[i].IdUsuario, ultimoUsuario) &&
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
*/
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
                printf("Datos de la operacion que provoca el patron 3:\n");
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
        if (strcmp(registros[i].IdUsuario, ultimoUsuario) &&
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
        else
        {
            if (registros[i - 1].DineroRet > registros[i - 1].DineroIngr)
            {
                printf("El usuario %d ha ingresado menos de lo que ha retirado en un "
                       "mismo día.",
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
        qsort(registros, num_registros, sizeof(struct Operacion), comparar_por_fecha_inicio);
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
// Función de comparación para ordenar por fecha de inicio
int comparar_por_fecha_inicio(const void *a, const void *b)
{
    const struct Operacion *op1 = (const struct Operacion *)a;
    const struct Operacion *op2 = (const struct Operacion *)b;
    return strcmp(op1->FECHA_INICIO, op2->FECHA_INICIO);
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

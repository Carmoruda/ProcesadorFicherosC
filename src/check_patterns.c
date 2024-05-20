#include "../include/check_patterns.h"
#include <stdio.h>
pthread_mutex_t mutexPatterns; // Mutex para el control de patrones
pthread_mutex_t mutexLog;      // Mutex para el acceso al log

struct Operacion registros[MAX_RECORDS];
struct Operacion reg_patrones[MAX_RECORDS];

int num_registros;

/// @brief Compara dos operaciones para la funcion qsort, que ordena el fichero csv en orden de usuario.
/// @param a Primera operación.
/// @param b Segunda operación.
/// @return Devuelve un 1 si las usuarios coinciden, y un 0 en caso contrario.
int comparar_registros(const void *a, const void *b);

int comparar_por_fecha_inicio(const void *a, const void *b);

/// @brief Compara dos fechas de inicio de dos operaciones para verificar si se cumple la condicion del patron1.
/// @param fecha1 Primera fecha
/// @param fecha2 Segunda fecha.
/// @return Devuelve un 1 si las fechas estan en la misma hora, y un 0 en caso contrario.
int enLaMismaHora(char *fecha1, char *fecha2);

/// @brief Compara dos fechas de inicio de dos operaciones para verificar si se cumple la condicion del mismo día.
/// @param fecha1 Primera fecha
/// @param fecha2 Segunda fecha.
/// @return Devuelve un 1 si las fechas estan en el mismo día, y un 0 en caso contrario.
int enElMismoDía(char *fecha1, char *fecha2);

/// @brief Vuelca todas las operaciones procesadas del fichero consolidado en un vector de estructuras, para poder trabajar sobre el posteriormente.
/// @return Devuelve el número de operaciones que hay en el consolidado.
int readConsolidatedFile();

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
  char ultimoTiempo[100];
  bool cumpleCondicion = false;

  for (int i = 0; i < num_registros; i++)
  {

    // Verificar si es la misma persona y si la operación está dentro del rango de una hora
    if (strcmp(registros[i].IdUsuario, ultimoUsuario) && enLaMismaHora(registros[i].FECHA_INICIO, ultimoTiempo) == 1)
    {

      reg_patrones[contadorOperaciones] = registros[i];
      contadorOperaciones++;

      // Si el usuario realiza 5 o más operaciones dentro de una hora, hacer algo
      if (contadorOperaciones >= 5)
      {
        cumpleCondicion = true;
      }
    }
    else
    {

      if (cumpleCondicion == true)
      {

        for (int i = 0; i < contadorOperaciones; i++)
        {
          printf("Datos de la operacion que provoca el patron 1:\n  Sucursal: %d,IdOperacion: %s, FECHA_INICIO: %s, FECHA_FIN: %s, IdUsuario: %s, IdTipoOperacion: %s, NoOperacion: %d, Importe: %.2f, Estado: %s\n",
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
        printf("FIN DEL PATRON\n\n\n\n");
        cumpleCondicion = false;
      }

      // Reiniciar el contador de operaciones para un nuevo usuario o una nueva hora
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
  char ultimoUsuario[20];
  char ultimoTiempo[20];
  bool cumpleCondicion = false;

  for (int i = 0; i < num_registros; i++)
  {

    // Verificar si es la misma persona y si la operación está dentro del rango de una hora
    if (strcmp(registros[i].IdUsuario, ultimoUsuario) && enElMismoDía(registros[i].FECHA_INICIO, ultimoTiempo) == 1 && registros[i].Importe < 0)
    {
      printf("mismo user y mismo dia");
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
        for (int i = 0; i < contadorOperaciones; i++)
        {
          printf("Datos de operacion que provoca el patron 2:\n  Sucursal: %d,IdOperacion: %s, FECHA_INICIO: %s, FECHA_FIN: %s, IdUsuario: %s, IdTipoOperacion: %s, NoOperacion: %d, Importe: %.2f, Estado: %s\n",
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
        printf("FIN PATRON 2\n\n\n\n");
        cumpleCondicion = false;
      }

      // Reiniciar el contador de operaciones para un nuevo usuario o una nueva hora
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

  int contadorOperaciones = 0;
  char ultimoUsuario[20];
  char ultimoTiempo[20];
  bool cumpleCondicion = false;

  for (int i = 0; i < num_registros; i++)
  {
    // Verificar si es la misma persona y si la operación está dentro del rango de una hora
    if (strcmp(registros[i].IdUsuario, ultimoUsuario) && enElMismoDía(registros[i].FECHA_INICIO, ultimoTiempo) == 1 && strcmp(registros[i].Estado, "Error"))
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
        printf("Datos de las operaciones que cumplen con el patron 3:\n");
        for (int i = 0; i < contadorOperaciones; i++)
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
        cumpleCondicion = false;
      }

      // Reiniciar el contador de operaciones para un nuevo usuario o una nueva hora
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

  int contadorOperaciones = 0;
  char ultimoUsuario[20];
  char ultimoTiempo[20];
  int cumpleCondicion[4] = {0, 0, 0, 0};
  char tipo;

  for (int i = 0; i < num_registros; i++)
  {
    contadorOperaciones++;
    // Verificar si es la misma persona y si la operación está dentro del rango de un dia
    if (strcmp(registros[i].IdUsuario, ultimoUsuario) && enElMismoDía(registros[i].FECHA_INICIO, ultimoTiempo) == 1)
    {

      tipo = registros[i].IdTipoOperacion[7];
      switch (tipo)
      {
      case '1':
        cumpleCondicion[0] = 1;
        break;
      case '2':
        cumpleCondicion[1] = 1;
        break;
      case '3':
        cumpleCondicion[2] = 1;
        break;
      case '4':
        cumpleCondicion[3] = 1;
        break;
      }
    }

    else
    {
      printf("Estoy\n\n");
      if (cumpleCondicion[0] == cumpleCondicion[1] == cumpleCondicion[2] == cumpleCondicion[3] == 1)
      {
        printf("Datos de operacion que provoca el patron 3:\n");
        for (int i = 0; i < contadorOperaciones; i++)
        {
          printf("ucursal: %d,IdOperacion: %s, FECHA_INICIO: %s, FECHA_FIN: %s, IdUsuario: %s, IdTipoOperacion: %s, NoOperacion: %d, Importe: %.2f, Estado: %s\n",
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
        cumpleCondicion[0] = cumpleCondicion[1] = cumpleCondicion[2] = cumpleCondicion[3] = 0;
      }
    }

    // Actualizar el usuario y el tiempo para la próxima iteración
    strcpy(ultimoUsuario, registros[i].IdUsuario);
    strcpy(ultimoTiempo, registros[i].FECHA_INICIO);
  }

  pthread_exit(NULL);
}

void *pattern5(void *arg)
{

  int contadorOperaciones = 0;
  char ultimoUsuario[20];
  char ultimoTiempo[20];

  for (int i = 0; i < num_registros; i++)
  {
    // Verificar si es la misma persona y si la operación está dentro del rango de una hora
    if (strcmp(registros[i].IdUsuario, ultimoUsuario) && enElMismoDía(registros[i].FECHA_INICIO, ultimoTiempo) == 1)
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
        printf("El usuario %d ha ingresado menos de lo que ha retirado en un mismo día.", registros[i - 1].IdUsuario);
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
    registros[num_registros].DineroIngr = 0;
    registros[num_registros].DineroRet = 0;
    num_registros++;
  }
  // Cerrar el archivo
  fclose(archivo);

  // Desbloquear el mutex después de acceder al archivo
  // pthread_mutex_unlock(&mutexPatterns);

  // Ordenar el vector por fecha de inicio y usuario
  qsort(registros, num_registros, sizeof(struct Operacion), comparar_por_fecha_inicio);
  qsort(registros, num_registros, sizeof(struct Operacion), comparar_registros);

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
  time_t f1, f2, diferencia;
  f1 = (long)fecha1;
  f2 = (long)fecha2;
  diferencia = f1 - f2;
  unsigned long long int horas = diferencia / 60 / 60;
  diferencia -= 60 * 60 * horas;
  unsigned long long int minutos = diferencia / 60;
  diferencia -= 60 * minutos;
  return diferencia <= 3600 ? 1 : 0;
}

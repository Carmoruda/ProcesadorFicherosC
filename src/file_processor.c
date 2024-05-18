#include "../include/check_patterns.h"
#include "../include/error_messages.h"
#include "../include/program_data.h"
#include "../include/show_information.h"
#include <sys/inotify.h>
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

pthread_cond_t cond;          // Variable de condición de los hilos
pthread_mutex_t mutex;        // Mutex para la exclusión mutua
pthread_mutex_t mutexLogFile; // Mutex para el escritura en el archivo de log
sem_t sem_thread_creation;    // Semáforo para controlar la creación de hilos

DIR *folder;     // Directorio raíz de archivos de las sucursales
DIR *folder_SUC; // Directorio de archivos de la sucursal.

/// @brief Estructura que contiene la información de los archivos de las
/// sucursales
typedef struct sucursal_file
{
  char file_path[100];  // Ruta del fichero
  char file_name[100];  // Nombre del fichero
  char sucursal_number; // Número de la sucursal
  int num_operations;   // Número de operaciones realizadas
} sucursal_file;

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
void processFiles(sucursal_file *file);

/// @brief Verifica la llegada de nuevos archivos al directorio común
/// @param file_dir Directorio del que queremos verificar la llegada de nuevos archivos
void *verifyNewFile(void *file_dir);

/// @brief Controla la logica detrás del procesado de directorios de sucursales.
int processFilesProcess();

/// @brief Procesa los ficheros de un directorio concreto
/// @param folder_name Nombre del directorio que queremos procesar
void *processSucursalDirectory(void *folder_name);

/// @brief Muestra el menú de inicio del programa
/// @param error_flag flag de error en la opción del usuario
/// @return Opción elegida
int menu(int error_flag);

/// @brief Da comienzo a la auditoria
void StartAudit();

int main()
{
  // Leer archivo de configuración
  FILE *file = fopen(CONFIG_PATH, "r");
  readConfigFile(file);
  char *args[] = {"./create_structure.sh", NULL};
  bool isProgramRunning = true;

  do
  {

    switch (menu(0))
    {
    case 1: // Ejecutar script creación estructura directorios
      pid_t pid = fork();
      if (pid == -1)
      {
        perror("fork failed");
        return 1;
      }
      else if (pid == 0)
      {
        execvp(args[0], args);
        perror("execvp failed");
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
    // checkPatternsProcess(mutexLogFile, config_file.log_file, config_file.inventory_file);
  }
}

int menu(int error_flag)
{
  int opcion;
  printf("UFV AUDITA\n\n");

  if (error_flag == 1)
  {
    printf("Seleccione una opción válida.\n");
  }

  printf("1. Crear estructura de ficheros\n2. Iniciar\n3. Salir\n\n=> ");
  scanf("%d", &opcion);
  return opcion > 3 || opcion < 1 ? menu(1) : opcion;
}

void readConfigFile(FILE *pf_config)
{
  int contador = 0;
  char line[256];

  while (fgets(line, sizeof(line),
               pf_config)) // Leer línea por línea el archivo de configuración.
  {
    char *token = strtok(line, "="); // Separar la línea por el signo de igual.

    while (token != NULL) // Mientras queden palabras en la linea.
    {

      token[strcspn(token, "\n")] = '\0'; // Elimina el salto de línea.

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

      contador++;
      token = strtok(NULL, " "); // Siguiente palabra.
    }
  }
}

sucursal_file *newFile(char *file_path, char *file_name, char sucursal_number)
{
  sucursal_file *nueva_sucursal = (sucursal_file *)malloc(sizeof(sucursal_file)); // Reservamos memoria para el nuevo fichero

  strcpy(nueva_sucursal->file_path, file_path);      // Copiamos el nombre del fichero
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
  sleep(rand() % config_file.simulate_sleep_max +
        config_file.simulate_sleep_min);

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

  char newDataPath[100] = "../files_data/processed/";

  FILE *sucursal_file = fopen(file->file_path, "r");                // Archivo sucursal a procesar
  FILE *consolidated_file = fopen(config_file.inventory_file, "a"); // Archivo consolidado

  if (sucursal_file == NULL)
  {
    sprintf(logString, SUCURSAL_ERROR, file->file_path);
    printLogScreen(mutexLogFile, config_file.log_file, logString, logString);
    return;
  }

  if (consolidated_file == NULL)
  {
    printLogScreen(mutexLogFile, config_file.log_file, CONSOLIDADO_OPEN_ERROR, CONSOLIDADO_OPEN_ERROR);
    return;
  }

  // Formato fichero sucursal    ->
  // ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO
  // Formato fichero consolidado ->
  // ID_SUCURSAL;ID_OPERACIÓN;FECHA_INI;FECHA_FIN;ID_USUARIO;ID_TIPO_OPERACIÓN;NUM_OPERACIÓN;IMPORTE;ESTADO
  // Formato fichero log         ->
  // DD/MM/AAAA:::HH:MM:SS:::INICIO:::FIN:::NOMBRE_FICHERO:::NUMOPERACIONESCONSOLIDADAS

  // Bucle que leerá el fichero hasta que no haya más información.
  pthread_mutex_lock(&mutex); // Bloquear el mutex

  struct tm current_time = *localtime(&time_date); // Fecha y hora actual

  while (fgets(line, sizeof(line), sucursal_file))
  {
    fprintf(consolidated_file, "%c;%s", file->sucursal_number,
            line);          // Escribir en el fichero consolidado
    file->num_operations++; // Incrementar el número de operaciones
  }

  sprintf(logString, "%d/%d/%d:::%d:%d:%d:::%s:::%d", current_time.tm_mday,
          current_time.tm_mon + 1, current_time.tm_year + 1900,
          current_time.tm_hour, current_time.tm_min, current_time.tm_sec,
          file->file_path, file->num_operations);
  sprintf(screenString,
          "Fichero procesado: %s, con %d operaciones consolidadas",
          file->file_path, file->num_operations);

  printLogScreen(mutexLogFile, config_file.log_file, logString,
                 screenString); // Imprimir en el log

  fclose(sucursal_file);
  fclose(consolidated_file);

  rename(
      file->file_path,
      strcat(newDataPath,
             file->file_name)); // Mover el archivo a la carpeta de procesados
  pthread_mutex_unlock(&mutex); // Desbloquear el mutex
}

void *verifyNewFile(void *file_dir)
{
  char *logString; // Mensaje a escribir en el log
  int fileDescriptor, watchDescriptor;
  char buffer[BUFFER_LENGTH];

  // Se inicializa el descriptor del inotify
  fileDescriptor = inotify_init();

  if (fileDescriptor < 0)
  { // Se comprueba que se inicialice el descriptor
    printLogScreen(mutexLogFile, file_dir, INOTIFY_DESCRIPTOR_ERROR, INOTIFY_DESCRIPTOR_ERROR);
    exit(EXIT_FAILURE); // Si no se inincializa, avisa y finaliza el proceso con error
  }

  // Se establece el directorio a monitorear
  watchDescriptor = inotify_add_watch(fileDescriptor, file_dir, IN_CREATE);

  if (watchDescriptor < 0)
  { // Se comprueba que se inicialice el watcher
    printLogScreen(mutexLogFile, file_dir, INOTIFY_WATCHER_ERROR, INOTIFY_WATCHER_ERROR);
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
      printLogScreen(mutexLogFile, file_dir, INOTIFY_LENGTH_ERROR, INOTIFY_LENGTH_ERROR);
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
        printLogScreen(mutexLogFile, config_file.log_file, newNotificationLog, newNotificationScreen); // Imprimir en el log
        closedir(folder);                                                                              // Cerrar el directorio
        folder = opendir(config_file.path_files);                                                      // Abrir el directorio de nuevo
      }

      i += EVENT_SIZE + event->len; // Se actualiza el tamaño
    }
  }
}

// --- PROCESS FUNCTIONS ---

int processFilesProcess()
{
  sucursal_file *nueva_sucursal;

  // Path a los archivos
  char *suc_1_dir_name = malloc(200 * sizeof(char));
  char *suc_2_dir_name = malloc(200 * sizeof(char));
  char *suc_3_dir_name = malloc(200 * sizeof(char));
  char *suc_4_dir_name = malloc(200 * sizeof(char));

  sprintf(suc_1_dir_name, "%s/%s1/", config_file.path_files, config_file.suc_dir);
  sprintf(suc_2_dir_name, "%s/%s2/", config_file.path_files, config_file.suc_dir);
  sprintf(suc_3_dir_name, "%s/%s3/", config_file.path_files, config_file.suc_dir);
  sprintf(suc_4_dir_name, "%s/%s4/", config_file.path_files, config_file.suc_dir);

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

  sprintf(logString,
          "%d/%d/%d:::%d:%d:%d:::PROCESAMIENTO ARCHIVOS INICIADO:::DIRECTORIO %s",
          current_time.tm_mday, current_time.tm_mon + 1,
          current_time.tm_year + 1900, current_time.tm_hour, current_time.tm_min,
          current_time.tm_sec, config_file.path_files);

  sprintf(screenString,
          "Procesamiento de archivos iniciado en el directorio: %s",
          config_file.path_files);

  printLogScreen(mutexLogFile, config_file.log_file, logString, screenString);

  // Se inicializa un hilo encargado de comprobar la llegada de nuevos archivos
  pthread_t newFileThread_SUC1, newFileThread_SUC2, newFileThread_SUC3, newFileThread_SUC4;

  int controler_SUC1, controler_SUC2, controler_SUC3, controler_SUC4;

  controler_SUC1 = pthread_create(&newFileThread_SUC1, NULL, verifyNewFile, suc_1_dir_name);
  controler_SUC2 = pthread_create(&newFileThread_SUC2, NULL, verifyNewFile, suc_2_dir_name);
  controler_SUC3 = pthread_create(&newFileThread_SUC3, NULL, verifyNewFile, suc_3_dir_name);
  controler_SUC4 = pthread_create(&newFileThread_SUC4, NULL, verifyNewFile, suc_4_dir_name);

  if (controler_SUC1 < 0 || controler_SUC2 < 0 || controler_SUC3 < 0 || controler_SUC4 < 0)
  {
    printLogScreen(mutexLogFile, config_file.log_file, VERIFIER_THREAD_ERROR, VERIFIER_THREAD_ERROR);
  }

  // Se inicializa un semáforo para sincronizar el procesado de archivos
  sem_init(&sem_thread_creation, 0, config_file.num_processes);

  pthread_create(&th1, NULL, processSucursalDirectory, suc_1_dir_name); // Crear hilo sucursal 1
  pthread_create(&th2, NULL, processSucursalDirectory, suc_2_dir_name); // Crear hilo sucursal 2
  pthread_create(&th3, NULL, processSucursalDirectory, suc_3_dir_name); // Crear hilo sucursal 3
  pthread_create(&th4, NULL, processSucursalDirectory, suc_4_dir_name); // Crear hilo sucursal 4

  pthread_join(th1, NULL);
  pthread_join(th2, NULL);
  pthread_join(th3, NULL);
  pthread_join(th4, NULL);

  pthread_join(newFileThread_SUC1, NULL);
  pthread_join(newFileThread_SUC2, NULL);
  pthread_join(newFileThread_SUC3, NULL);
  pthread_join(newFileThread_SUC4, NULL);

  return 0;
}

void *processSucursalDirectory(void *folder_name)
{
  // Inicializar hilo
  pthread_t th;

  // Strings imprimir
  char *logString = malloc(600 * sizeof(char));    // Mensaje a mostrar en el log
  char *screenString = malloc(600 * sizeof(char)); // Mensaje a mostrar por pantalla

  char filePath[100];
  struct dirent *directorio = malloc(sizeof(struct dirent));
  sucursal_file *nueva_sucursal = (sucursal_file *)malloc(sizeof(sucursal_file)); // Reservamos memoria para el nuevo fichero

  folder = opendir(folder_name);

  if (folder == NULL)
  {
    sprintf(logString, FOLDER_OPEN_ERROR, folder);
    printLogScreen(mutexLogFile, config_file.log_file, logString, logString);
  }

  while (1)
  {
    while ((directorio = readdir(folder)) != NULL)
    {

      if (directorio->d_type == DT_REG) // Comprobar que sea un archivo
      {
        sprintf(filePath, "%s%s", folder_name, directorio->d_name);
        nueva_sucursal = newFile(filePath, directorio->d_name, filePath[19]); // Añadimos un archivo de la sucursal 1 a la lista
        sem_wait(&sem_thread_creation);
        pthread_create(&th, NULL, reader, nueva_sucursal); // Crear hilo 1
        sem_post(&sem_thread_creation);
        break;
      }

      pthread_join(th, NULL);
    }
  }

  sleep(1);
}
